#include <iostream>
#include <functional>
#include <unordered_map>
#include <memory>

#include <Windows.h>

#include "client.h"
#include "logging.h"
#include "pipe_client.h"
#include "serialization.h"

#define PIPE "\\\\.\\pipe\\foobar2000"

enum Event {
  BACK = 0x42,
  PAUSE = 0x50,
  NEXT = 0x4E,
  RANDOM = 0x52,
  PLAYBACK_ORDER_DEFAULT = 0x44,
  PLAYBACK_ORDER_SHUFFLE_ALBUMS = 0x53
};


using events_map = std::unordered_map<Event, std::function<void()>>;


class Dispatcher {  

private:  
  PipeClient pipe_client;
  Client client;

  std::function<void()> wrap_callback(std::function<void(Client)> func) {
    return [func, &client=client] {
      try {
        func(client);
      }
      catch (RPCException& exc) {
        auto msg = exc.what();
        logMessage(msg);
      };
    };
  }

public:
  events_map events;

  Dispatcher(std::string pipe_address) {
    pipe_client = PipeClient::connect(pipe_address);
    client = Client(pipe_client);

    events = {
      {
        Event::PAUSE,
        wrap_callback([](Client client) { client.play_or_pause(); })
      },
      {
        Event::BACK,
        wrap_callback([](Client client) { client.previous(); })
      },
      {
        Event::NEXT,
        wrap_callback([](Client client) { client.next(); })
      },
      {
        Event::RANDOM, wrap_callback([](Client client) {
        client.playback_order_set_active(PlaybackOrder::Random);
        client.next();
       })
      },
      {
        Event::PLAYBACK_ORDER_DEFAULT,
         wrap_callback([](Client client) { client.playback_order_set_active(PlaybackOrder::Default); })
      },
      {
        Event::PLAYBACK_ORDER_SHUFFLE_ALBUMS,
         wrap_callback([](Client client) { client.playback_order_set_active(PlaybackOrder::ShuffleAlbums); })
       }
    };
  }

  void stop() {
    pipe_client.close();
  }

};



void register_hot_keys(Dispatcher & dispatcher) {
 for (events_map::iterator iterator = dispatcher.events.begin();
      iterator != dispatcher.events.end();
      iterator++) {

   auto key = iterator->first;
   if (RegisterHotKey(NULL, key, MOD_CONTROL | MOD_SHIFT, key)) {
     logMessage("Registered hotkey CONTROL + SHIFT + %x", key);
   } else {
     logMessage("Could not register hot key CONTROL + SHIFT + %x", key);
    }
  }
}

int main() {
  
  MSG msg;
  Dispatcher dispatcher(PIPE);
  register_hot_keys(dispatcher);
  
  while (GetMessage(&msg, NULL, 0, 0) != 0)
  {    
    if (msg.message == WM_HOTKEY)
    {
      events_map::const_iterator elem = dispatcher.events.find((Event)msg.wParam);

      if (elem == dispatcher.events.end()) {
        logMessage("Could not find element to dispatch to.");
      }
      else {
        logMessage("Received keyboard event %d", (Event)msg.wParam);
        elem->second();
      }
    }
  }
  dispatcher.stop();

  return 0;

}