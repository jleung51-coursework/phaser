/*
  This C++ file contains unit tests for the authentication server.
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

class AuthFixture {
public:
  static constexpr const char* addr {"http://localhost:34568/"};
  static constexpr const char* auth_addr {"http://localhost:34570/"};
  static constexpr const char* userid {"user"};
  static constexpr const char* user_pwd {"user"};
  static constexpr const char* auth_table {"AuthTable"};
  static constexpr const char* auth_table_partition {"Userid"};
  static constexpr const char* auth_pwd_prop {"Password"};

  static constexpr const char* table {"DataTable"};
  static constexpr const char* partition {"USA"};
  static constexpr const char* row {"Franklin,Aretha"};
  static constexpr const char* property {"Song"};
  static constexpr const char* prop_val {"RESPECT"};

public:
  AuthFixture() {

    // Create table DataTable and add a test entry
    {
      int make_result {create_table(addr, table)};
      cerr << "create result " << make_result << endl;
      if (make_result != status_codes::Created && make_result != status_codes::Accepted) {
        throw std::exception();
      }
      int put_result {put_entity (
        addr,
        table,
        partition,
        row,
        property,
        prop_val
      )};
      cerr << "data table insertion result " << put_result << endl;
      if (put_result != status_codes::OK) {
        throw std::exception();
      }
    }

    // Create table AuthTable and add an entry authenticating the test case
    // in DataTable
    {
      int make_result {create_table(addr, auth_table)};
      cerr << "create result " << make_result << endl;
      if (make_result != status_codes::Created && make_result != status_codes::Accepted) {
        throw std::exception();
      }

      vector<pair<string, value>> properties;
      properties.push_back( make_pair(string(auth_pwd_prop), value::string(user_pwd)) );
      properties.push_back( make_pair("DataPartition", value::string(AuthFixture::partition)) );
      properties.push_back( make_pair("DataRow", value::string(AuthFixture::row)) );

      assert(properties.size() == 3);

      int user_result {put_entity (
        addr,
        auth_table,
        auth_table_partition,
        userid,
        properties
      )};
      cerr << "auth table insertion result " << user_result << endl;
      if (user_result != status_codes::OK) {
        throw std::exception();
      }
    }

  }

  ~AuthFixture() {

    // Delete entry in DataTable
    {
      int del_ent_result {delete_entity (
        addr,
        table,
        partition,
        row
      )};
      cout << "delete datatable result " << del_ent_result << endl;
      if (del_ent_result != status_codes::OK) {
        throw std::exception();
      }
    }

    // Delete entry in AuthTable
    {
      int del_ent_result {delete_entity (
        addr,
        auth_table,
        auth_table_partition,
        userid
      )};
      cout << "delete authtable result " << del_ent_result << endl;
      if (del_ent_result != status_codes::OK) {
        throw std::exception();
      }
    }

  }
};

SUITE(UPDATE_AUTH) {
  TEST_FIXTURE(AuthFixture, PutAuth) {
    pair<string,string> added_prop {make_pair(string("born"),string("1942"))};

    cout << "Requesting token" << endl;
    pair<status_code,string> token_res {
      get_update_token(AuthFixture::auth_addr,
                       AuthFixture::userid,
                       AuthFixture::user_pwd)};
    cout << "Token response " << token_res.first << endl;
    CHECK_EQUAL (token_res.first, status_codes::OK);

    pair<status_code,value> result {
      do_request (methods::PUT,
                  string(AuthFixture::addr)
                  + update_entity_auth + "/"
                  + AuthFixture::table + "/"
                  + token_res.second + "/"
                  + AuthFixture::partition + "/"
                  + AuthFixture::row,
                  value::object (vector<pair<string,value>>
                                   {make_pair(added_prop.first,
                                              value::string(added_prop.second))})
                  )};
    CHECK_EQUAL(status_codes::OK, result.first);

    pair<status_code,value> ret_res {
      do_request (methods::GET,
                  string(AuthFixture::addr)
                  + read_entity_admin + "/"
                  + AuthFixture::table + "/"
                  + AuthFixture::partition + "/"
                  + AuthFixture::row)};
    CHECK_EQUAL (status_codes::OK, ret_res.first);
    value expect {
      build_json_object (
                         vector<pair<string,string>> {
                           added_prop,
                           make_pair(string(AuthFixture::property),
                                     string(AuthFixture::prop_val))}
                         )};

    cout << AuthFixture::property << endl;
    compare_json_values (expect, ret_res.second);
  }
}

SUITE(GET_READ_TOKEN){
  TEST_FIXTURE(AuthFixture, GetReadToken){
    pair<status_code, value> result;
    vector<pair<string,value>> passwordbody;

    //correct everything
    passwordbody.push_back( make_pair( string(AuthFixture::auth_pwd_prop), value::string(user_pwd) ) );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + get_read_token_op + "/"
      + string(AuthFixture::userid),
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::OK, result.first);
    passwordbody.clear();

    //no get read token
    passwordbody.push_back( make_pair( string(AuthFixture::auth_pwd_prop), value::string(user_pwd) ) );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + string(AuthFixture::userid),
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    passwordbody.clear();

    //wrong property
    passwordbody.push_back(  make_pair( "NotPassword", value::string(user_pwd) )  );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + get_read_token_op + "/"
      + string(AuthFixture::userid),
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    passwordbody.clear();

    //userid not found
    passwordbody.push_back(  make_pair( string(AuthFixture::auth_pwd_prop), value::string(user_pwd) )  );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + get_read_token_op + "/"
      + "WrongUser",
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    //password wrong
    passwordbody.push_back(  make_pair( string(AuthFixture::auth_pwd_prop), value::string("WrongPassword") )  );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + get_read_token_op + "/"
      + string(AuthFixture::userid),
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    //userid missing
    passwordbody.push_back( make_pair( string(AuthFixture::auth_pwd_prop), value::string(user_pwd) ) );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + get_read_token_op + "/"
      + "/",
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    passwordbody.clear();

    //empty password
    passwordbody.push_back( make_pair( string(AuthFixture::auth_pwd_prop), value::string("") ) );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + get_read_token_op + "/"
      + string(AuthFixture::userid),
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    passwordbody.clear();

    //extra properties
    passwordbody.push_back( make_pair( "NotAProperty", value::string("NotAPassword") ) );
    passwordbody.push_back( make_pair( string(AuthFixture::auth_pwd_prop), value::string(user_pwd) ) );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + get_read_token_op + "/"
      + string(AuthFixture::userid),
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    passwordbody.clear();

  }
}


SUITE(GET_UPDATE_TOKEN){
  TEST_FIXTURE(AuthFixture, GetUpdateToken){
    pair<status_code, value> result;
    vector<pair<string,value>> passwordbody;

   //correct everything
    passwordbody.push_back( make_pair( string(AuthFixture::auth_pwd_prop), value::string(user_pwd) ) );
    result = do_request( methods::GET,
      string(AuthFixture::auth_addr)
      + get_update_token_op + "/"
      + string(AuthFixture::userid),
      value::object(passwordbody) );
    CHECK_EQUAL(status_codes::OK, result.first);
    passwordbody.clear();

  }

}
