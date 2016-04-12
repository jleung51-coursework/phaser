/*
 Authorization Server code for CMPT 276, Spring 2016.

 As defined in ServerUrls.h, the URI for this server is
 http://localhost:34570.
 */

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <cpprest/http_listener.h>
#include <cpprest/json.h>

#include <was/common.h>
#include <was/table.h>

#include "../include/make_unique.h"
#include "../include/ServerUrls.h"
#include "../include/TableCache.h"

#include "../include/azure_keys.h"

using azure::storage::storage_exception;
using azure::storage::cloud_table;
using azure::storage::cloud_table_client;
using azure::storage::edm_type;
using azure::storage::entity_property;
using azure::storage::query_comparison_operator;
using azure::storage::query_logical_operator;
using azure::storage::table_entity;
using azure::storage::table_operation;
using azure::storage::table_query;
using azure::storage::table_query_iterator;
using azure::storage::table_request_options;
using azure::storage::table_result;
using azure::storage::table_shared_access_policy;

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

using prop_str_vals_t = vector<pair<string,string>>;

const string auth_table_name {"AuthTable"};
const string auth_table_userid_partition {"Userid"};
const string auth_table_password_prop {"Password"};
const string auth_table_partition_prop {"DataPartition"};
const string auth_table_row_prop {"DataRow"};
const string data_table_name {"DataTable"};

const string get_read_token_op {"GetReadToken"};
const string get_update_token_op {"GetUpdateToken"};
const string get_update_data_op {"GetUpdateData"};

/*
  Cache of opened tables
 */
TableCache table_cache {};

/*
  Convert properties represented in Azure Storage type
  to prop_str_vals_t type.
 */
