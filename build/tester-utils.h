/*
  This C++ header file contains general testing utilities for the servers.
 */

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <cpprest/http_client.h>
#include <cpprest/json.h>

#include <pplx/pplxtasks.h>

#include <UnitTest++/UnitTest++.h>
#include <UnitTest++/TestReporterStdout.h>

using std::cout;
using std::cerr;
using std::endl;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

using web::http::http_headers;
using web::http::http_request;
using web::http::http_response;
using web::http::method;
using web::http::methods;
using web::http::status_code;
using web::http::status_codes;
using web::http::uri_builder;

using web::http::client::http_client;

using web::json::object;
using web::json::value;

namespace rest_operations {

const string create_table_op {"CreateTableAdmin"};
const string delete_table_op {"DeleteTableAdmin"};

const string read_entity_admin {"ReadEntityAdmin"};
const string update_entity_admin {"UpdateEntityAdmin"};
const string delete_entity_admin {"DeleteEntityAdmin"};

const string read_entity_auth {"ReadEntityAuth"};
const string update_entity_auth {"UpdateEntityAuth"};

const string get_read_token_op  {"GetReadToken"};
const string get_update_token_op {"GetUpdateToken"};
const string get_update_data_op {"GetUpdateData"};

// The two optional operations from Assignment 1
const string add_property_admin {"AddPropertyAdmin"};
const string update_property_admin {"UpdatePropertyAdmin"};

// UserServer Operations
const string sign_on {"SignOn"};
const string sign_off {"SignOff"};
const string add_friend {"AddFriend"};
const string un_friend {"UnFriend"};
const string update_status {"UpdateStatus"};
const string read_friend_list {"ReadFriendList"};

}

/*
  Make an HTTP request, returning the status code and any JSON value in the body

  method: member of web::http::methods
  uri_string: uri of the request
  req_body: [optional] a json::value to be passed as the message body

  If the response has a body with Content-Type: application/json,
  the second part of the result is the json::value of the body.
  If the response does not have that Content-Type, the second part
  of the result is simply json::value {}.
 */

// Version with explicit third argument
pair<status_code,value> do_request (const method& http_method, const string& uri_string, const value& req_body);

// Version that defaults third argument
pair<status_code,value> do_request (const method& http_method, const string& uri_string);

/*
  Utility to create a table

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
 */
int create_table (const string& addr, const string& table);

/*
  Utility to compare two JSON objects

  This is an internal routine---you probably want to call compare_json_values().
 */
bool compare_json_objects (const object& expected_o, const object& actual_o);

/*
  Utility to compare two JSON objects represented as values

  expected: json::value that was expected---must be an object
  actual: json::value that was actually returned---must be an object
*/
bool compare_json_values (const value& expected, const value& actual);

/*
  Utility to compre expected JSON array with actual

  exp: vector of objects, sorted by Partition/Row property
    The routine will throw if exp is not sorted.
  actual: JSON array value of JSON objects
    The routine will throw if actual is not an array or if
    one or more values is not an object.

  Note the deliberate asymmetry of the how the two arguments are handled:

  exp is set up by the test, so we *require* it to be of the correct
  type (vector<object>) and to be sorted and throw if it is not.

  actual is returned by the database and may not be an array, may not
  be values, and may not be sorted by partition/row, so we have
  to check whether it has those characteristics and convert it
  to a type comparable to exp.
*/
bool compare_json_arrays(const vector<object>& exp, const value& actual);

/*
  Utility to create JSON object value from vector of properties
*/
value build_json_object (const vector<pair<string,string>>& properties);

/*
  Utility to delete a table

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
 */
int delete_table (const string& addr, const string& table);

/*
  Utility to put an entity with a single property

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
  partition: Partition of the entity
  row: Row of the entity
  prop: Name of the property
  pstring: Value of the property, as a string
 */
int put_entity(const string& addr, const string& table, const string& partition, const string& row, const string& prop, const string& pstring);

/*
  Utility to put an entity with multiple properties

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
  partition: Partition of the entity
  row: Row of the entity
  props: vector of string/value pairs representing the properties
 */
int put_entity(const string& addr, const string& table, const string& partition, const string& row,
              const vector<pair<string,value>>& props);

/*
  Utility to delete an entity

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
  partition: Partition of the entity
  row: Row of the entity
 */
int delete_entity (const string& addr, const string& table, const string& partition, const string& row);

/*
  Utility to get a token good for updating a specific entry
  from a specific table for one day.
 */
pair<status_code,string> get_update_token(const string& addr,  const string& userid, const string& password);

/*
  Utility to get a token good for reading a specific entry
  from a specific table for one day.
 */
pair<status_code,string> get_read_token(const string& addr,  const string& userid, const string& password);
