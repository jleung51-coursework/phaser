/*
  This C++ file contains unit tests for the basic server.
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

using hissy_fit = std::exception;

using namespace rest_operations;

/*
  A sample fixture that ensures TestTable exists, and
  at least has the entity Franklin,Aretha/USA
  with the property "Song": "RESPECT".

  The entity is deleted when the fixture shuts down
  but the table is left. See the comments in the code
  for the reason for this design.
 */
class BasicFixture {
public:
  static constexpr const char* addr {"http://localhost:34568/"};
  static constexpr const char* table {"TestTable"};
  static constexpr const char* partition {"USA"};
  static constexpr const char* row {"Franklin,Aretha"};
  static constexpr const char* property {"Song"};
  static constexpr const char* prop_val {"RESPECT"};

public:
  BasicFixture() {
    int make_result {create_table(addr, table)};
    cerr << "create result " << make_result << endl;
    if (make_result != status_codes::Created && make_result != status_codes::Accepted) {
      throw hissy_fit();
    }
    int put_result {put_entity (addr, table, partition, row, property, prop_val)};
    cerr << "put result " << put_result << endl;
    if (put_result != status_codes::OK) {
      throw hissy_fit();
    }
  }

  ~BasicFixture() {
    int del_ent_result {delete_entity (addr, table, partition, row)};
    if (del_ent_result != status_codes::OK) {
      throw hissy_fit();
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
        throw hissy_fit();
      }
      */
  }
};

