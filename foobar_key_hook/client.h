#pragma once
#include <vector>

#include "exceptions.h"
#include "logging.h"
#include "pipe_client.h"
#include "serialization.h"

#define SUCCESS 0

enum PlaybackOrder {
  Default,
  RepeatPlaylist,
  RepeatTrack,
  Random,
  ShuffleTracks,
  ShuffleAlbums
};


class Client {
private:
  PipeClient underlying_client;
  
  template<typename T>
  std::tuple<int, T> unpack_content(std::vector<char> & content) {
    try {
      return serialization::Msgpack::unpack<std::tuple<int, T>>(content);
    }
    catch (msgpack::v1::type_error) {
      throw RPCException("Failed unpacking result, due to bad_cast");
    }
  }

  template<typename T>
  T process_request(serialization::Payload content) {
    T unpacked;
    int status;    
    std::vector<char> received;
    
    auto send_result = underlying_client.send(content.data, content.size);
    if (send_result.isFailed()) {
      auto msg = tfm::format(
        "Failed sending to pipe, with error %d",
        send_result.error());
      throw RPCException(msg);
    }

    auto result = underlying_client.recv();
    if (result.isFailed()) {
      auto msg = tfm::format("Failed receiving data from pipe %d", result.error());      
      throw RPCException(msg);      
    }

    std::tie(std::ignore, received) = result.result();
    
    std::tuple<int, T> unpacked_result = unpack_content<T>(received);
    std::tie(status, unpacked) = unpacked_result;
    if (status != SUCCESS) {
      auto msg = tfm::format("Operation failed with error %s", unpacked);
      throw RPCException(msg);
    }
    return unpacked;
  }

public:

  Client() {}
  Client(PipeClient client) : underlying_client(client) {}
  
  virtual bool play_or_pause();
  virtual bool previous();
  virtual bool next();
  virtual bool playback_order_set_active(PlaybackOrder value);

};
