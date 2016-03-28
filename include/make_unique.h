#ifndef MAKE_UNIQUE_H_X
#define MAKE_UNIQUE_H_X

#include <memory>
#include <string>

namespace std {

  template<typename T, typename... Args>
  unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>{new T{forward<Args>(args)...}};
  }

}

#endif
