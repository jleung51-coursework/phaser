/*
  Push Server code for CMPT 276 Group Assignment, Spring 2016.

  This server pushes status updates to all a user’s friends in the network.

  This server supports a single operation: push a status update to all friends
  of this user.

  This server handles disallowed method malformed request.
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

//#include "../include/azure_keys.h"

#include "../include/ServerUtils.h"
#include "../include/ClientUtils.h"

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

//using prop_vals_t = vector<pair<string,value>>;
using friends_list_t = std::vector<std::pair<std::string,std::string>>;

constexpr const char* def_url = "http://localhost:34574";

const string push_status_op {"PushStatus"};
const string read_entity_admin {"ReadEntityAdmin"};
const string update_entity_admin {"UpdateEntityAdmin"};

const string data_table_name {"DataTable"};

//TableCache table_cache {};


//---------------------------------------------------------------------------------------

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


//---------------------------------------------------------------------------------------

void handle_post (http_request message) {
  //TODO Not implemented yet!
  //message.reply(status_codes::NotImplemented);
  
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** POST " << path << endl;
  auto paths = uri::split_path(path);

  // need operation name, country, username, status = 4
  if (paths.size() != 4 ) {
    message.reply(status_codes::BadRequest);
    return;
  }
  //no PushStatus included
  else if(paths[0] != push_status_op) {
    message.reply(status_codes::BadRequest);
    return;
  }
  // no friends body included
  else if(!has_json_body(message)) {
    message.reply(status_codes::BadRequest);
    return;
  }

  unordered_map<string, string> json_body {get_json_bourne(message)};
  unordered_map<string, string>::const_iterator json_body_friends_iterator
    {json_body.find("Friends")};
  //no properties or more than one property
  if(json_body.size() != 1) {
    message.reply(status_codes::BadRequest);
    return;
  }
  // No 'Friends' property
  else if(json_body_friends_iterator == json_body.end()) {
    message.reply(status_codes::BadRequest);
    return;
  }

  string friends_list_unparsed {json_body_friends_iterator->second};
  friends_list_t friends_list { parse_friends_list(friends_list_unparsed) };
  string old_updates;
  string new_updates;
  pair<status_code,value> result;
  vector<pair<string,value>> update_property;
  
  for ( int i = 0; i < friends_list.size(); i++ ){
    // get properties of entity
    result = do_request(methods::GET, def_url 
      + data_table_name + "/" 
      + read_entity_admin + "/" 
      + string(friends_list[i].first) + "/" 
      + string(friends_list[i].second) );
    //CHECK_EQUAL(status_codes::OK, result.first);
    //get old updates
    old_updates = get_json_object_prop(result.second, "Updates");
    if (old_updates == ""){
      message.reply(status_codes::InternalError);
        return;
    }
    cout << "Old Statuses: " << old_updates << endl;
    //add new update
    new_updates = string(paths[3]) + old_updates;
    update_property.push_back( make_pair("Updates", value::string(new_updates) ) );
    //update property of friend
    result = do_request(methods::PUT, def_url 
      + data_table_name + "/" 
      + update_entity_admin + "/" 
      + string(friends_list[i].first) + "/" 
      + string(friends_list[i].second), value::object(update_property) );
    //CHECK_EQUAL(status_codes::OK, result.first);
    update_property.clear();
    cout << "Updated Statuses: " << new_updates << endl;
  }
  
  message.reply(status_codes::OK);
  return;
}

int main (int argc, char const * argv[]) {
  cout << "Parsing connection string" << endl;
  //table_cache.init (storage_connection_string);

  cout << "Opening listener" << endl;
  http_listener listener {def_url};
  listener.support(methods::POST, &handle_post); // Push a status update to friends
  listener.open().wait();

  cout << "Enter carriage return to stop server." << endl;
  string line;
  getline(std::cin, line);

  // Shut it down
  listener.close().wait();
  cout << "Closed" << endl;
}
