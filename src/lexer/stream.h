#pragma once

#include <fstream>
#include <string>

#include "../util/diagnostic.h"

class LocatableStream {
  public:
    LocatableStream(std::string const& file_name)
        : _stream(std::ifstream(file_name))
        , _file_name(file_name) {
        if (!this->_stream.good()) {
            error("File '", file_name, "' does not exist");
        }
    }

    unsigned char get();
    std::string getStr(size_t length);
    std::string getLine();

    unsigned char peek(size_t offset = 0);
    std::string peekStr(size_t length);

    Locatable loc();

  private:
    unsigned char getOrEof();

    std::ifstream _stream;

    std::string const& _file_name;
    size_t _line = 1;
    size_t _column = 1;
};
