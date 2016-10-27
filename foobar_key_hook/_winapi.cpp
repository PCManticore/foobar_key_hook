#pragma once
#include <string>
#include <tuple>
#include <vector>

#include "exceptions.h"
#include "_winapi.h"
#include "logging.h"
#include "result.h"


Result<std::tuple<DWORD, DWORD>> write_to_pipe(
  HANDLE handle, std::string writeBuffer, int len) {

  DWORD written;
  BOOL ret;
  DWORD err;

  ret = WriteFile(
    handle, writeBuffer.c_str(), len, &written,
    nullptr);
  err = ret ? 0 : GetLastError();

  if (!ret) {
    logMessage("Failed writing to pipe: %d", err);
    return Result<std::tuple<DWORD, DWORD>>::withError(err);
  }
  return Result<std::tuple<DWORD, DWORD>>(std::make_tuple(err, written));
}


Result<std::tuple<DWORD, DWORD>> read_from_pipe(HANDLE handle, char * readBuffer, int size) {

  DWORD nread;
  DWORD err;
  BOOL ret;

  ret = ReadFile(handle, readBuffer, size, &nread, nullptr);

  err = ret ? 0 : GetLastError();
  if (!ret) {    
    if (err != ERROR_MORE_DATA) {
      logMessage("Failed reading from pipe: %d", err);
      return Result<std::tuple<DWORD, DWORD>>::withError(err);
    }
  }
  return Result<std::tuple<DWORD, DWORD>>(std::make_tuple(err, nread));
}

Result<std::tuple<DWORD, DWORD>> peek_named_pipe(HANDLE handle) {
  DWORD navail, nleft;
  BOOL ret;

  ret = PeekNamedPipe(handle, NULL, 0, NULL, &navail, &nleft);

  if (!ret) {
    return Result<std::tuple<DWORD, DWORD>>::withError(GetLastError());
  }
  return Result<std::tuple<DWORD, DWORD>>(std::make_tuple(navail, nleft));

}

Result<std::tuple<DWORD, std::vector<char>>> get_more_data(HANDLE handle) {
  DWORD nleft;
  DWORD nread;
  auto resultPeek = peek_named_pipe(handle);

  if (resultPeek.isFailed()) {
    return Result<std::tuple<DWORD, std::vector<char>>>::withError(resultPeek.error());
  }
  std::tie(std::ignore, nleft) = resultPeek.result();

  assert(nleft > 0);

  std::vector<char> buffer(nleft);
  auto result = read_from_pipe(handle, &buffer[0], nleft);

  if (result.isFailed()) {
    return Result<std::tuple<DWORD, std::vector<char>>>::withError(result.error());
  }

  std::tie(std::ignore, nread) = result.result();
  return Result<std::tuple<DWORD, std::vector<char>>>(std::make_tuple(nread, buffer));
}

Result<DWORD> send_bytes(HANDLE handle, std::string writeBuffer, int len) {

  auto result = write_to_pipe(handle, writeBuffer, len);
  if (result.isFailed()) {
    return Result<DWORD>::withError(result.error());
  }

  return Result<DWORD>(GetLastError());

}

Result<std::tuple<DWORD, DWORD>> recv_bytes(HANDLE handle, char * readBuffer, int size) {  
  DWORD lastError;
  DWORD nread;

  Result<std::tuple<DWORD, DWORD>> result = read_from_pipe(handle, readBuffer, size);
  if (result.isFailed()) {
    logMessage("Failed receiving from pipe %d", result.error());
    return result;
  }
  std::tie(lastError, std::ignore) = result.result();

  switch (lastError) {
  case ERROR_MORE_DATA:
  case ERROR_SUCCESS:
    // Operation succeeded or needs additional operations.
    return Result<std::tuple<DWORD, DWORD>>(std::make_tuple(lastError, nread));
  default:
    return Result<std::tuple<DWORD, DWORD>>::withError(lastError);
  }
  return Result<std::tuple<DWORD, DWORD>>::withError(lastError);
}


HANDLE connect_to_pipe(std::string pipe, DWORD connection_delay) {

  auto handle = CreateFile(
    pipe.c_str(),
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    0,
    NULL);

  if (handle == INVALID_HANDLE_VALUE) {
    if (GetLastError() != ERROR_PIPE_BUSY) {
      std::string msg = tfm::format(
        "Could not open pipe, failed with error: %d", GetLastError()
      );
      throw RPCException(msg);
    }

    if (!WaitNamedPipe(pipe.c_str(), connection_delay)) {
      throw RPCException("Cannot connect to pipe, it seems busy");
    }
  }

  DWORD dwMode = PIPE_READMODE_MESSAGE;
  BOOL success = SetNamedPipeHandleState(
    handle,    // pipe handle 
    &dwMode,  // new pipe mode 
    NULL,     // don't set maximum bytes 
    NULL);    // don't set maximum time 
  if (!success)
  {
    auto msg = tfm::format(
      "Cannot change the mode of the pipe, "
      "failing with error %s",
      GetLastError());
    throw RPCException(msg);
  }

  return handle;
}
