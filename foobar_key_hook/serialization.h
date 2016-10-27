#pragma once
#include <string>
#include <tuple>
#include <vector>

#include "msgpack.hpp"


namespace serialization {

  class Payload {
  public:
    std::string data;
    int size;

    Payload(std::string value, int value_size) {
      data = value;
      size = value_size;
    }
  };

  class Msgpack {

  public:
    template<typename T>
    static T unpack(std::vector<char> buf) {
      const char* bufdata = buf.data();
      msgpack::unpacked result;
      msgpack::unpack(result, bufdata, buf.size());
      msgpack::object obj = result.get();
      return obj.as<T>();
    };

    template<typename T>
    static Payload pack(T data) {
      msgpack::sbuffer sbuf;
      msgpack::pack(sbuf, data);
      // This is quite trashy, but we will lose the buffer
      // once it goes out of scope.
      Payload payload(std::string(sbuf.data(), sbuf.size()), sbuf.size());
      return payload;
    }

  };
};
