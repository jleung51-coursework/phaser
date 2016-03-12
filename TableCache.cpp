#include "TableCache.h"

#include <cassert>
#include <string>
#include <unordered_map>

#include <was/storage_account.h>
#include <was/table.h>

using azure::storage::cloud_storage_account;
using azure::storage::cloud_table;
using azure::storage::cloud_table_client;
using azure::storage::storage_uri;

using pplx::extensibility::critical_section_t;
using pplx::extensibility::scoped_critical_section_t;

using std::string;

using web::http::uri;

using cache_t = std::unordered_map<string,cloud_table>;

cloud_table TableCache::lookup_table(const string& table_name) {
  assert (client.base_uri ().path() != "");
  scoped_critical_section_t lock {resplock};

  auto entry (table_cache.find(table_name));
  if (entry == table_cache.end()) {
      cloud_table table {client.get_table_reference(table_name)};
      table_cache[table_name] = table;
      return table;
  }
  return entry->second;
}

bool TableCache::delete_entry(const string& table_name) {
  scoped_critical_section_t lock {resplock};

  cache_t::size_type count {table_cache.erase(table_name)};
  return count == 1;
}
