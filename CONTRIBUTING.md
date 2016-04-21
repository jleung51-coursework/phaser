# Contributing

## Setup

### Development Environment

This project requires several packages and libraries; here are some instructions to get your environment set up accordingly.

The directory structure is as follows:

```
[parent_directory]
 ├ casablanca (C++ REST SDK)
 ├ azure-storage-cpp (Microsoft Azure Storage)
 ├ unittest-cpp (UnitTest++)
 └ phaser
```

#### Necessary Packages

* General programming:
  * cmake
  * git
  * g++
  * make
* Boost libraries:
  * libboost1.54-all-dev
* Client-server communication:
  * libssl-dev
* Microsoft Azure Storage Client Library dependencies:
  * libxml++2.6-dev
  * libxml++2.6-doc
  * uuid-dev

To make sure you have all of these up-to-date, run:
```
sudo apt-get update
sudo apt-get install cmake git g++ make libboost1.54-all-dev libssl-dev libxml++2.6-dev libxml++2.6-doc uuid-dev
```

### C++ REST SDK

To install the [C++ REST SDK](https://github.com/Microsoft/cpprestsdk) (codenamed *Casablanca*) into a directory named `casablanca/` in the current directory, run the following commands:

```
git clone https://github.com/Microsoft/cpprestsdk.git casablanca
cd casablanca/Release
mkdir build.release
cd build.release
CXX=g++-4.8
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### Microsoft Azure Storage Client Library

To install the [Microsoft Azure Storage Client Library](https://github.com/Azure/azure-storage-cpp) into a directory named `azure-storage-cpp/` in the current directory, run the following commands:

(Replace the CASABLANCA_DIR path with the path to your local `casablanca` installation if necessary)

```
git clone https://github.com/Azure/azure-storage-cpp.git
cd azure-storage-cpp/Microsoft.WindowsAzure.Storage
mkdir build.release
cd build.release
CASABLANCA_DIR=../../casablanca
CXX=g++-4.8
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### UnitTest++

To install [UnitTest++](https://github.com/unittest-cpp/unittest-cpp) into a directory named `unittest-cpp` in the current directory, run the following commands:

```
git clone https://github.com/unittest-cpp/unittest-cpp.git
cd unittest-cpp/builds
cmake -G "Unix Makefiles" ../
cmake --build ./
```

### phaser

Grab a local copy of the project from us:

```
git clone https://github.com/CMPT276-2016spring/phaser
```

or, if you've already cloned the project:

```
git clone https://github.com/USERNAME/phaser
```

### Microsoft Azure account

You'll need a [Microsoft Azure](https://azure.microsoft.com) account to run these servers, in order to connect to the databases.

Head to the [Azure Portal](https://portal.azure.com/) once you have your account.

Set up your Microsoft Azure storage account **[further instructions to be added]**.

Now it's time to authenticate the project with your Microsoft Azure account.  
In the Azure Portal, navigate to the left menu and click on "All resources". Choose the newly-created entry, which should have the Type "Storage account".  
If the "Settings" menu does not automatically open, click the small button which says "All settings" to the upper right of the section named "Services". Under "General", click "Access keys". Take note of the storage account name and access keys; you will need them in the next section.

Copy `include/azure_keys_default.h` to `include/azure_keys.h`. `azure_keys.h` has been added to the `.gitignore` file; do NOT commit your `azure_keys.h` file to the repository.  
In `azure_keys.h`:
1. Copy the text from "Storage account name" and replace `[AZURE_STORAGE_ACCOUNT_NAME]` in the string `tables_endpoint` with it.
2. Copy the text from "Storage account name" and paste it after the `AccountName=` property (but before the `;"`) in the string `storage_connection_string`.
3. Copy either of the access keys and paste it after the `AccountKey=` property (but before the `"`) in the string `storage_connection_string`.

## Issue Priorities

| Priority Level | Meaning |
| --- | --- |
DEFCON 0 | Needs immediate attention, critical to fix
DEFCON 1 | Needs attention prior to release
DEFCON 2 | Strongly recommended to be completed prior to release
DEFCON 3 | Recommended and useful to focus on
DEFCON 4 | Complete only if there's nothing else to do
