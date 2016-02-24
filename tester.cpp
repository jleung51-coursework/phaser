/*
  Sample unit tests for BasicServer
 */

#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <cpprest/http_client.h>
#include <cpprest/json.h>

#include <pplx/pplxtasks.h>

#include <UnitTest++/UnitTest++.h>

using std::cerr;
using std::cout;
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

using web::json::value;

/*
  Make an HTTP request, returning the status code and any JSON value in the body

  method: member of web::http::methods
  uri_string: uri of the request
  req_body: [optional] a json::value to be passed as the message body

  If the response has a body with Content-Type: application/json,
  the second part of the result is the json::value of the body.
  If the response does not have that Content-Type, the second part
  of the result is simply json::value {}.

  You're welcome to read this code but bear in mind: It's the single
  trickiest part of the sample code. You can just call it without
  attending to its internals, if you prefer.
 */

// Version with explicit third argument
pair<status_code,value> do_request (const method& http_method, const string& uri_string, const value& req_body) {
  http_request request {http_method};
  if (req_body != value {}) {
    http_headers& headers (request.headers());
    headers.add("Content-Type", "application/json");
    request.set_body(req_body);
  }

  status_code code;
  value resp_body;
  http_client client {uri_string};
  client.request (request)
    .then([&code](http_response response)
	  {
	    code = response.status_code();
	    const http_headers& headers {response.headers()};
	    auto content_type (headers.find("Content-Type"));
	    if (content_type == headers.end() ||
		content_type->second != "application/json")
	      return pplx::task<value> ([] { return value {};});
	    else
	      return response.extract_json();
	  })
    .then([&resp_body](value v) -> void
	  {
	    resp_body = v;
	    return;
	  })
    .wait();
  return make_pair(code, resp_body);
}

// Version that defaults third argument
pair<status_code,value> do_request (const method& http_method, const string& uri_string) {
  return do_request (http_method, uri_string, value {});
}

/*
  Utility to create a table

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
 */
int create_table (const string& addr, const string& table) {
  pair<status_code,value> result {do_request (methods::POST, addr + "CreateTable/" + table)};
  return result.first;
}

/*
  Utility to delete a table

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
 */
int delete_table (const string& addr, const string& table) {
  // SIGH--Note that REST SDK uses "methods::DEL", not "methods::DELETE"
  pair<status_code,value> result {
    do_request (methods::DEL,
		addr + "DeleteTable/" + table)};
  return result.first;
}

/*
  Utility to put an entity with a single property

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
  partition: Partition of the entity 
  row: Row of the entity
  prop: Name of the property
  pstring: Value of the property, as a string
 */
int put_entity(const string& addr, const string& table, const string& partition, const string& row, const string& prop, const string& pstring) {
  pair<status_code,value> result {
    do_request (methods::PUT,
		addr + "UpdateEntity/" + table + "/" + partition + "/" + row,
		value::object (vector<pair<string,value>>
			       {make_pair(prop, value::string(pstring))}))};
  return result.first;
}

/*
  Utility to delete an entity

  addr: Prefix of the URI (protocol, address, and port)
  table: Table in which to insert the entity
  partition: Partition of the entity 
  row: Row of the entity
 */
int delete_entity (const string& addr, const string& table, const string& partition, const string& row)  {
  // SIGH--Note that REST SDK uses "methods::DEL", not "methods::DELETE"
  pair<status_code,value> result {
    do_request (methods::DEL,
		addr + "DeleteEntity/" + table + "/" + partition + "/" + row)};
  return result.first;
}

/*
  A sample fixture that ensures TestTable exists, and
  at least has the entity Franklin,Aretha/USA
  with the property "Song": "RESPECT".

  The entity is deleted when the fixture shuts down
  but the table is left. See the comments in the code
  for the reason for this design.
 */
SUITE(GET) {
  class GetFixture {
  public:
    static constexpr const char* addr {"http://127.0.0.1:34568/"};
    static constexpr const char* table {"TestTable"};
    static constexpr const char* partition {"Franklin,Aretha"};
    static constexpr const char* row {"USA"};
    static constexpr const char* property {"Song"};
    static constexpr const char* prop_val {"RESPECT"};

  public:
    GetFixture() {
      int make_result {create_table(addr, table)};
      cerr << "create result " << make_result << endl;
      if (make_result != status_codes::Created && make_result != status_codes::Accepted) {
	throw std::exception();
      }
      int put_result {put_entity (addr, table, partition, row, property, prop_val)};
      cerr << "put result " << put_result << endl;
      if (put_result != status_codes::OK) {
	throw std::exception();
      }
    }
    ~GetFixture() {
      int del_ent_result {delete_entity (addr, table, partition, row)};
      if (del_ent_result != status_codes::OK) {
	throw std::exception();
      }

      /*
	In traditional unit testing, we might delete the table after every test.

	However, in cloud NoSQL environments (Azure Tables, Amazon DynamoDB)
	creating and deleting tables are rate-limited operations. So we
	leave the table after each test but delete all its entities.
       */
      cout << "Skipping table delete" << endl;
      /*
      int del_result {delete_table(addr, table)};
      cerr << "delete result " << del_result << endl;
      if (del_result != status_codes::OK) {
	throw std::exception();
      }
      */
    }
  };

  /*
    A test of GET of a single entity
   */
  TEST_FIXTURE(GetFixture, GetSingle) {
    pair<status_code,value> result {
      do_request (methods::GET,
		  string(GetFixture::addr)
		  + GetFixture::table + "/"
		  + GetFixture::partition + "/"
		  + GetFixture::row)};
      
      CHECK_EQUAL(string("{\"")
		  + GetFixture::property
		  + "\":\""
		  + GetFixture::prop_val
		  + "\"}",
		  result.second.serialize());
      CHECK_EQUAL(status_codes::OK, result.first);
    }

  /*
    A test of GET all table entries
   */
  TEST_FIXTURE(GetFixture, GetAll) {
    string partition {"Katherines,The"};
    string row {"Canada"};
    string property {"Home"};
    string prop_val {"Vancouver"};
    int put_result {put_entity (GetFixture::addr, GetFixture::table, partition, row, property, prop_val)};
    cerr << "put result " << put_result << endl;
    assert (put_result == status_codes::OK);

    pair<status_code,value> result {
      do_request (methods::GET,
		  string(GetFixture::addr)
		  + string(GetFixture::table))};
    CHECK(result.second.is_array());
    CHECK_EQUAL(2, result.second.as_array().size());
    /*
      Checking the body is not well-supported by UnitTest++, as we have to test
      independent of the order of returned values.
     */
    //CHECK_EQUAL(body.serialize(), string("{\"")+string(GetFixture::property)+ "\":\""+string(GetFixture::prop_val)+"\"}");
    CHECK_EQUAL(status_codes::OK, result.first);
    CHECK_EQUAL(status_codes::OK, delete_entity (GetFixture::addr, GetFixture::table, partition, row));
  }
}

/*
  Locate and run all tests
 */
int main(int argc, const char* argv[]) {
  return UnitTest::RunAllTests();
}