SUITE(GET) {
  /*
    GET test of a single specified entity.

    URI: http://localhost:34568/TableName/PartitionName/RowName

    The Row name cannot be "*".
   */
  TEST_FIXTURE(BasicFixture, GetSingle) {

    // Proper request
    pair<status_code,value> result {
      do_request(
        methods::GET,
        string(BasicFixture::addr)
        + read_entity_admin + "/"
        + BasicFixture::table + "/"
        + BasicFixture::partition + "/"
        + BasicFixture::row
      )
    };
    CHECK_EQUAL(status_codes::OK, result.first);
    CHECK_EQUAL(
      string("{\"")
      + BasicFixture::property
      + "\":\""
      + BasicFixture::prop_val
      + "\"}",
      result.second.serialize()
    );

    // Incorrect table name
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + "NonexistentTable/"
      + BasicFixture::partition + "/"
      + BasicFixture::row
    );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    // Incorrect partition name
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + BasicFixture::table + "/"
      + "NonexistentPartition/"
      + BasicFixture::row
    );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    // Incorrect row name
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + BasicFixture::table + "/"
      + BasicFixture::partition + "/"
      + "NonexistentRow"
    );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    //TODO
    // The following commented tests currently induce a segmentation fault
    // due to the lack of checking for empty paths in handle_get()
    /*
    // Missing table name and slash
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + BasicFixture::partition + "/"
      + BasicFixture::row
    );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    // Missing partition name and slash
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + BasicFixture::table + "/"
      + BasicFixture::row
    );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    // Missing row name and slash
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + BasicFixture::table + "/"
      + BasicFixture::partition
    );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    // Empty table name
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + "/"
      + BasicFixture::partition + "/"
      + BasicFixture::row
    );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    // Empty partition name
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + BasicFixture::table + "/"
      + "/"
      + BasicFixture::row
    );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    // Empty row name
    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + BasicFixture::table + "/"
      + BasicFixture::partition + "/"
    );
    CHECK_EQUAL(status_codes::BadRequest, result.first);
    */
  }

  /*
    GET test of entities which have all the properties listed in the
    JSON object, regardless of their values.

    URI: http://localhost:34568/TableName
    Body (JSON object): A name denotes a property; a value is the string "*".
      E.g. {"born":"*","name":"*"} where "born" and "name" are properties.
   */
  TEST_FIXTURE(BasicFixture, GetProperties) {

    // Add additional entity p2
    string p2_partition {"Katherines,The"};
    string p2_row {"Canada"};
    string p2_property {"Home"};
    string p2_prop_val {"Vancouver"};
    int put_result {
      put_entity (
        BasicFixture::addr, BasicFixture::table,
        p2_partition, p2_row,
        p2_property, p2_prop_val
      )
    };
    cerr << "put result " << put_result << endl;
    assert (put_result == status_codes::OK);

    // Add additional entity p3
    string p3_partition {"Person"};
    string p3_row {"Country"};
    string p3_property {"City"};
    string p3_prop_val {"CityName"};
    put_result = put_entity (
      BasicFixture::addr, BasicFixture::table,
      p3_partition, p3_row,
      p3_property, p3_prop_val
    );
    cerr << "put result " << put_result << endl;
    if (put_result != status_codes::OK) {
      delete_entity (BasicFixture::addr, BasicFixture::table, p2_partition, p2_row);
      assert (put_result == status_codes::OK);  // Exit program; show error message
    }

    // Add property to match
    put_result = put_entity (
      BasicFixture::addr, BasicFixture::table,
      p3_partition, p3_row,
      "Home", "Vancouver"
    );
    cerr << "put result " << put_result << endl;
    if (put_result != status_codes::OK) {
      delete_entity (BasicFixture::addr, BasicFixture::table, p2_partition, p2_row);
      delete_entity (BasicFixture::addr, BasicFixture::table, p3_partition, p3_row);
      // Exit program; show error message
      assert (put_result == status_codes::OK);
    }

    // Proper request
    // Uses 1 property to request the 1 entity:
    //   Person/Country, with properties "City:CityName, Home:Vancouver"
    vector<pair<string, value>> desired_properties;
    desired_properties.push_back( make_pair("City", value::string("*") ));

    pair<status_code,value> result {
      do_request(
        methods::GET,
        string(BasicFixture::addr)
        + read_entity_admin + "/"
        + BasicFixture::table,
        value::object(desired_properties)
      )
    };
    CHECK_EQUAL(status_codes::OK, result.first);
    CHECK(result.second.is_array());
    CHECK_EQUAL(1, result.second.as_array().size());

    // Proper request
    // Uses 2 properties to request the 1 entity:
    //   Person/Country, with properties "City:CityName, Home:Vancouver"
    desired_properties.clear();
    desired_properties.push_back( make_pair("City", value::string("*") ));
    desired_properties.push_back( make_pair("Home", value::string("*") ));

    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + BasicFixture::table,
      value::object(desired_properties)
    );
    CHECK_EQUAL(status_codes::OK, result.first);
    CHECK(result.second.is_array());
    CHECK_EQUAL(1, result.second.as_array().size());

    // Proper request
    // Uses 1 property to request the 2 entities:
    //   Katherines,The/Canada, with properties "Home:Vancouver"
    //   Person/Country, with properties "City:CityName, Home:Vancouver"
    desired_properties.clear();
    desired_properties.push_back( make_pair("Home", value::string("*") ));

    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + BasicFixture::table,
      value::object(desired_properties)
    );
    CHECK_EQUAL(status_codes::OK, result.first);
    CHECK(result.second.is_array());
    CHECK_EQUAL(2, result.second.as_array().size());

    // Proper request
    // Uses 1 property to request 0 entities
    desired_properties.clear();
    desired_properties.push_back(
      make_pair("NonexistentProperty", value::string("*") )
    );

    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + BasicFixture::table,
      value::object(desired_properties)
    );
    CHECK_EQUAL(status_codes::OK, result.first);
    CHECK(result.second.is_array());
    CHECK_EQUAL(0, result.second.as_array().size());

    // Empty table name
    desired_properties.clear();
    desired_properties.push_back( make_pair("City", value::string("*") ));

    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/",
      value::object(desired_properties)
    );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    // Incorrect table name
    desired_properties.clear();
    desired_properties.push_back( make_pair("City", value::string("*") ));

    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + "NonexistentTable",
      value::object(desired_properties)
    );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    // JSON object with no properties
    // Should be the same as a proper GetAll request
    desired_properties.clear();

    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + BasicFixture::table,
      value::object(desired_properties)
    );
    CHECK_EQUAL(status_codes::OK, result.first);
    CHECK_EQUAL(3, result.second.as_array().size());

    // JSON object with non-'*' value
    desired_properties.clear();
    desired_properties.push_back(
      make_pair("City", value::string("NonAsteriskValue") )
    );

    result = do_request(
      methods::GET,
      string(BasicFixture::addr)
      + read_entity_admin + "/"
      + BasicFixture::table,
      value::object(desired_properties)
    );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    // Cleaning up created entities
    CHECK_EQUAL(
      status_codes::OK,
      delete_entity (
        BasicFixture::addr, BasicFixture::table, p2_partition, p2_row
      )
    );
    CHECK_EQUAL(
      status_codes::OK,
      delete_entity (
        BasicFixture::addr, BasicFixture::table, p3_partition, p3_row
      )
    );
  }
