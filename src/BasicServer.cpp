/*
 Basic Server code for CMPT 276, Spring 2016.
 */

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdexcept>
#include <string>

#include <cpprest/base_uri.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>

#include <pplx/pplxtasks.h>

#include <was/common.h>
#include <was/storage_account.h>
#include <was/table.h>

#include "../include/TableCache.h"

#include "../include/make_unique.h"

#include "../include/azure_keys.h"

#include "../include/ServerUtils.h"

using azure::storage::cloud_storage_account;
using azure::storage::storage_credentials;
using azure::storage::storage_exception;
using azure::storage::cloud_table;
using azure::storage::cloud_table_client;
using azure::storage::edm_type;
using azure::storage::entity_property;
using azure::storage::table_entity;
using azure::storage::table_operation;
using azure::storage::table_query;
using azure::storage::table_query_iterator;
using azure::storage::table_result;

using pplx::extensibility::critical_section_t;
using pplx::extensibility::scoped_critical_section_t;

using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::make_pair;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

using web::http::http_headers;
using web::http::http_request;
using web::http::methods;
using web::http::status_code;
using web::http::status_codes;
using web::http::uri;

using web::json::value;

using web::http::experimental::listener::http_listener;

using prop_vals_t = vector<pair<string,value>>;

/*
  Convert properties represented in Azure Storage type
  to prop_vals_t type.
 */
prop_vals_t get_properties (const table_entity::properties_type& properties, prop_vals_t values = prop_vals_t {});

/*
  Return true if an HTTP request has a JSON body

  This routine can be called multiple times on the same message.
 */
bool has_json_body (http_request message);

/*
  Given an HTTP message with a JSON body, return the JSON
  body as an unordered map of strings to strings.
  get_json_body and get_json_bourne are valid and identical function calls.

  If the message has no JSON body, return an empty map.

  THIS ROUTINE CAN ONLY BE CALLED ONCE FOR A GIVEN MESSAGE
  (see http://microsoft.github.io/cpprestsdk/classweb_1_1http_1_1http__request.html#ae6c3d7532fe943de75dcc0445456cbc7
  for source of this limit).

  Note that all types of JSON values are returned as strings.
  Use C++ conversion utilities to convert to numbers or dates
  as necessary.
 */
unordered_map<string,string> get_json_body(http_request message);
unordered_map<string,string> get_json_bourne(http_request message);

