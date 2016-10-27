#pragma once
#include <string>
#include <vector>
#include <Windows.h>

#include "exceptions.h"
#include "logging.h"
#include "result.h"
#include "_winapi.h"
#include "../dependencies/tinyformat/tinyformat.h"


#define INITIAL_READ_SIZE 128


class PipeClient {
private:
  HANDLE handle;

public:
  PipeClient() {}
  PipeClient(HANDLE handleObj) : handle(handleObj) {}
  PipeClient(const PipeClient& other) : handle(other.handle) {}
  PipeClient& operator=(const PipeClient& other) {
    handle = other.handle;
    return *this;
  }

  static PipeClient connect(std::string pipe, DWORD connection_delay = 2000) {
    HANDLE handle = connect_to_pipe(pipe, connection_delay);
    return PipeClient(handle);
  }

  void close() {
    CloseHandle(handle);
  }

  Result<std::tuple<DWORD, std::vector<char>>> recv() {
    // TODO: kinda convoluted code
    DWORD lastError, nread = 0;
    std::vector<char> moreData;
    std::vector<char> buffer(INITIAL_READ_SIZE);

    auto result = recv_bytes(handle, &buffer[0], INITIAL_READ_SIZE);
    if (result.isFailed()) {
      logMessage("Failed receiving bytes %d", result.error());
      return Result<std::tuple<DWORD, std::vector<char>>>::withError(result.error());
    }

    std::tie(lastError, nread) = result.result();
    if (lastError == ERROR_MORE_DATA) {
      Result<std::tuple<DWORD, std::vector<char>>> moreDataResult = get_more_data(handle);
      if (moreDataResult.isFailed()) {
        logMessage("Failed retrieving more data %d", moreDataResult.error());
        return Result<std::tuple<DWORD, std::vector<char>>>::withError(moreDataResult.error());
      }

      std::tie(std::ignore, moreData) = moreDataResult.result();

      buffer.insert(end(buffer), begin(moreData), end(moreData));

    }
    return Result<std::tuple<DWORD, std::vector<char>>>(
      std::make_tuple(nread + buffer.size(), buffer)
      );
  }

  Result<DWORD> send(std::string content) {
    return send(content, content.length());
  }

  Result<DWORD> send(std::string content, DWORD length) {
    return send_bytes(handle, content, length);
  }
};

