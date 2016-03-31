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
using web::http::status_codes;
using web::http::uri;

using web::json::value;

using web::http::experimental::listener::http_listener;

using prop_vals_t = vector<pair<string,value>>;

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

/*
  Cache of opened tables
 */
TableCache table_cache {};

/*
  Convert properties represented in Azure Storage type
  to prop_vals_t type.
 */
prop_vals_t get_properties (const table_entity::properties_type& properties, prop_vals_t values = prop_vals_t {}) {
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

  Operation names: ReadEntityAdmin, ReadEntityAuth

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
      JSON object representing an array where each element is a property
      represented by a string / JSON value pair. The first value of each element
      is the property name, and the second value of each element is "*".
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
  // [0] refers to the operation name
  // Evaluated after size() to ensure legitimate access
  //Check for use of admin or auth
  else if (paths[0] != read_entity_admin && paths[0] != read_entity_auth)
  {
    message.reply(status_codes::BadRequest);
    return;
  }
  // Check for specified table
  cloud_table table {table_cache.lookup_table(paths[1])};
  if ( ! table.exists()) {
    message.reply(status_codes::NotFound);
    return;
  }

  // GET all from table or with specific properties
  if (paths.size() == 2) {
  	unordered_map<string,string> json_body = get_json_body (message);
    for(const auto v : json_body){
      if(v.second != "*"){
        message.reply(status_codes::BadRequest);
        return;
      }
    }
    //Get all with specific properties
    if(json_body.size() > 0){
      // creating vector for all properties to loop through later
      table_query query {};
      table_query_iterator end;
      table_query_iterator it = table.execute_query(query);
      vector<value> key_vec;
      while (it != end) {
        cout << "Key: " << it->partition_key() << " / " << it->row_key() << endl;
        prop_vals_t keys {
          make_pair("Partition",value::string(it->partition_key())),
          make_pair("Row", value::string(it->row_key()))};
        keys = get_properties(it->properties(), keys);

        bool found_all_properties = true;
        for(const auto desired_property : json_body) {
          bool found = false;
          for(const auto property : keys) {
            if(desired_property.first == property.first) {
              found = true;
            }
          }
          if(!found) {
            found_all_properties = false;
          }
        }

        if(found_all_properties) {
          key_vec.push_back(value::object(keys));
        }
        ++it;
      }
      message.reply(status_codes::OK, value::array(key_vec));
    }
    //Get all from table
    else{
      table_query query {};
      table_query_iterator end;
      table_query_iterator it = table.execute_query(query);
      vector<value> key_vec;
      while (it != end) {
        cout << "Key: " << it->partition_key() << " / " << it->row_key() << endl;
        prop_vals_t keys {
    make_pair("Partition",value::string(it->partition_key())),
    make_pair("Row", value::string(it->row_key()))};
        keys = get_properties(it->properties(), keys);
        key_vec.push_back(value::object(keys));
        ++it;
      }
      message.reply(status_codes::OK, value::array(key_vec));
    }
    return;
  }

  // GET all entities from a specific partition: Partition == paths[1], * == paths[2]
  // Checking for malformed request
  if (paths.size() == 3 || paths[2] == "")
  {
    //Path includes table and partition but no row
    //Or table and row but no partition
    //Or partition and row but no table
    message.reply(status_codes::BadRequest);
    return;
  }
  // User has indicated they want all items in this partition by the `*`
  if (paths.size() == 4 && paths[3] == "*")
  {
    // Create Query
    table_query query {};
    table_query_iterator end;
    query.set_filter_string(azure::storage::table_query::generate_filter_condition("PartitionKey", azure::storage::query_comparison_operator::equal, paths[2]));
    // Execute Query
    table_query_iterator it = table.execute_query(query);
    // Parse into vector
    vector<value> key_vec;
    while (it != end) {
      cout << "Key: " << it->partition_key() << " / " << it->row_key() << endl;
      prop_vals_t keys {
  make_pair("Partition",value::string(it->partition_key())),
  make_pair("Row", value::string(it->row_key()))};
      keys = get_properties(it->properties(), keys);
      key_vec.push_back(value::object(keys));
      ++it;
    }
    // message reply
    message.reply(status_codes::OK, value::array(key_vec));
    return;
  }

  // GET specific entry: Partition == paths[1], Row == paths[2]
  //Setting up for operation
  table_operation retrieve_operation {table_operation::retrieve_entity(paths[2], paths[3])};
  table_result retrieve_result;
  // Check if using auth
  if (paths[0] == read_entity_auth)
  {
  	// Retrieve entity using token method
  	pair<web::http::status_code,table_entity> result_pair = read_with_token(message, tables_endpoint);
  	// Convert into results type
  	retrieve_result.set_http_status_code(result_pair.first);
  	retrieve_result.set_entity(result_pair.second);
  }
  // Using Admin
  else if(paths[0] == read_entity_admin)
  {
  	// Retrieve item as usual
  	retrieve_result = table.execute(retrieve_operation);
  }
  //Check status codes
  cout << "HTTP code: " << retrieve_result.http_status_code() << endl;
  if (retrieve_result.http_status_code() == status_codes::NotFound) {   // if the table does not exist
    message.reply(status_codes::NotFound);
    return;
  }
  //Place entity and parse properties
  table_entity entity {retrieve_result.entity()};
  table_entity::properties_type properties {entity.properties()};

  // If the entity has any properties, return them as JSON
  prop_vals_t values (get_properties(properties));
  if (values.size() > 0)
    message.reply(status_codes::OK, value::object(values));
  else
    message.reply(status_codes::OK);
}

/*
  Top-level routine for processing all HTTP POST requests.

  HTTP URL for this server is defined in this file as http://localhost:34568.

  Operation:
    Creates a table or ensures the table exists.
  Body:
    None.
  URI:
    http://localhost:34568/CreateTableAdmin/TABLE_NAME
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
  else if(paths[0] == "AddPropertyAdmin" || paths[0] == "UpdatePropertyAdmin"){
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