// Unnamed namespace for local functions and structures
namespace {

struct get_request_t {
  string operation {};
  string token {};
  string table {};
  string partition {};
  string row {};
  unsigned int paths_count {};
};

constexpr const char* def_url = "http://localhost:34568";

//Unauthorized Options
const string create_table {"CreateTable"};
const string delete_table {"DeleteTable"};
const string update_entity {"UpdateEntity"};
const string delete_entity {"DeleteEntity"};

//Authorized Operations
const string create_table_op {"CreateTableAdmin"};
const string delete_table_op {"DeleteTableAdmin"};

const string read_entity_admin {"ReadEntityAdmin"};
const string update_entity_admin {"UpdateEntityAdmin"};
const string delete_entity_admin {"DeleteEntityAdmin"};

const string read_entity_auth {"ReadEntityAuth"};
const string update_entity_auth {"UpdateEntityAuth"};

const string get_read_token_op  {"GetReadToken"};
const string get_update_token_op {"GetUpdateToken"};

// The two optional operations from Assignment 1
const string add_property_admin {"AddPropertyAdmin"};
const string update_property_admin {"UpdatePropertyAdmin"};

// Cache of opened tables
TableCache table_cache {};

/*
  This local function returns the contents of the GET request in a
  get_request_t variable.
  The returned request parameters are not guaranteed to be valid for the table.

  An exception will be thrown if:
    The operation name is invalid (invalid_argument)
    The request is incorrectly formed in terms of the number of paths
      (see documentation for handle_get) (invalid_argument)
 */
get_request_t parse_get_request_paths(http_request message) {
  string path { uri::decode(message.relative_uri().path()) };
  auto paths = uri::split_path(path);

  get_request_t req;
  req.paths_count = paths.size();

  if(req.paths_count < 1) {
    throw std::invalid_argument ("Error: parse_get_request_paths() was "\
      "given an incorrectly-formed request.\n");
  }
  req.operation = paths[0];

  if(req.operation == read_entity_admin) {
    switch(req.paths_count) {
      case 2:
        req.table = paths[1];
        break;
      case 4:
        req.table = paths[1];
        req.partition = paths[2];
        req.row = paths[3];
        break;
      default:
        throw std::invalid_argument ("Error: parse_get_request_paths() was "\
          "given an incorrectly-formed request.\n");
    }
  }
  else if(req.operation == read_entity_auth) {
    switch(req.paths_count) {
      case 5:
        req.table = paths[1];
        req.token = paths[2];
        req.partition = paths[3];
        req.row = paths[4];
        break;
      default:
        throw std::invalid_argument ("Error: parse_get_request_paths() was "\
          "given an incorrectly-formed request.\n");
    }
  }
  else {
    throw std::invalid_argument ("Error: parse_get_request_paths() was given "\
      "an invalid operation.\n");
  }

  return req;
}

/*
  This local function returns a vector of JSON objects.
  If the JSON body has no elements, then the vector will contain all
  entities in the table.
  If the JSON body has elements, then the vector will contain all
  entities in the table which have all of the properties in the JSON body
  (regardless of value).
  Each element in the vector is a single entity.

  JSON body:
    JSON object where the name is the property name and the value is "*".
    E.g. {"born":"*", "art":"*"} would return all entities in the requested
    table which have properties "born" and "art".

  An exception is thrown if:
    The operation is incorrect or nonexistent (invalid_argument)
    The table name is incorrect or nonexistent (invalid_argument)
    Not all elements in the JSON body have the value "*" (invalid_argument)
 */
vector<value> get_table_or_properties(get_request_t request,
                                      unordered_map<string, string> json_body) {

  if (request.operation != read_entity_admin) {
    throw std::invalid_argument ("Error: get_table_or_properties() was "\
      "given an invalid operation.\n");
  }

  // Check for specified table
  cloud_table table {table_cache.lookup_table(request.table)};
  if ( !table.exists() ) {
    throw std::invalid_argument ("Error: get_table_or_properties() was "\
      "given an invalid table name.\n");
  }

  for(const auto v : json_body){
    if(v.second != "*"){
      throw std::invalid_argument ("Error: get_table_or_properties() was "\
        "given a JSON body which had at least one element with a value "\
        "other than \"*\".\n");
    }
  }

  // Creating vector for all properties to loop through later
  table_query query {};
  table_query_iterator end;
  table_query_iterator it = table.execute_query(query);
  vector<value> entities;

  while (it != end) {
    cout << "Key: " << it->partition_key() << " / " << it->row_key() << endl;

    prop_vals_t entity {
      make_pair("Partition",value::string( it->partition_key() )),
      make_pair("Row", value::string( it->row_key() ))
    };
    entity = get_properties(it->properties(), entity);

    if(json_body.size() == 0) {
      entities.push_back(value::object(entity));
    }
    else {
      bool found_all_properties = true;
      for(const auto desired_property : json_body) {
        bool found_property = false;
        for(const auto p : entity) {
          if(desired_property.first == p.first) {
            found_property = true;
          }
        }
        if(!found_property) {
          found_all_properties = false;
        }
      }

      if(found_all_properties) {
        entities.push_back(value::object(entity));
      }
    }

    ++it;
  }

  return entities;
}

/*
  This local function returns a vector of JSON objects with all entities in a
  requested partition. Each element is a single entity.

  An exception is thrown if:
    The operation is incorrect or nonexistent (invalid_argument)
    The table name is incorrect or nonexistent (invalid_argument)
    The row name is not "*" (logic_error)
 */
vector<value> get_partition(get_request_t request) {
  if (request.operation != read_entity_admin) {
    throw std::invalid_argument ("Error: get_partition() was given an "\
      "invalid operation.\n");
  }

  // Check for specified table
  cloud_table table {table_cache.lookup_table(request.table)};
  if ( !table.exists() ) {
    throw std::invalid_argument ("Error: get_partition() was given an "\
      "invalid table name.\n");
  }

  // Placed here for logical order - operation, table, row
  if (request.row != "*") {
    throw std::logic_error ("Error: get_partition() was given an "\
      "invalid row name (which should be \"*\").\n");
  }

  // Create Query
  table_query query {};
  table_query_iterator end;
  query.set_filter_string(
    azure::storage::table_query::generate_filter_condition(
      "PartitionKey",
      azure::storage::query_comparison_operator::equal,
      request.partition
    )
  );
  table_query_iterator it = table.execute_query(query);

  // Parse into vector
  vector<value> entity_vec;
  while (it != end) {
    cout << "Key: " << it->partition_key() << " / " << it->row_key() << endl;

    prop_vals_t entity {
      make_pair( "Partition", value::string(it->partition_key()) ),
      make_pair( "Row", value::string(it->row_key()) )
    };
    entity = get_properties(it->properties(), entity);
    entity_vec.push_back(value::object(entity));
    ++it;
  }

  return entity_vec;
}

/*
  This local function returns all properties of a requested entity.
  Any error when authenticating with the token will return a status code
  other than status_codes::OK.

  An exception is thrown if:
    The operation is incorrect or nonexistent (invalid_argument)
    The table name is incorrect or nonexistent (invalid_argument)
    The partition name is nonexistent (invalid_argument)
    The row name is nonexistent (invalid_argument)
    The row name is "*" (logic_error)
    The operation is ReadEntityAuth but the token is nonexistent (logic_error)
 */
pair<status_code, prop_vals_t> get_specific(http_request message,
                                            get_request_t request) {
  if (request.operation != read_entity_admin &&
      request.operation != read_entity_auth) {
    throw std::invalid_argument ("Error: get_specific() was given an "\
      "invalid operation.\n");
  }

  // Check for specified table
  cloud_table table {table_cache.lookup_table(request.table)};
  if ( !table.exists() ) {
    throw std::invalid_argument ("Error: get_specific() was given an "\
      "invalid table name.\n");
  }

  if (request.partition == "") {
    throw std::invalid_argument ("Error: get_specific was not given a "\
      "partition name.\n");
  }
  else if (request.row == "") {
    throw std::invalid_argument ("Error: get_specific was not given a "\
      "row name.\n");
  }
  else if (request.row == "*") {
    throw std::logic_error ("Error: get_specific() was given the "\
      "row name \"*\", which should be for get_partition().\n");
  }
  else if (request.operation == read_entity_auth && request.token == "") {
    throw std::logic_error ("Error: get_specific() was given the "\
      "operation ReadEntityAuth, but was not given a token.\n");
  }

  table_operation retrieve_operation {
    table_operation::retrieve_entity(
      request.partition,
      request.row
    )
  };

  table_result retrieve_result;
  if (request.operation == read_entity_auth)
  {
  	// Retrieve entity using token method
  	pair<web::http::status_code, table_entity> result_pair =
      read_with_token(message, tables_endpoint);
  	// Convert into results type
  	retrieve_result.set_http_status_code(result_pair.first);
  	retrieve_result.set_entity(result_pair.second);
  }
  else if(request.operation == read_entity_admin)
  {
  	retrieve_result = table.execute(retrieve_operation);
  }

  //Check status codes
  cout << "HTTP code: " << retrieve_result.http_status_code() << endl;
  if (retrieve_result.http_status_code() == status_codes::NotFound) {
    prop_vals_t empty_prop;
    return make_pair(status_codes::NotFound, empty_prop);
  }
  //Place entity and parse properties
  table_entity entity {retrieve_result.entity()};
  table_entity::properties_type properties {entity.properties()};

  // If the entity has any properties, return them as JSON
  prop_vals_t values (get_properties(properties));
  return make_pair(status_codes::OK, values);
}

}  // Unnamed namespace for local functions and structures

