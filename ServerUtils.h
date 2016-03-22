#ifndef ServerUtils_h
#define ServerUtils_h

#include <string>
#include <utility>

#include <cpprest/http_listener.h>

#include <was/table.h>

std::pair<web::http::status_code,azure::storage::table_entity>
read_with_token(const web::http::http_request& message,
                const std::string& endpoint);


web::http::status_code
update_with_token (const web::http::http_request& message,
                   const std::string& endpoint,
                   const std::unordered_map<std::string,std::string>& props);
#endif