prop_str_vals_t get_string_properties (const table_entity::properties_type& properties) {
  prop_str_vals_t values {};
  for (const auto v : properties) {
    if (v.second.property_type() == edm_type::string) {
      values.push_back(make_pair(v.first,v.second.string_value()));
    }
    else {
      // Force the value as string in any case
      values.push_back(make_pair(v.first, v.second.str()));
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
  Return a token for 24 hours of access to the specified table,
  for the single entity defind by the partition and row.

  permissions: A bitwise OR ('|')  of table_shared_access_poligy::permission
    constants.

    For read-only:
      table_shared_access_policy::permissions::read
    For read and update:
      table_shared_access_policy::permissions::read |
      table_shared_access_policy::permissions::update
 */
pair<status_code,string> do_get_token (const cloud_table& data_table,
                   const string& partition,
                   const string& row,
                   uint8_t permissions) {

  utility::datetime exptime {utility::datetime::utc_now() + utility::datetime::from_days(1)};
  try {
    string limited_access_token {
      data_table.get_shared_access_signature(table_shared_access_policy {
                                               exptime,
                                               permissions},
                                             string(), // Unnamed policy
                                             // Start of range (inclusive)
                                             partition,
                                             row,
                                             // End of range (inclusive)
                                             partition,
                                             row)
        // Following token allows read access to entire table
        //table.get_shared_access_signature(table_shared_access_policy {exptime, permissions})
      };
    cout << "Token " << limited_access_token << endl;
    return make_pair(status_codes::OK, limited_access_token);
  }
  catch (const storage_exception& e) {
    cout << "Azure Table Storage error: " << e.what() << endl;
    cout << e.result().extended_error().message() << endl;
    return make_pair(status_codes::InternalError, string{});
  }
}

/*
  Top-level routine for processing all HTTP GET requests.

  HTTP URL for this server is defined in this file as http://localhost:34570.

  Possible operations:

    Operation name:
      GetReadToken
    Operation:
      Returns a JSON object with a single property named "token", with the
      value of a string which is the authentication token from Microsoft Azure.
      The authentication token allows for read operations ONLY.
    Body:
      JSON object with a single property named "Password", with the value of
      a string which is the password for the user ID provided in the URI.
    URI:
      http://localhost:34570/GetReadToken/USER_ID

  TODO: GetUpdateToken has not been implemented yet.
 */
void handle_get(http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** AuthServer GET " << path << endl;
  auto paths = uri::split_path(path);
  // Need at least an operation and userid
  if (paths.size() < 2) {
    message.reply(status_codes::BadRequest);
    return;
  }
  // [0] refers to the operation name
  // Evaluated after size() to ensure legitimate access
  else if(paths[0] != get_read_token_op && paths[0] != get_update_token_op && paths[0] != get_update_data_op ) {
    message.reply(status_codes::BadRequest);
    return;
  }
  else if(!has_json_body(message)) {
    message.reply(status_codes::BadRequest);
    return;
  }

  unordered_map<string, string> json_body {get_json_bourne(message)};
  unordered_map<string, string>::const_iterator json_body_password_iterator
    {json_body.find("Password")};

  if(json_body.size() != 1) {
    message.reply(status_codes::BadRequest);
    return;
  }
  // No 'Password' property
  else if(json_body_password_iterator == json_body.end()) {
    message.reply(status_codes::BadRequest);
    return;
  }

  const string userid = paths[1];
  const string password_given = json_body_password_iterator->second;
  string password_actual;
  string authenticated_partition;
  string authenticated_row;

  if(password_given.empty()) {
    message.reply(status_codes::BadRequest);
  }

  cloud_table table {table_cache.lookup_table(auth_table_name)};
  if(!table.exists()) {
    message.reply(status_codes::InternalError);
    return;
  }

  // Search through the table AuthTable, partition Userid
  table_query query {};
  query.set_filter_string( table_query::combine_filter_conditions(
    table_query::generate_filter_condition(
      U("PartitionKey"),
      query_comparison_operator::equal,
      U("Userid")
    ),
    query_logical_operator::op_and,
    table_query::generate_filter_condition(
      U("RowKey"),
      query_comparison_operator::equal,
      U(userid)
    )
  ));
  table_query_iterator end;
  table_query_iterator it = table.execute_query(query);
  if(it == end) {
    // User ID not found
    message.reply(status_codes::NotFound);
    return;
  }

  prop_str_vals_t properties = get_string_properties(it->properties());
  for(auto p : properties) {
    string property_name = p.first;
    if(property_name == auth_table_password_prop) {
      password_actual = p.second;
    }
    else if(property_name == auth_table_partition_prop) {
      authenticated_partition = p.second;
    }
    else if(property_name == auth_table_row_prop) {
      authenticated_row = p.second;
    }
    else {
      // Invalid property
      message.reply(status_codes::InternalError);
      return;
    }
  }

  if( password_actual.empty() ||
      authenticated_partition.empty() ||
      authenticated_row.empty() ) {
    // At least one of the necessary properties not found
    message.reply(status_codes::InternalError);
    return;
  }
  else if(password_given != password_actual) {
    // Incorrect Password
    // Same status code as incorrect user ID for security purposes
    message.reply(status_codes::NotFound);
    return;
  }

  table = table_cache.lookup_table(data_table_name);
  if(!table.exists()) {
    message.reply(status_codes::InternalError);
    return;
  }

  // get a read or read|update token
  pair<status_code, string> result;
  if(paths[0] == get_read_token_op ) {
  result = do_get_token
  (
    table,
    authenticated_partition,
    authenticated_row,
    table_shared_access_policy::permissions::read
  );
  }
  else {
  result = do_get_token
  (
    table,
    authenticated_partition,
    authenticated_row,
    table_shared_access_policy::permissions::read | table_shared_access_policy::permissions::update
  );
  }

  if(result.first == status_codes::OK) {
    vector<pair<string, value>> json_token;
    if ( paths[0] == get_update_token_op || paths[0] == get_read_token_op ){
      json_token.push_back( make_pair("token", value::string(result.second)));
    }
    if ( paths[0] == get_update_data_op ){
      json_token.push_back( make_pair("token", value::string(result.second)) );
      json_token.push_back( make_pair("DataPartition", value::string(authenticated_partition)) );
      json_token.push_back( make_pair("DataRow", value::string(authenticated_row)) );

      cout << "Partition " << value::string(authenticated_partition) << endl;
      cout << "Row " << value::string(authenticated_row) << endl;
    }


    message.reply(result.first, value::object(json_token));
    return;
  }


  else {
    message.reply(result.first);
    return;
  }
}
/*
void get_update_data_print(string partition, string row){
  cout << "Partition" << partition << endl;
  cout << "Row" << row << endl;
}*/

/*
  Top-level routine for processing all HTTP POST requests.
 */
void handle_post(http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** POST " << path << endl;
}

/*
  Top-level routine for processing all HTTP PUT requests.
 */
void handle_put(http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** PUT " << path << endl;
}

/*
  Top-level routine for processing all HTTP DELETE requests.
 */
void handle_delete(http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** DELETE " << path << endl;
}

/*
  Main authentication server routine

  Install handlers for the HTTP requests and open the listener,
  which processes each request asynchronously.

  Note that, unlike BasicServer, AuthServer only
  installs the listeners for GET. Any other HTTP
  method will produce a Method Not Allowed (405)
  response.

  If you want to support other methods, uncomment
  the call below that hooks in a the appropriate
  listener.

  Wait for a carriage return, then shut the server down.
 */
int main (int argc, char const * argv[]) {
  cout << "AuthServer: Parsing connection string" << endl;
  table_cache.init (storage_connection_string);

  cout << "AuthServer: Opening listener" << endl;
  http_listener listener {server_urls::auth_server};
  listener.support(methods::GET, &handle_get);
  //listener.support(methods::POST, &handle_post);
  //listener.support(methods::PUT, &handle_put);
  //listener.support(methods::DEL, &handle_delete);
  listener.open().wait(); // Wait for listener to complete starting

  cout << "Enter carriage return to stop AuthServer." << endl;
  string line;
  getline(std::cin, line);

  // Shut it down
  listener.close().wait();
  cout << "AuthServer closed" << endl;
}