/*
  Convert properties represented in Azure Storage type
  to prop_vals_t type.
 */
prop_vals_t get_properties (const table_entity::properties_type& properties, prop_vals_t values) {
  for (const auto v : properties) {
    if (v.second.property_type() == edm_type::string) {
      values.push_back(make_pair(v.first, value::string(v.second.string_value())));
    }
    else if (v.second.property_type() == edm_type::datetime) {
      values.push_back(make_pair(v.first, value::string(v.second.str())));
    }
    else if(v.second.property_type() == edm_type::int32) {
      values.push_back(make_pair(v.first, value::number(v.second.int32_value())));
    }
    else if(v.second.property_type() == edm_type::int64) {
      values.push_back(make_pair(v.first, value::number(v.second.int64_value())));
    }
    else if(v.second.property_type() == edm_type::double_floating_point) {
      values.push_back(make_pair(v.first, value::number(v.second.double_value())));
    }
    else if(v.second.property_type() == edm_type::boolean) {
      values.push_back(make_pair(v.first, value::boolean(v.second.boolean_value())));
    }
    else {
      values.push_back(make_pair(v.first, value::string(v.second.str())));
    }
  }
  return values;
}

/*
  Return true if an HTTP request has a JSON body

  This routine can be called multiple times on the same message.
 */
