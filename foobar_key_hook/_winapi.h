#pragma once
#include <tuple>
#include <vector>
#include <string>

#include <windows.h>

#include "result.h"


Result<DWORD> send_bytes(HANDLE handle, std::string writeBuffer, int len);
Result<std::tuple<DWORD, DWORD>> recv_bytes(HANDLE handle, char * readBuffer, int size);
Result<std::tuple<DWORD, std::vector<char>>> get_more_data(HANDLE handle);
HANDLE connect_to_pipe(std::string pipe, DWORD connection_delay);
