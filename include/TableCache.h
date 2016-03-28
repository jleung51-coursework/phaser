#ifndef TableCache_h
#define TableCache_h

#include <string>
#include <unordered_map>

#include <pplx/pplxtasks.h>

#include <was/storage_account.h>
#include <was/table.h>

class TableCache {
private:
  azure::storage::cloud_storage_account account;
  azure::storage::cloud_table_client client;
  std::unordered_map<std::string,azure::storage::cloud_table> table_cache;
  pplx::extensibility::critical_section_t resplock;
public:
  TableCache () : 
    account {},
    client {},
    table_cache {},
    resplock {}
    {};

  void init(const std::string& connection) {
    account  = azure::storage::cloud_storage_account::parse(connection);
    client = account.create_cloud_table_client();
  };

  azure::storage::cloud_table lookup_table(const std::string& table_name);
  bool delete_entry(const std::string& table_name);
};

#endif
