# Phaser

Set of C++ servers built with the [Microsoft C++ REST SDK](https://github.com/microsoft/cpprestsdk) which works as a back-end storage service in conjunction with [Microsoft Azure Storage](https://docs.microsoft.com/en-us/azure/storage/).

## Software Stack

| Purpose | Technology |
| --- | --- |
| Development Language | C++ |
| Back-End Web Server Framework | [Microsoft C++ REST SDK](https://github.com/microsoft/cpprestsdk) |
| Testing Framework | [UnitTest++](https://github.com/unittest-cpp/unittest-cpp) |
| Build System | CMake |
| Application Container | Vagrant |
| Database (Online) | [Microsoft Azure Storage](https://docs.microsoft.com/en-us/azure/storage/) |

## Setup

First, clone the repository to your local machine and enter the project directory:
```shell
git clone https://github.com/jleung51-coursework/phaser.git
cd phaser/
```

The cleanest way to run the server is to run it inside a virtual machine. To do this, install [VirtualBox](https://www.virtualbox.org/) and [Vagrant](https://www.vagrantup.com/) on your machine.

Build the virtual machine using the Vagrantfile in the project directory:
```shell
vagrant up
```

This will take some time to build.

Enter the virtual machine:
```shell
vagrant ssh
```

# About

This project was a group project in CMPT 276: Introduction to Software Engineering at Simon Fraser University for the Spring 2016 semester.

## Main Contributors

* Daphne Chong
* Jeffrey Leung
* Yi-Hsuan Wu
* Joshua Wu