//  }
//};

  /*
    A test of GET all table entries

    Demonstrates use of new compare_json_arrays() function.
   */
  TEST_FIXTURE(BasicFixture, GetAll) {
    string partition {"Canada"};
    string row {"Katherines,The"};
    string property {"Home"};
    string prop_val {"Vancouver"};
    int put_result {put_entity (BasicFixture::addr, BasicFixture::table, partition, row, property, prop_val)};
    cerr << "put result " << put_result << endl;
    assert (put_result == status_codes::OK);

    pair<status_code,value> result {
      do_request (methods::GET,
                  string(BasicFixture::addr)
                  + read_entity_admin + "/"
                  + string(BasicFixture::table))};
    CHECK_EQUAL(status_codes::OK, result.first);

    /*
    //GET all tests

    // table name does not exist
    result = do_request( methods::GET, string(BasicFixture::addr) + "NonexistentTable" );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    // addr/table/ <- add "/" at end
    result = do_request( methods::GET, string(BasicFixture::addr) + string(BasicFixture::table) + "/" );
    CHECK_EQUAL(status_codes::OK, result.first);

    // just addr, no table name
    result = do_request( methods::GET, string(BasicFixture::addr) );
    CHECK_EQUAL( status_codes::BadRequest, result.first);


    // no addr doesn't work
    result = do_request( methods::GET, string(BasicFixture::table) );
    CHECK_EQUAL( status_codes::BadRequest, result.first);


    // table/table, second table name as partition
    result = do_request( methods::GET, string(BasicFixture::addr) + string(BasicFixture::table) + "/" + string(BasicFixture::table) );
    CHECK_EQUAL( status_codes::BadRequest, result.first);
    */

    value obj1 {
      value::object(vector<pair<string,value>> {
          make_pair(string("Partition"), value::string(partition)),
          make_pair(string("Row"), value::string(row)),
          make_pair(property, value::string(prop_val))
      })
    };
    value obj2 {
      value::object(vector<pair<string,value>> {
          make_pair(string("Partition"), value::string(BasicFixture::partition)),
          make_pair(string("Row"), value::string(BasicFixture::row)),
          make_pair(string(BasicFixture::property), value::string(BasicFixture::prop_val))
      })
    };
    vector<object> exp {
      obj1.as_object(),
      obj2.as_object()
    };
    compare_json_arrays(exp, result.second);
    CHECK_EQUAL(status_codes::OK, delete_entity (BasicFixture::addr, BasicFixture::table, partition, row));
  }

  /*
    A test of GET all entities from a specific partition
  */
  TEST_FIXTURE(BasicFixture, GetPartition) {
    string partition {"Katherines,The"};
    string row {"Canada"};
    string property {"Home"};
    string prop_val {"Vancouver"};
    int put_result {put_entity (BasicFixture::addr, BasicFixture::table, partition, row, property, prop_val)};
    cerr << "put result " << put_result << endl;
    assert (put_result == status_codes::OK);

    pair<status_code,value> result {
      do_request (methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + string(BasicFixture::table))
    };

    CHECK(result.second.is_array());
    CHECK_EQUAL(2, result.second.as_array().size());
    /*
      Checking the body is not well-supported by UnitTest++, as we have to test
      independent of the order of returned values.
     */
    //CHECK_EQUAL(body.serialize(), string("{\"")+string(BasicFixture::property)+ "\":\""+string(BasicFixture::prop_val)+"\"}");
    CHECK_EQUAL(status_codes::OK, result.first);

    //TESTS

    // table does not exist
    result = do_request( methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + "NonexistentTable" );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    // missing table name
    result = do_request( methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + "/" + string(BasicFixture::partition) + "/*" );
    CHECK_EQUAL( status_codes::BadRequest, result.first);

    // missing partition name
    result = do_request( methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + string(BasicFixture::table) + "//*" );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    //missing * for row
    result = do_request( methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + string(BasicFixture::table) + "/" + partition + "/" );
    CHECK_EQUAL(status_codes::BadRequest, result.first);

    //give a correct row name
    result = do_request( methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + string(BasicFixture::table) + "/" + string(BasicFixture::partition) + "/" + string(BasicFixture::row) );
    CHECK_EQUAL(status_codes::OK, result.first);

    //give a row name that doesn't match
    result = do_request( methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + string(BasicFixture::table) + "/" + partition + "/Korea" );
    CHECK_EQUAL(status_codes::NotFound, result.first);

    //deletes key Katherines,The
    CHECK_EQUAL(status_codes::OK, delete_entity (BasicFixture::addr, BasicFixture::table, partition, row) );

    //to show it is gone: give correct Katherines,The
    // test deleted partition
    result = do_request( methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + string(BasicFixture::table) + "/" + partition + "/*" );
    CHECK_EQUAL(status_codes::OK, result.first);

    //table found and ok
    result = do_request( methods::GET, string(BasicFixture::addr) + read_entity_admin + "/" + string(BasicFixture::table) + "/" + string(BasicFixture::partition) + "/*" );
    CHECK_EQUAL(status_codes::OK, result.first);

    //don't need this anymore because Katherins,The is deleted already
    //CHECK_EQUAL(status_codes::OK, delete_entity (BasicFixture::addr, BasicFixture::table, partition, row) );
  }
};