bool has_json_body (http_request message) {
  return message.headers()["Content-type"] == "application/json";
}

/*
  Given an HTTP message with a JSON body, return the JSON
  body as an unordered map of strings to strings.
  get_json_body and get_json_bourne are valid and identical function calls.

  If the message has no JSON body, return an empty map.

  THIS ROUTINE CAN ONLY BE CALLED ONCE FOR A GIVEN MESSAGE
  (see http://microsoft.github.io/cpprestsdk/classweb_1_1http_1_1http__request.html#ae6c3d7532fe943de75dcc0445456cbc7
  for source of this limit).

  Note that all types of JSON values are returned as strings.
  Use C++ conversion utilities to convert to numbers or dates
  as necessary.
 */

unordered_map<string,string> get_json_body(http_request message) {
  unordered_map<string,string> results {};
  const http_headers& headers {message.headers()};
  auto content_type (headers.find("Content-Type"));
  if (content_type == headers.end() ||
      content_type->second != "application/json")
    return results;

  value json{};
  message.extract_json(true)
    .then([&json](value v) -> bool
	  {
            json = v;
	    return true;
	  })
    .wait();

  if (json.is_object()) {
    for (const auto& v : json.as_object()) {
      if (v.second.is_string()) {
	results[v.first] = v.second.as_string();
      }
      else {
	results[v.first] = v.second.serialize();
      }
    }
  }
  return results;
}

unordered_map<string,string> get_json_bourne(http_request message) {
 return get_json_body(message);
}

/*
  Top-level routine for processing all HTTP GET requests.

  HTTP URL for this server is defined in this file as http://localhost:34568.

  Operation names: ReadEntityAdmin, ReadEntityAuth (with authentication token)

  Possible operations:

    Operation:
      Returns a JSON object with all properties of a requested entity.
    Body:
      None.
    Administrative URI:
      http://localhost:34568/ReadEntityAdmin/TABLE_NAME/PARTITION_NAME/ROW_NAME
      (ROW_NAME cannot be "*")
    Authenticated URI:
      http://localhost:34568/ReadEntityAuth/TABLE_NAME/AUTHENTICATION_TOKEN/PARTITION_NAME/ROW_NAME
      (AUTHENTICATION_TOKEN is obtained from AuthServer)
      (ROW_NAME cannot be "*")

    Operation:
      Returns a JSON array of objects containing all entities in the requested
      table which have all of the requested properties (regardless of value).
      Each element in the JSON array is a single entity.
    Body:
      JSON object where the name is the property name and the value is "*".
      E.g. {"born":"*", "art":"*"} would return all entities in the requested
      table which have properties "born" and "art".
    Administrative URI:
      http://localhost:34568/ReadEntityAdmin/TABLE_NAME
    Authenticated URI:
      http://localhost:34568/ReadEntityAuth/TABLE_NAME/AUTHENTICATION_TOKEN
      (AUTHENTICATION_TOKEN is obtained from AuthServer)

    Operation:
      Returns a JSON array of objects with all entities in a
      requested partition. Each element in the JSON array is a single entity.
    Body:
      None.
    Administrative URI:
      http://localhost:34568/ReadEntityAdmin/TABLE_NAME/PARTITION_NAME/*
      (row name can only be "*")
    Authenticated URI:
      http://localhost:34568/ReadEntityAuth/TABLE_NAME/AUTHENTICATION_TOKEN/PARTITION_NAME/*
      (AUTHENTICATION_TOKEN is obtained from AuthServer)
      (row name can only be "*")

    Operation:
      Returns a JSON array of objects with all entities in a
      requested table. Each element in the JSON array is a single entity,
      and the (JSON) properties of an element are the (database) properties of
      the entity.
      The partition and row names of each entity are returned as if they were
      a property of the entity - that is, as the values of properties named
      "Partition" and "Row" respectively.
    Body:
      None.
    Administrative URI:
      http://localhost:34568/ReadEntityAdmin/TABLE_NAME/PARTITION_NAME/*
      (ROW_NAME can only be "*")
    Authenticated URI:
      http://localhost:34568/ReadEntityAuth/TABLE_NAME/AUTHENTICATION_TOKEN/PARTITION_NAME/*
      (AUTHENTICATION_TOKEN is obtained from AuthServer)
      (ROW_NAME can only be "*")
    // TODO: This does not safely handle a property named "Partition" or "Row".
 */
