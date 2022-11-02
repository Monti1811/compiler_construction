#pragma once

#include <fstream>
#include <string>

#include "../util/diagnostic.h"

class LocatableStream {
   public:
    LocatableStream(std::string const& filename)
        : m_stream(std::ifstream(filename))
        , m_filename(filename) {}

    char get();
    std::string getline();
    char peek();
    char peek_twice();
    std::string read(int length);
    std::string peek_forward(int length);

    Locatable loc();
    
   private:
    std::ifstream m_stream;

    std::string const& m_filename;
    size_t m_line = 1;
    size_t m_column = 1;
};
