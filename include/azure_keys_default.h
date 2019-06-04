#ifndef AZURE_KEYS_H
#define AZURE_KEYS_H

#include <string>

const std::string tables_endpoint { "http://USERNAME.table.core.windows.net/" };

const std::string storage_connection_string {
	"DefaultEndpointsProtocol=https;"
	"AccountName=;"
	"AccountKey="};

#endif
