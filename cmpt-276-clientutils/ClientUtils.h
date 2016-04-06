#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cpprest/http_client.h>
#include <cpprest/json.h>

// Alias for a type representing the result of do_request()
using req_res_t = std::pair<web::http::status_code,web::json::value>;

// Alias for a vector representing a friends list
using friends_list_t = std::vector<std::pair<std::string,std::string>>;

// Alias for an unordered_map representing a JSON object's property/value pairs
using value_string_t = std::unordered_map<std::string,std::string>;

req_res_t
do_request (const web::http::method& http_method, const std::string& uri_string, const web::json::value& req_body);

req_res_t
do_request (const web::http::method& http_method, const std::string& uri_string);

web::json::value
build_json_value (const std::vector<std::pair<std::string,std::string>>& props);

web::json::value
build_json_value (const std::pair<std::string,std::string>& prop);

web::json::value
build_json_value (const std::string& propname, const std::string& propval);

web::json::value
build_json_value (const std::string& pname1, const std::string& pval1,
                  const std::string& pname2, const std::string& pval2);

value_string_t
unpack_json_object (const web::json::value& v);

web::json::value
get_json_object_prop_val (const web::json::value& v, const std::string& propname);

std::string
get_json_object_prop (const web::json::value& v, const std::string& propname);

extern char pair_separator;
extern char pair_delimiter;

friends_list_t
parse_friends_list (const std::string& friends_list);

std::string friends_list_to_string(const friends_list_t& list);

#endif
