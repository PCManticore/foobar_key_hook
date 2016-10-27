#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

#include "../dependencies/tinyformat/tinyformat.h"

#define LOGFILE std::experimental::filesystem::current_path() / "foobarhook.log"

template<typename ...Args>
void logMessage(std::string message, const Args&... args) {

  std::ostringstream oss;
  std::string formattedMessage;
  const char* fmt_c;

  fmt_c = message.c_str();
  tfm::format(oss, fmt_c, args...);
  formattedMessage = oss.str();

  std::cout << formattedMessage << std::endl;

  auto stream = std::ofstream(LOGFILE, std::ios::app);
  stream << formattedMessage << std::endl;
  stream.close();

}
