/*
 Server utilities Assignment 2, CMPT 276, Spring 2016.
 */

#include "ServerUtils.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <was/table.h>

using azure::storage::cloud_table;
using azure::storage::cloud_table_client;
using azure::storage::entity_property;
using azure::storage::storage_credentials;
using azure::storage::storage_exception;
using azure::storage::table_entity;
using azure::storage::table_operation;
using azure::storage::table_result;

using std::cout;
using std::endl;
using std::make_pair;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

using web::http::http_request;
using web::http::status_code;
using web::http::status_codes;
using web::http::uri;

/*
  Read from a table using a security token

  message is used only for its path, which must be split using the undecoded
    URI, as the token may have '/' characters encoded via %2F. After the
    undecoded path is split, the resulting parameters are decoded
    individually.
  endpoint is the URI endpoint for Azure tables. It takes the form
    "http://STORAGE.table.core.windows.net/", where STORAGE is
    replaced by the user's Azure Storage account name.

  Returns a pair:
    first: HTTP status code from the read
    second: if the status code is OK, the entity read from the table
 */
pair<status_code,table_entity> read_with_token (const http_request& message,
                                                 const string& endpoint) {
  /*
    Tokens can contain %2F ('/'). Thus we split the URI path
    *before* decoding and pass the undecoded values to Azure Storage
   */
  const string undecoded_path {message.relative_uri().path()};
  const vector<string> undecoded_paths {uri::split_path(undecoded_path)};
  if (undecoded_paths.size () != 5) {
    return make_pair (status_codes::BadRequest, table_entity{});
  }

  const string tname {undecoded_paths[1]};
  const string token {undecoded_paths[2]};
  const string partition {undecoded_paths[3]};
  const string row {undecoded_paths[4]};

  try {
    uri endpoint_uri {endpoint};
    storage_credentials creds {token};
    cloud_table_client client {endpoint_uri, creds};

    table_operation op {table_operation::retrieve_entity(partition, row)};
    cloud_table table_cred {client.get_table_reference(tname)};
    table_result retrieve_result {table_cred.execute(op)};
    if (retrieve_result.http_status_code() == status_codes::NotFound) {
      cout << "Not found" << endl;
      return make_pair (status_codes::NotFound,
                         table_entity{});
    }

    table_entity entity {retrieve_result.entity()};
    return make_pair (status_codes::OK,
                       entity);
  }
  catch (const storage_exception& e) {
    cout << "Azure Table Storage error: " << e.what() << endl;
    cout << e.result().extended_error().message() << endl;
    if (e.result().http_status_code() == status_codes::Forbidden)
      return make_pair (status_codes::Forbidden,
                         table_entity{});
    else
      return make_pair (status_codes::InternalError,
                         table_entity{});
  }
}

/*
  Write to a table using a security token

  message is used only for its path, which must be split using the undecoded
    URI, as the token may have '/' characters encoded via %2F. After the
    undecoded path is split, the resulting parameters are decoded
    individually.
  endpoint is the URI endpoint for Azure tables. It takes the form
    "http://STORAGE.table.core.windows.net/", where STORAGE is
    replaced by the user's Azure Storage account name.
  props is an unordered_map of properties to be merged into
    the entity. This will typically be the result of get_json_body().

  Returns:  HTTP status code from the write.
 */
status_code update_with_token (const http_request& message,
                               const string& endpoint,
                               const unordered_map<string,string>& props) {
  
  /*
    Tokens can contain %2F ('/'). Thus we split the URI path
    *before* decoding and pass the undecoded values to Azure Storage
   */
  const string undecoded_path {message.relative_uri().path()};
  const vector<string> undecoded_paths {uri::split_path(undecoded_path)};
  if (undecoded_paths.size () != 5) {
    return status_codes::BadRequest;
  }
  
  const string tname {undecoded_paths[1]};
  const string token {undecoded_paths[2]};
  const string partition {undecoded_paths[3]};
  const string row {undecoded_paths[4]};
  table_entity entity {partition, row};
  try {
    uri endpoint_uri {endpoint};
    storage_credentials creds {token};
    cloud_table_client client {endpoint_uri, creds};

    table_entity::properties_type& properties = entity.properties();
    for (const auto v : props) {
      properties[v.first] = entity_property {v.second};
    }

    table_operation op {table_operation::merge_entity(entity)};
    cloud_table table_cred {client.get_table_reference(tname)};
    table_result update_result {table_cred.execute(op)};
    status_code status {static_cast<status_code> (update_result.http_status_code())};
    if (status == status_codes::NoContent || status == status_codes::OK)
      return status_codes::OK;
    else
      return status;
  }
  catch (const storage_exception& e)
  {
    cout << "Azure Table Storage error: " << e.what() << endl;
    cout << e.result().extended_error().message() << endl;
    if (e.result().http_status_code() == status_codes::Forbidden)
      return status_codes::Forbidden;
    else
      return status_codes::InternalError;
  }
}