void handle_get(http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** GET " << path << endl;
  auto paths = uri::split_path(path);
  // Need at least an operation name and table name
  if (paths.size() < 2)
  {
    message.reply(status_codes::BadRequest);
    return;
  }

  // Checking for well-formed request before passing to
  // parse_get_request_paths()
  // [0] refers to the operation name
  // Evaluated after size() to ensure legitimate access
  if (paths[0] == read_entity_admin) {

    if(paths.size() != 2 &&
       paths.size() != 4) {
      message.reply(status_codes::BadRequest);
      return;
    }
  }

  else if (paths[0] == read_entity_auth) {

    if(paths.size() != 5) {
      message.reply(status_codes::BadRequest);
      return;
    }
  }

  // Incorrect operation name
  else {
    message.reply(status_codes::BadRequest);
    return;
  }

  get_request_t request;
  try {
    request = parse_get_request_paths(message);
  }
  catch( const std::exception& e ) {
    cout << e.what();
    message.reply(status_codes::InternalError);
    return;
  }

  // Check for specified table
  cloud_table table {table_cache.lookup_table(request.table)};
  if ( ! table.exists()) {
    message.reply(status_codes::NotFound);
    return;
  }

  // Get all entities in the table, or
  // Get all entities in the table with specific properties
  if (request.paths_count == 2) {
    unordered_map<string,string> json_body = get_json_body (message);
    for(const auto v : json_body){
      if(v.second != "*"){
        message.reply(status_codes::BadRequest);
        return;
      }
    }
    vector<value> v;
    try {
      v = get_table_or_properties(request, json_body);
    }
    catch(const std::exception& e) {
      cout << e.what();
      message.reply(status_codes::InternalError);
      return;
    }
    message.reply(status_codes::OK, value::array(v));
    return;
  }

  // Get all entities in the partition
  else if (request.paths_count == 4 && request.row == "*")
  {
    vector<value> entities;
    try {
      entities = get_partition(request);
    }
    catch (const std::exception& e) {
      cout << e.what();
      message.reply(status_codes::InternalError);
      return;
    }

    message.reply(status_codes::OK, value::array(entities));
    return;
  }

  // Get a specific entity (administrative or authorized)
  else if( (request.paths_count == 4 &&
            request.operation == read_entity_admin) ||
           (request.paths_count == 5 &&
            request.operation == read_entity_auth) ) {

    pair<status_code, prop_vals_t> result;
    try {
      result = get_specific(message, request);
    }
    catch(const std::exception& e) {
      cout << e.what();
      message.reply(status_codes::InternalError);
      return;
    }

    if(result.first != status_codes::OK) {
      message.reply(result.first);
      return;
    }
    else if (result.second.size() > 0) {
      message.reply(status_codes::OK, value::object(result.second));
      return;
    }
    else {
      message.reply(status_codes::OK);
      return;
    }
  }

  // Invalid/badly-formed request was not caught earlier
  else {
    message.reply(status_codes::InternalError);
    return;
  }
}

/*
  Top-level routine for processing all HTTP POST requests.
 */
void handle_post(http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** POST " << path << endl;

  auto paths = uri::split_path(path);
  // Need at least an operation and a table name
  if (paths.size() < 2) {
    message.reply(status_codes::BadRequest);
    return;
  }
  // [0] refers to the operation name
  // Evaluated after size() to ensure legitimate access
  else if (paths[0] != create_table_op) {
    message.reply(status_codes::BadRequest);
    return;
  }

  string table_name {paths[1]};
  cloud_table table {table_cache.lookup_table(table_name)};

  // Create table (idempotent if table exists)
  cout << "Create " << table_name << endl;
  bool created {table.create_if_not_exists()};
  cout << "Administrative table URI " << table.uri().primary_uri().to_string() << endl;
  if (created)
    message.reply(status_codes::Created);
  else
    message.reply(status_codes::Accepted);
}

