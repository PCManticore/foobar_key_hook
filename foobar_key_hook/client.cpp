#pragma once
#include <tuple>
#include <vector>

#include "client.h"
#include "pipe_client.h"
#include "serialization.h"


bool Client::play_or_pause() {  
  auto content = serialization::Msgpack::pack(
    std::make_tuple(
      "PlaybackControl.play_or_pause"));
  return process_request<bool>(content);
}

bool Client::previous() {
  auto content = serialization::Msgpack::pack(
    std::make_tuple(
      "PlaybackControl.previous"));
  return process_request<bool>(content);
}

bool Client::next() {
  auto content = serialization::Msgpack::pack(
    std::make_tuple(
      "PlaybackControl.next"));
  return process_request<bool>(content);
}

bool Client::playback_order_set_active(PlaybackOrder value) {
  auto params = serialization::Msgpack::pack((int)value);
  auto content = serialization::Msgpack::pack(
    std::make_tuple(
      "Playlist.playback_order_set_active",
      params.data)
  );
  return process_request<bool>(content);
}

std::tuple<bool, size_t, size_t> Client::get_playing_item_location() {  
  auto content = serialization::Msgpack::pack(
    std::make_tuple("Playlist.get_playing_item_location")
  );
  return process_request<std::tuple<bool, size_t, size_t>>(content);
}

bool Client::queue_add_items(size_t playlist, size_t item) {
  auto params = serialization::Msgpack::pack(
    std::make_tuple(playlist, item)
  );
  auto content = serialization::Msgpack::pack(
    std::make_tuple("Playlist.queue_add_item_playlist", params.data)
  );
  return process_request<bool>(content);
}
