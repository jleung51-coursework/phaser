/*
  This C++ file contains unit tests for the user server.
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

class UserFixture {
public:

	// Constants for initializing tests
	// Represents a user's credentials
	static constexpr const char* basic_addr {"http://localhost:34568/"};
	static constexpr const char* auth_addr {"http://localhost:34570/"};
	static constexpr const char* user_addr {"http://localhost:34572/"};
	static constexpr const char* userid {"user"};
	static constexpr const char* user_pwd {"user"};
	static constexpr const char* auth_table {"AuthTable"};
	static constexpr const char* auth_table_partition {"Userid"};
	static constexpr const char* auth_pwd_prop {"Password"};

	// Represents the user's entry including friends and status
	static constexpr const char* table {"DataTable"};
	static constexpr const char* partition {"USA"};
	static constexpr const char* row {"Franklin,Aretha"};
	static constexpr const char* friends {"Friends"};
	static constexpr const char* friends_val {"USA;Shinoda,Mike|Canada;Edwards,Kathleen|Korea;Bae,Doona"};
	static constexpr const char* status {"Status"};
	static constexpr const char* status_val {"A status update which is 43 characters long"};
	static constexpr const char* updates {"Updates"};
	static constexpr const char* updates_val {""};

	// Constants for use in tests
	static constexpr const char* status_update {"A different status"};
	static constexpr const char* friend_country {"UK"};
	static constexpr const char* friend_name {"Scott,Tom"};

public:
  UserFixture() {

    // Create table DataTable and add a test entry
    {
      int make_result {create_table(basic_addr, table)};
      cerr << "create result " << make_result << endl;
      if (make_result != status_codes::Created && make_result != status_codes::Accepted) {
        throw std::exception();
      }
      int put_result {put_entity (
        basic_addr,
        table,
        partition,
        row,
        vector<pair<string,value>>{make_pair(string(friends),value::string(friends_val)),
        	make_pair(string(status),value::string(status_val)),
        	make_pair(string(updates),value::string(updates_val))
        }
      )};
      cerr << "data table insertion result " << put_result << endl;
      if (put_result != status_codes::OK) {
        throw std::exception();
      }
    }

    // Create table AuthTable and add an entry authenticating the test case
    // in DataTable
    {
      int make_result {create_table(basic_addr, auth_table)};
      cerr << "create result " << make_result << endl;
      if (make_result != status_codes::Created && make_result != status_codes::Accepted) {
        throw std::exception();
      }

      vector<pair<string, value>> properties;
      properties.push_back( make_pair(string(auth_pwd_prop), value::string(user_pwd)) );
      properties.push_back( make_pair("DataPartition", value::string(UserFixture::partition)) );
      properties.push_back( make_pair("DataRow", value::string(UserFixture::row)) );

      assert(properties.size() == 3);

      int user_result {put_entity (
        basic_addr,
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

  ~UserFixture() {

    // Delete entry in DataTable
    {
      int del_ent_result {delete_entity (
        basic_addr,
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
        basic_addr,
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

SUITE(USERSERVER_POST) {
	TEST_FIXTURE(UserFixture, Proper) {
		vector<pair<string, value>> password_json;
		pair<status_code, value> result;

		// Proper request
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string(user_pwd)
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			sign_on + "/" +
			UserFixture::userid,
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::OK, result.first);
		password_json.clear();

		// 1 extra parameter
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string(user_pwd)
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			sign_on + "/" +
			UserFixture::userid + "/" +
			"excess_parameter",
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::OK, result.first);
		password_json.clear();
	}

	TEST_FIXTURE(UserFixture, SignOff_ExtraJSON) {
	}

	TEST_FIXTURE(UserFixture, SignOn_BadRequest) {
		vector<pair<string, value>> password_json;
		pair<status_code, value> result;

		// No operation name
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string(user_pwd)
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			UserFixture::userid,
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::BadRequest, result.first);
		password_json.clear();

		// Incorrect operation name
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string(user_pwd)
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			"IncorrectOperation" + "/" +
			UserFixture::userid,
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::BadRequest, result.first);
		password_json.clear();

		// No userid
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string(user_pwd)
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			sign_on,
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::BadRequest, result.first);
		password_json.clear();

		// No operation name or userid
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string(user_pwd)
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr),
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::BadRequest, result.first);
		password_json.clear();

		// No JSON body (size = 0)
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			sign_on + "/" +
			UserFixture::userid
		);
		CHECK_EQUAL(status_codes::BadRequest, result.first);
		password_json.clear();

		// JSON body has too many objects (size > 1)
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string(user_pwd)
		));
		password_json.push_back( make_pair(
			"Username",
			value::string("username")
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			sign_on + "/" +
			UserFixture::userid,
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::BadRequest, result.first);
		password_json.clear();

		// JSON body name is incorrect
		password_json.push_back( make_pair(
			string("pASSWORD"),
			value::string(user_pwd)
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			sign_on + "/" +
			UserFixture::userid,
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::BadRequest, result.first);
		password_json.clear();
	}

	TEST_FIXTURE(UserFixture, SignOff_BadRequest) {
	}

	TEST_FIXTURE(UserFixture, SignOn_IncorrectParameters) {
		vector<pair<string, value>> password_json;
		pair<status_code, value> result;

		// Userid is incorrect
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string(user_pwd)
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			sign_on + "/" +
			"incorrect_userid",
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::NotFound, result.first);
		password_json.clear();

		// Password is incorrect
		password_json.push_back( make_pair(
			string(auth_pwd_prop),
			value::string("incorrect_password")
		));
		result = do_request(
			methods::POST,
			string(UserFixture::user_addr) +
			sign_on + "/" +
			UserFixture::userid,
			value::object(password_json)
		);
		CHECK_EQUAL(status_codes::NotFound, result.first);
		password_json.clear();
	}

	TEST_FIXTURE(UserFixture, SignOff_IncorrectParameters) {
	}

	TEST_FIXTURE(UserFixture, SignOffTwice) {
	}
}
