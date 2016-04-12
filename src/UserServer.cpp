/*
  User Server code for CMPT 276 Group Assignment, Spring 2016.

  This server manages a user’s social networking session.

  This server supports sign on/off, add friend, unfriend, update status,
  get user's friend list.

  This server handles disallowed method malformed request.

  As defined in ServerUrls.h, the URI for this server is
  http://localhost:34572.
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

#include "../include/ClientUtils.h"
#include "../include/ServerUrls.h"
#include "../include/ServerUtils.h"
#include "../include/TableCache.h"

#include "../include/azure_keys.h"


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

using web::http::client::http_client;
using web::http::http_headers;
using web::http::http_request;
using web::http::http_response;
using web::http::method;
using web::http::methods;
using web::http::status_code;
using web::http::status_codes;
using web::http::uri;

using web::json::value;

using web::http::experimental::listener::http_listener;

using prop_vals_t = vector<pair<string,value>>;

const string get_update_data_op {"GetUpdateData"};
const string update_entity_auth_op {"UpdateEntityAuth"};

const string sign_on {"SignOn"}; //POST
const string sign_off {"SignOff"}; //POST

const string add_friend {"AddFriend"}; // PUT
const string unfriend {"UnFriend"}; //PUT
const string update_status {"UpdateStatus"}; //PUT

const string get_friend_list {"ReadFriendList"}; //GET

// Cache of active sessions
std::unordered_map< string, std::tuple<string, string, string> > sessions;

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

void handle_post (http_request message){
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** POST " << path << endl;
  auto paths = uri::split_path(path);

  // Operation name and user ID
  if(paths.size() < 2) {
    message.reply(status_codes::BadRequest);
    return;
  }
  else if(paths[0] != sign_on && paths[0] != sign_off) {
    message.reply(status_codes::BadRequest);
  }

  const string operation = paths[0];
  const string userid = paths[1];

  if(operation == sign_on) {
    if(!has_json_body(message)) {
      message.reply(status_codes::BadRequest);
      return;
    }

    unordered_map<string, string> json_body {get_json_bourne(message)};
    if(json_body.size() != 1) {
      message.reply(status_codes::BadRequest);
      return;
    }

    unordered_map<string, string>::const_iterator json_body_password_iterator {json_body.find("Password")};
    // No 'Password' property
    if(json_body_password_iterator == json_body.end()) {
      message.reply(status_codes::BadRequest);
      return;
    }

    vector<pair<string, value>> json_pw;
    json_pw.push_back(make_pair(
        json_body_password_iterator->first,
        value::string(json_body_password_iterator->second)
    ));

    pair<status_code, value> result;
    result = do_request(
      methods::GET,
      string(server_urls::auth_server) + "/" +
      get_update_data_op + "/" +
      userid,
      value::object(json_pw)
    );
    if(result.first != status_codes::OK) {
      message.reply(result.first);
      return;
    }
    else if(result.second.size() != 3) {
      message.reply(status_codes::InternalError);
      return;
    }

    const string token = get_json_object_prop(
      result.second,
      "token"
    );
    const string data_partition = get_json_object_prop(
      result.second,
      "DataPartition"
    );
    const string data_row = get_json_object_prop(
      result.second,
      "DataRow"
    );
    if(token.empty() ||
       data_partition.empty() ||
       data_row.empty() ) {
      message.reply(status_codes::InternalError);
      return;
    }

    std::tuple<string, string, string> tuple_insert(
      token,
      data_partition,
      data_row);
    std::pair<string, std::tuple<string, string, string>> pair_insert(
      userid,
      tuple_insert
    );
    sessions.insert(pair_insert);

    message.reply(status_codes::OK);
    return;
  }

  else if(operation == sign_off) {
    auto session = sessions.find(userid);
    if(session == sessions.end()) {
      message.reply(status_codes::NotFound);
      return;
    }

    sessions.erase(session);
    message.reply(status_codes::OK);
    return;
  }

  else {
    message.reply(status_codes::InternalError);
    return;
  }
}

void handle_put (http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** POST " << path << endl;
  auto paths = uri::split_path(path);

  if(true/*basic criteria*/){}
  else if (paths[0] == add_friend) {}
  else if (paths[0] == unfriend) {}
  else if (paths[0] == update_status) {}
  else {
    // malformed request
    vector<value> vec;
    message.reply(status_codes::BadRequest, value::array(vec));
    return;
  }
}

void handle_get (http_request message) {
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** POST " << path << endl;
  auto paths = uri::split_path(path);

  if(true/*basic criteria*/){}
  else if (paths[0] == get_friend_list) {}
  else {
    // malformed request
    vector<value> vec;
    message.reply(status_codes::BadRequest, value::array(vec));
    return;
  }
}


int main (int argc, char const * argv[]) {
  cout << "Opening listener" << endl;
  http_listener listener {server_urls::user_server};
  listener.support(methods::GET, &handle_get); // Get user's friend list
  listener.support(methods::POST, &handle_post); // SignOn, SignOff
  listener.support(methods::PUT, &handle_put); // Add friend, Unfriend, Update Status
  /*TO DO: Disallowed method*/
  listener.open().wait(); // Wait for listener to complete starting

  cout << "Enter carriage return to stop server." << endl;
  string line;
  getline(std::cin, line);

  // Shut it down
  listener.close().wait();
  cout << "Closed" << endl;
}
