/*
  Utilities primarily of use in clients.

  However the build_json_value () functions may also
  be useful in server code that needs to construct a
  JSON value for a response to its client.  
 */

#include "ClientUtils.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <utility>

#include <cpprest/http_client.h>
#include <cpprest/json.h>

#include <pplx/pplxtasks.h>

using std::make_pair;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

using web::http::http_headers;
using web::http::http_request;
using web::http::http_response;
using web::http::method;
using web::http::status_code;
using web::http::status_codes;

using web::http::client::http_client;

using web::json::object;
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

  If the URI denotes an address/port combination that cannot be
  located (say because the server is not running or the port 
  number is incorrect), the routine throws a web::uri_exception().

  NOTE:  This version differs slightly from the do_request() that
  was included in the original tester.cpp.  In the case where
  the response has no JSON object as a message body,
  this version returns an empty JSON object (value::object ())
  as the second half of the pair.  The old version returned
  a null JSON value (value {}).  The old version was more
  precise but the new version is easier to work with, albeit
  ambiguous in some edge cases that don't matter for these 
  assignments.

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
              return pplx::task<value> ([] { return value::object ();});
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
 Return a JSON object value whose (0 or more) properties are specified as a 
 vector of <string,string> pairs
 */
value build_json_value (const vector<pair<string,string>>& props) {
  vector<pair<string,value>> vals {};
  std::transform (props.begin(),
                  props.end(),
                  std::back_inserter(vals),
                  [] (const pair<string,string>& p) {
                    return make_pair(p.first, value::string(p.second));
                  });
  return value::object(vals);
}

/*
  Return a JSON object value with a single property, specified
  as a pair of strings
 */
value build_json_value (const pair<string,string>& prop) {
  return value::object(vector<pair<string,value>> {
      make_pair (prop.first, value::string (prop.second))
    });
}

/*
  Return a JSON object value with a single property, specified
  as two string arguments
 */
value build_json_value (const string& propname, const string& propval) {
  return value::object(vector<pair<string,value>> {
      make_pair (propname, value::string (propval))
    });
}

/*
  Return a JSON object value with two properties, specified
  as four string arguments
 */
value build_json_value (const string& pname1, const string& pval1,
                        const string& pname2, const string& pval2) {
  return value::object(vector<pair<string,value>> {
      make_pair (pname1, value::string (pval1)),
      make_pair (pname2, value::string (pval2))
    });
}

/*
  Return unordered_map representing property/value pairs in a JSON object

  v: A value that must be is_object()

  All values are converted to strings, either directly if the property
  is a string or by serialize() if they are not.
 */
unordered_map<string,string> unpack_json_object (const value& v) {
  assert(v.is_object());
  
  unordered_map<string,string> res {};
  object obj {v.as_object()};
  for (const auto& p : obj) {
    if (p.second.is_string())
      res[p.first] = p.second.as_string();
    else
      res[p.first] = p.second.serialize();
  }

  return res;
}

/*
  Return a value associated with a given property name in a JSON object
  v: A JSON value, must be is_object()
  propname: Name of property whose value to retrieve

  If v is not an object, an assertion is thrown.

  If v is an object but does not have a property named propname,
  return a null JSON value (http://microsoft.github.io/cpprestsdk/classweb_1_1json_1_1value.html#a3f5ac5b637f5f3a55f01fc95b49ea671)
 */
value get_json_object_prop_val (const value& v, const string& propname) {
  assert(v.is_object());

  object obj {v.as_object()};
  return obj [propname];
}

/*
  Return a string representation of a value associated with a given
  property name in a JSON object

  v: A JSON value, must be is_object()
  propname: Name of property whose value to retrieve

  If v is not an object, an assertion is thrown

  If v is an object but does not have a property named propname,
  return an empty string.

  If v is an object with a property named propname,
  - if the property is a string, return the string
  - if the property is any non-null, non-string type, return its serialization
 */
string get_json_object_prop (const value& v, const string& propname) {
  value propval {get_json_object_prop_val (v, propname)};
  if (propval.is_null())
    return string {};
  else if (propval.is_string())
    return propval.as_string();
  else
    return propval.serialize();
}

using pos_t = string::size_type;

char pair_separator {'|'};
char pair_delimiter {';'};

static pos_t next_delim (const string& s, char delim, pos_t start) {
  return s.find(delim, start);
}

/*
 Return a vector of (country, name) pairs representing a list of friends

 The parameter is a string representing a friends list as described below:

 A friends list consists of a string of paired character sequences, country
 and full name. The pairs are separated by a character, pair_separator, while the
 elements of the pair are delimited by a distinct character, pair_delimiter.

 The standard form of a friend-list only places the pair_separator between
 consecutive pairs. However, the function will accept strings with a leading
 or trailing pair_separator.

 If a pair does not include a pair_delimeter, the function throws a
 std::invalid_argument.

 If there are characters after the last pair_separator that do not include a 
 pair_delimiter, they are ignored.

 Assuming pair_separator is '|' and pair_delimiter is ';', correct 
 friends_list strings would include

   "USA;Madonna|Canada;Edwards,Kathleen|Korea;BigBang" (standard form)
   "|USA;Madonna|Canada;Edwards,Kathleen|Korea;BigBang|" (leading+trailing seps)
   "USA;Madonna|Canada;Edwards,Kathleen|KoreaBigBang" (but "KoreaBigBang" is ignored)

 but the following will throw an std::invalid_arugment

   "USAMadonna|Canada" (no delimiter in opening "pair")

 Note: gcc 4.8.4's version of STL does not include full support for <regexp>,
 so this algorthm uses the more basic (and entirely adequate) string::find.
 */
friends_list_t parse_friends_list (const string& friends_list) {
  friends_list_t res {};

  pos_t start {0};
  if (friends_list[start] == pair_separator)
    start++; // Skip any initial separator
  for (pos_t delim {next_delim (friends_list, pair_delimiter, start)};
       delim != string::npos; 
       delim = next_delim (friends_list, pair_delimiter, start)) {
    pos_t end {next_delim (friends_list, pair_separator, start)};
    if (end == string::npos)
      end = friends_list.size();
    if (end <= delim+1)
      throw std::invalid_argument(string("Misformed friends list: ") + friends_list);
    string country {friends_list.substr (start, delim-start)};
    string name {friends_list.substr (delim+1, end-delim-1)};
    res.push_back (make_pair (country, name));
    start = end+1;
  }
  return res;
}

/*
  Return the string representation of a friends list 
 */
string friends_list_to_string (const friends_list_t& list) {
  string result {};
  bool started {false};

  for (const auto& p : list) {
    if (started)
      result += pair_separator;
    result += p.first + pair_delimiter + p.second;
    started = true;
  }
  return result;
}
