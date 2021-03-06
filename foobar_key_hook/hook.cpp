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
  BACK = 0x42, // B
  PAUSE = 0x50, // P
  NEXT = 0x4E, // N
  RANDOM = 0x52, // R
  PLAYBACK_ORDER_DEFAULT = 0x44, // D
  PLAYBACK_ORDER_SHUFFLE_ALBUMS = 0x53, //S
  REPEAT = 0x41 // A
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
       },
       {
         Event::REPEAT,
         wrap_callback([](Client client) {
           bool success;
           size_t playlist;
           size_t item;

           auto result = client.get_playing_item_location();
           std::tie(success, playlist, item) = result;

           if (success) {
             client.queue_add_items(playlist, item);
             // Jump to the next play of the same song.
             client.next();
           }
         })
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{  
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