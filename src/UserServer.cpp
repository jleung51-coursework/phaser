/*
  User Server code for CMPT 276 Group Assignment, Spring 2016.

  This server manages a user’s social networking session.

  This server supports sign on/off, add friend, unfriend, update status,
  get user's friend list.

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

constexpr const char* def_url = "http://localhost:34572";

const string sign_on {"SignOn"}; //POST
const string sign_off {"SignOff"}; //POST

const string add_friend {"AddFriend"}; // PUT
const string unfriend {"UnFriend"}; //PUT
const string update_status {"UpdateStatus"}; //PUT

const string get_friend_list {"ReadFriendList"}; //GET

// Cache of opened tables
TableCache table_cache {};

void handle_post (http_request message){
  string path {uri::decode(message.relative_uri().path())};
  cout << endl << "**** POST " << path << endl;
  auto paths = uri::split_path(path);

  if(true/*basic criteria*/){}
  else if (paths[0] == sign_on) {}
  else if (paths[0] == sign_off) {}
  else {
    // malformed request
    vector<value> vec;
    message.reply(status_codes::BadRequest, value::array(vec));
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
  cout << "Parsing connection string" << endl;
  table_cache.init (storage_connection_string);

  cout << "Opening listener" << endl;
  http_listener listener {def_url};
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
