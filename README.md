# Phaser

C++ [RESTful](https://en.wikipedia.org/wiki/Representational_state_transfer) servers which:
1. Interact with Microsoft Azure Storage Tables for database storage,
2. Authenticate a client with read or read-and-update permissions for 24 hours,
3. Create records of active user sessions and status updates, and
4. Push new status updates to all currently active users.

Originated as a group project in CMPT 276 at SFU, for the Spring 2016 semester.  
All development from 2016-04-12 onwards is independent from the course material.

Migrated from [GitLab](https://csil-git1.cs.surrey.sfu.ca/cmpt276-group-faze/phaser) on 2016-04-12.

## Technologies

| Purpose | Technology |
|---|---|
| Communication protocol | [REST](https://en.wikipedia.org/wiki/Representational_state_transfer) |
| Database storage | [Microsoft Azure Storage Client Library](https://github.com/Azure/azure-storage-cpp) |
| Client-server communication API | [Microsoft C++ REST SDK](https://github.com/Microsoft/cpprestsdk) |
| Unit testing framework | [UnitTest++](https://github.com/unittest-cpp/unittest-cpp) |

## Main Contributors

* Daphne Chong
* Jeffrey Leung
* Yi-Hsuan Wu
* Joshua Wu
