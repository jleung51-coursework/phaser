/*
  This C++ file contains unit tests for the push server.
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

#include "tester-utils.h"

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

using namespace rest_operations;

class PushFixture {
public:

  // User Entity 0  
  static constexpr const char* userid_0 {"user_0"};
  static constexpr const char* row_0 {"0,Ben"};

  static constexpr const char* friends_val_0 {"USA;1,Ben|USA;2,Ben"};
  static constexpr const char* status_val_0 {"A_status_update_which_is_47_characters_long_by_0"};
  static constexpr const char* status_update_0 {"A_status_update_which_is_52_characters_long_by_0"};

  // User Entity 1  
  static constexpr const char* userid_1 {"user_1"};
  static constexpr const char* row_1 {"1,Ben"};

  static constexpr const char* friends_val_1 {"USA;0,Ben|USA;2,Ben"};
  static constexpr const char* status_val_1 {"A_status_update_which_is_47_characters_long_by_1"};
  static constexpr const char* status_update_1 {"A_status_update_which_is_52_characters_long_by_1"};

  // User Entity 2  
  static constexpr const char* userid_2 {"user_2"};
  static constexpr const char* row_2 {"2,Ben"};

  static constexpr const char* friends_val_2 {"USA;0,Ben|USA;1,Ben"};
  static constexpr const char* status_val_2 {"A_status_update_which_is_47_characters_long_by_2"};
  static constexpr const char* status_update_2 {"A_status_update_which_is_52_characters_long_by_2"};
  
  // Constants for initializing tests
  // Represents a user's credentials
  static constexpr const char* addr {"http://localhost:34568/"};
  static constexpr const char* auth_addr {"http://localhost:34570/"};
  static constexpr const char* user_addr {"http://localhost:34572/"};
  static constexpr const char* push_addr {"http://localhost:34574/"};
  static constexpr const char* user_pwd {"user"};
  static constexpr const char* auth_table {"AuthTable"};
  static constexpr const char* auth_table_partition {"Userid"};
  static constexpr const char* auth_pwd_prop {"Password"};
  static constexpr const char* push_status_op {"PushStatus"};

  // Represents the user's entry including friends and status
  static constexpr const char* table {"DataTable"};
  static constexpr const char* partition {"USA"};
  static constexpr const char* friends {"Friends"};
  static constexpr const char* status {"Status"};
  static constexpr const char* updates {"Updates"};
  static constexpr const char* updates_val {""};



public:
  PushFixture() {

    // Create table DataTable and add a test entry
    {
      int make_result {create_table(addr, table)};
      cerr << "create result " << make_result << endl;
      if (make_result != status_codes::Created && make_result != status_codes::Accepted) {
        throw std::exception();
      }
      //Create entity 0 in DataTable
      int put_result {put_entity (
        addr,
        table,
        partition,
        row_0,
        vector<pair<string,value>>{make_pair(string(friends),value::string(friends_val_0)),
          make_pair(string(status),value::string(status_val_0)),
          make_pair(string(updates),value::string(updates_val))
        }
      )};
      cerr << "data table insertion result " << put_result << endl;
      if (put_result != status_codes::OK) {
        throw std::exception();
      }
      // Create entity 1 in DataTable
      put_result = put_entity (
        addr,
        table,
        partition,
        row_1,
        vector<pair<string,value>>{make_pair(string(friends),value::string(friends_val_1)),
          make_pair(string(status),value::string(status_val_1)),
          make_pair(string(updates),value::string(updates_val))
        }
      );
      cerr << "data table insertion result " << put_result << endl;
      if (put_result != status_codes::OK) {
        throw std::exception();
      }
      // Create entity 2 in DataTable
      put_result = put_entity (
        addr,
        table,
        partition,
        row_2,
        vector<pair<string,value>>{make_pair(string(friends),value::string(friends_val_2)),
          make_pair(string(status),value::string(status_val_2)),
          make_pair(string(updates),value::string(updates_val))
        }
      );
      cerr << "data table insertion result " << put_result << endl;
      if (put_result != status_codes::OK) {
        throw std::exception();
      }
    }




    // Create table AuthTable and add an entry 0 authenticating the test case
    // in DataTable
    {
      int make_result {create_table(addr, auth_table)};
      cerr << "create result " << make_result << endl;
      if (make_result != status_codes::Created && make_result != status_codes::Accepted) {
        throw std::exception();
      }

      vector<pair<string, value>> properties;
      properties.push_back( make_pair(string(auth_pwd_prop), value::string(user_pwd)) );
      properties.push_back( make_pair("DataPartition", value::string(PushFixture::partition)) );
      properties.push_back( make_pair("DataRow", value::string(PushFixture::row_0)) );

      assert(properties.size() == 3);

      int user_result {put_entity (
        addr,
        auth_table,
        auth_table_partition,
        userid_0,
        properties
      )};
      cerr << "auth table insertion result " << user_result << endl;
      if (user_result != status_codes::OK) {
        throw std::exception();
      }
    }
    // Create entry 1 authenticating test cases
    {
      vector<pair<string, value>> properties;
      properties.push_back( make_pair(string(auth_pwd_prop), value::string(user_pwd)) );
      properties.push_back( make_pair("DataPartition", value::string(PushFixture::partition)) );
      properties.push_back( make_pair("DataRow", value::string(PushFixture::row_1)) );

      assert(properties.size() == 3);

      int user_result {put_entity (
        addr,
        auth_table,
        auth_table_partition,
        userid_1,
        properties
      )};
      cerr << "auth table insertion result " << user_result << endl;
      if (user_result != status_codes::OK) {
        throw std::exception();
      }
    }
    // Create entry 2 authenticating test cases
    {
      vector<pair<string, value>> properties;
      properties.push_back( make_pair(string(auth_pwd_prop), value::string(user_pwd)) );
      properties.push_back( make_pair("DataPartition", value::string(PushFixture::partition)) );
      properties.push_back( make_pair("DataRow", value::string(PushFixture::row_1)) );

      assert(properties.size() == 3);

      int user_result {put_entity (
        addr,
        auth_table,
        auth_table_partition,
        userid_2,
        properties
      )};
      cerr << "auth table insertion result " << user_result << endl;
      if (user_result != status_codes::OK) {
        throw std::exception();
      }
    }

  }

  ~PushFixture() {

    // Delete entry 0 in DataTable
    {
      int del_ent_result {delete_entity (
        addr,
        table,
        partition,
        row_0
      )};
      cout << "delete datatable result " << del_ent_result << endl;
      if (del_ent_result != status_codes::OK) {
        throw std::exception();
      }
    }
    // Delete entry 1 in DataTable
    {
      int del_ent_result {delete_entity (
        addr,
        table,
        partition,
        row_1
      )};
      cout << "delete datatable result " << del_ent_result << endl;
      if (del_ent_result != status_codes::OK) {
        throw std::exception();
      }
    }
  // Delete entry 2 in DataTable
    {
      int del_ent_result {delete_entity (
        addr,
        table,
        partition,
        row_2
      )};
      cout << "delete datatable result " << del_ent_result << endl;
      if (del_ent_result != status_codes::OK) {
        throw std::exception();
      }
    }

    // Delete entry 0 in AuthTable
    {
      int del_ent_result {delete_entity (
        addr,
        auth_table,
        auth_table_partition,
        userid_0
      )};
      cout << "delete authtable result 0 " << del_ent_result << endl;
      if (del_ent_result != status_codes::OK) {
        throw std::exception();
      }
    }
    // Delete entry 1 in AuthTable
    {
      int del_ent_result {delete_entity (
        addr,
        auth_table,
        auth_table_partition,
        userid_1
      )};
      cout << "delete authtable result 1 " << del_ent_result << endl;
      if (del_ent_result != status_codes::OK) {
        throw std::exception();
      }
    }
    // Delete entry 2 in AuthTable
    {
      int del_ent_result {delete_entity (
        addr,
        auth_table,
        auth_table_partition,
        userid_2
      )};
      cout << "delete authtable result 2 " << del_ent_result << endl;
      if (del_ent_result != status_codes::OK) {
        throw std::exception();
      }
    }

  }
};

SUITE(PUSH_SERVER){

  TEST_FIXTURE(PushFixture, PushStatus) {
    pair<status_code,value> result;
    vector<pair<string, value>> friends_list;
    
    //returns OK

    /*//SEE OLD Status
    result = do_request( methods::GET, 
      string(PushFixture::addr) 
                  + "ReadEntityAdmin/" 
                  + PushFixture::table 
                  + PushFixture::partition 
                  + PushFixture::row_0);
    CHECK_EQUAL(status_codes::OK, result.first);

    string update_val { get_json_object_prop( result.second, "Updates")};
    cout << string( << endl;
*/
/*
    friends_list.push_back( make_pair(string(friends), value::string(friends_val_0) ) );
    result = do_request (methods::POST,
                  string(PushFixture::push_addr)
                  + PushFixture::push_status_op + "/"
                  //+ PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0 + "/"
                  + PushFixture::status_update_0,
                  value::object(friends_list) );
    CHECK_EQUAL(status_codes::OK, result.first);
    friends_list.clear();

    //get updated status -- should be different
    result = do_request( methods::GET, 
      string(PushFixture::addr) 
                  + "ReadEntityAdmin/" 
                  + PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0);
    CHECK_EQUAL(status_codes::OK, result.first);

    //string new_updates_val {};
*/
  
    //no Push Status -- Bad Request
    friends_list.push_back( make_pair(string(friends), value::string(friends_val_0) ) );
    result = do_request (methods::POST,
                  string(PushFixture::push_addr)                  
                  + "NotPushStatus" + "/"
                  //+ PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0 + "/"
                  + PushFixture::status_update_0,
                  value::object(friends_list) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    friends_list.clear();
  
    //no Friends property -- Bad Request
    friends_list.push_back( make_pair("NotFriends", value::string(friends_val_0) ) );
    result = do_request (methods::POST,
                  string(PushFixture::push_addr)
                  + PushFixture::push_status_op + "/"
                  //+ PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0 + "/"
                  + PushFixture::status_update_0,
                  value::object(friends_list) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    friends_list.clear();

    //too many properties -- Bad Request
    friends_list.push_back( make_pair(string(friends), value::string(friends_val_0) ) );
    friends_list.push_back( make_pair("TooMany", value::string(friends_val_0) ) );
    result = do_request (methods::POST,
                  string(PushFixture::push_addr)                  
                  + PushFixture::push_status_op + "/"
                  //+ PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0 + "/"
                  + PushFixture::status_update_0,
                  value::object(friends_list) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    friends_list.clear();

    //wrong number of parameters -- Bad Request
    friends_list.push_back( make_pair(string(friends), value::string(friends_val_0) ) );
    result = do_request (methods::POST,
                  string(PushFixture::push_addr)
                  + PushFixture::push_status_op + "/"
                  //+ PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  //+ PushFixture::row_0 + "/"
                  + PushFixture::status_update_0,
                  value::object(friends_list) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    friends_list.clear();

    //wrong number of parameters -- Bad Request
    friends_list.push_back( make_pair(string(friends), value::string(friends_val_0) ) );
    result = do_request (methods::POST,
                  string(PushFixture::push_addr)
                  + PushFixture::push_status_op + "/"
                  //+ PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0 + "/"
                  + PushFixture::row_0 + "/"
                  + PushFixture::status_update_0,
                  value::object(friends_list) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    friends_list.clear();

    //no friends body given -- Bad Request
    result = do_request (methods::POST,
                  string(PushFixture::push_addr)
                  + PushFixture::push_status_op + "/"
                  //+ PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0 + "/"
                  + PushFixture::status_update_0);
    CHECK_EQUAL(status_codes::BadRequest, result.first);


    friends_list.push_back( make_pair(string(friends), value::string(friends_val_0) ) );
    result = do_request (methods::POST,
                  string(PushFixture::push_addr)
                  + PushFixture::push_status_op + "/"
                  //+ PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0 + "/"
                  + PushFixture::status_update_0,
                  value::object(friends_list) );
    CHECK_EQUAL(status_codes::OK, result.first);
    friends_list.clear();

    //get updated status -- should be different
    result = do_request( methods::GET, 
      string(PushFixture::addr) 
                  + "ReadEntityAdmin/" 
                  + PushFixture::table + "/"
                  + PushFixture::partition + "/"
                  + PushFixture::row_0);
    CHECK_EQUAL(status_codes::OK, result.first);


  } // bracket for testfixture

} //bracket for suite(pushserver)