/*
  Top-level routine for processing all HTTP PUT requests.
 */
void handle_put(http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** PUT " << path << endl;

  auto paths = uri::split_path(path);
  // Need at least an operation, table name, partition, and row
  if (paths.size() < 4) {
    message.reply(status_codes::BadRequest);
    return;
  }
  // [0] refers to the operation name
  // Evaluated after size() to ensure legitimate access
  else if(paths[0] == add_property_admin || paths[0] == update_property_admin){
    message.reply(status_codes::NotImplemented);
    return;
  }
  else if(paths[0] == update_entity_auth){
    unordered_map<string,string> json_body {get_json_bourne (message)};
    message.reply(update_with_token (message, tables_endpoint, json_body));
    return;
  }
  else if(paths[0] == add_property_admin || paths[0] == update_property_admin) {
    message.reply(status_codes::NotImplemented);
    return;
  }
  else if (paths[0] != update_entity_admin) {
    message.reply(status_codes::BadRequest);
    return;
  }

  unordered_map<string,string> json_body {get_json_bourne (message)};

  cloud_table table {table_cache.lookup_table(paths[1])};
  if ( ! table.exists()) {
    message.reply(status_codes::NotFound);
    return;
  }

  table_entity entity {paths[2], paths[3]}; // partition and row

  // Update entity
  cout << "Update " << entity.partition_key() << " / " << entity.row_key() << endl;
  table_entity::properties_type& properties = entity.properties();
  for (const auto v : json_body) {
    properties[v.first] = entity_property {v.second};
  }

  table_operation operation {table_operation::insert_or_merge_entity(entity)};
  table_result op_result {table.execute(operation)};

  message.reply(status_codes::OK);
  return;
}

/*
  Top-level routine for processing all HTTP DELETE requests.
 */
void handle_delete(http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** DELETE " << path << endl;
  auto paths = uri::split_path(path);
  // Need at least an operation and table name
  if (paths.size() < 2) {
	message.reply(status_codes::BadRequest);
	return;
  }

  string table_name {paths[1]};
  cloud_table table {table_cache.lookup_table(table_name)};

  // Delete table
  if (paths[0] == delete_table_op) {
    cout << "Delete " << table_name << endl;
    if ( ! table.exists()) {
      message.reply(status_codes::NotFound);
    }
    table.delete_table();
    table_cache.delete_entry(table_name);
    message.reply(status_codes::OK);
  }
  // Delete entity
  else if (paths[0] == delete_entity_admin) {
    // For delete entity, also need partition and row
    if (paths.size() < 4) {
	message.reply(status_codes::BadRequest);
	return;
    }
    table_entity entity {paths[2], paths[3]};
    cout << "Delete " << entity.partition_key() << " / " << entity.row_key()<< endl;

    table_operation operation {table_operation::delete_entity(entity)};
    table_result op_result {table.execute(operation)};

    int code {op_result.http_status_code()};
    if (code == status_codes::NoContent) {
      code = status_codes::OK;
    }
    message.reply(code);
  }
  else {
    message.reply(status_codes::BadRequest);
  }
}

/*
  Main server routine

  Install handlers for the HTTP requests and open the listener,
  which processes each request asynchronously.

  Wait for a carriage return, then shut the server down.
 */
int main (int argc, char const * argv[]) {
  cout << "Parsing connection string" << endl;
  table_cache.init (storage_connection_string);

  cout << "Opening listener" << endl;
  http_listener listener {def_url};
  listener.support(methods::GET, &handle_get);
  listener.support(methods::POST, &handle_post);
  listener.support(methods::PUT, &handle_put);
  listener.support(methods::DEL, &handle_delete);
  listener.open().wait(); // Wait for listener to complete starting

  cout << "Enter carriage return to stop server." << endl;
  string line;
  getline(std::cin, line);

  // Shut it down
  listener.close().wait();
  cout << "Closed" << endl;
}
