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

Make sure you have all of these up-to-date:
```
sudo apt-get update
sudo apt-get install cmake git g++ make libboost1.54-all-dev libssl-dev libxml++2.6-dev libxml++2.6-doc uuid-dev
```

#### C++ REST SDK

Install the [C++ REST SDK](https://github.com/Microsoft/cpprestsdk) (codenamed *Casablanca*) into a directory named `casablanca/` in the current directory by running the following commands:

```
git clone https://github.com/Microsoft/cpprestsdk.git casablanca
cd casablanca/Release
mkdir build.release
cd build.release
CXX=g++-4.8
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

#### Microsoft Azure Storage Client Library

Install the [Microsoft Azure Storage Client Library](https://github.com/Azure/azure-storage-cpp) into a directory named `azure-storage-cpp/` in the current directory by running the following commands:

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

#### UnitTest++

Install [UnitTest++](https://github.com/unittest-cpp/unittest-cpp) into a directory named `unittest-cpp` in the current directory by running the following commands:

```
git clone https://github.com/unittest-cpp/unittest-cpp.git
cd unittest-cpp/builds
cmake -G "Unix Makefiles" ../
cmake --build ./
```

#### phaser

Grab a local copy of the project from us:

```
git clone https://github.com/CMPT276-2016spring/phaser
```

or, if you've already cloned the project:

```
git clone https://github.com/USERNAME/phaser
```

### Microsoft Azure account

You'll need a [Microsoft Azure](https://azure.microsoft.com) subscription to run these servers, in order to connect to the databases.

Head to the [Azure Portal](https://portal.azure.com/) once you have your account.

#### Setting up the Azure Resource

In the left menu, choose *Resource groups* and click the * **+** Add* button to create a new resource. Name it anything you like, choose a subscription, and click *Create*.

Navigate to *Resource groups* again and click on the group you just created. Click the * **+** Add * button at the top and search for a *Storage account*. When you find it, click the option and choose *Create*.  
In the new window, enter a unique name for the storage account into the *Name* field. Click *Type* to select the type of your storage account (Standard-LRS *L Locally Redundant* being the most affordable). Make sure you have the resource group is set to the group you just created.  
Click the *Create* button and wait until your new resource is up and available!

#### Authenticating the Project

Now it's time to authenticate the project with your Microsoft Azure account.  
In the Azure Portal, navigate to the left menu and click on *All resources*. Choose the newly-created entry, which should have the Type *Storage account*.  
If the *Settings* menu does not automatically open, click the small button which says *All settings* to the upper right of the section named *Services*.  
Under *General*, click *Access keys*. Take note of the storage account name and access keys; you will need them in the next section.

Copy `include/azure_keys_default.h` to `include/azure_keys.h`. `azure_keys.h` has been added to the `.gitignore` file; do NOT commit your `azure_keys.h` file to the repository.  
In `azure_keys.h`:
1. Copy the text from *Storage account name* and replace `[AZURE_STORAGE_ACCOUNT_NAME]` in the string `tables_endpoint` with it.
2. Copy the text from *Storage account name* and paste it after the `AccountName=` property (but before the `;"`) in the string `storage_connection_string`.
3. Copy either of the access keys and paste it after the `AccountKey=` property (but before the `"`) in the string `storage_connection_string`.

Your environment should be set up and ready to go!

## Running the Servers and Tests

To build the servers and tests, navigate into the *build/* directory. Run CMake and build the executables:

```
cmake .
make
```

Assuming you've configured your directories and environment correctly, everything should build correctly without errors or warnings. If not, recheck your directory structure and go over the setup instructions again. Raise an issue if you're still having problems, and we'll do our best to help you!

Run each server in its own terminal - for example:

```
./basicserver
```

Once all the servers you need are up, you can run the tests:

```
./tester [suite [test]]
```

## Server Setup Requirements

Some servers require a particular setup before normal operation; if they are not set up correctly, the server may return `http::status_codes::InternalError (500)`.

### AuthServer

BasicServer must be running alongside AuthServer.

Two tables named DataTable and AuthTable are required.

DataTable may contain any or no arbitrary partition(s), row(s), and property(ies). Access for reading or reading/updating can be authorized for any single row in DataTable.

AuthTable contains a single partition named `Userid`. The name of each row in the partition corresponds to a username. Each row contains exactly 3 properties: `Password`, `DataPartition`, and `DataRow`.  
`Password` is the password for the specific user, and `DataPartition` and `DataRow` are the names of the partition and row referring to the single entry in DataTable which the user is authorized to access.

### UserServer

BasicServer and AuthServer must be running alongside UserServer.

DataTable contains any or no partition(s) and row(s). A partition is the name of the region where a user is located; a row is the name of a user in the form `LastName,FirstName`. Each row has at least the 3 properties `Friends`, `Status`, and `Updates`.  
The `Friends` property contains a string representing the user's friends. Each friend is separated by a pipe character (`|`); the partition and row are separated by a semicolon character (`;`). E.g.: `USA;Shinoda,Mike|Canada;Edwards,Kathleen`.  
The `Status` property contains a string representing the user's most recent status update. Currently, the characters `/ \n & " ' %` should not be included in a status.  
The `Updates` property contains a string representing the user's friends' status updates. Each new status update is added to the end of `Updates`, with the newline character (`\n`) separating each status update. E.g.: `StatusUpdate1\nStatusUpdate2`.

### PushServer

BasicServer, AuthServer, and UserServer must be running alongside PushServer.

## Issue Priorities

| Priority Level | Meaning |
| --- | --- |
DEFCON 0 | Needs immediate attention, critical to fix
DEFCON 1 | Needs attention prior to release
DEFCON 2 | Strongly recommended to be completed prior to release
DEFCON 3 | Recommended and useful to focus on
DEFCON 4 | Complete only if there's nothing else to do
