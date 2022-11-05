#pragma once

#include <fstream>
#include <string>

#include "../util/diagnostic.h"

class LocatableStream {
   public:
    LocatableStream(std::string const& filename)
        : m_stream(std::ifstream(filename))
        , m_filename(filename) {}

    unsigned char get();
    std::string get_str(size_t length);
    std::string get_line();

    unsigned char peek(size_t offset = 0);
    std::string peek_str(size_t length);

    Locatable loc();
 
   private:
    unsigned char get_or_eof();

    std::ifstream m_stream;

    std::string const& m_filename;
    size_t m_line = 1;
    size_t m_column = 1;
};
