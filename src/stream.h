#pragma once

#include <fstream>
#include <string>
#include "diagnostic.h"

class LocatableStream {
    public:
    LocatableStream(std::string filename)
        : m_filename(filename)
    {
        m_line = 1;
        m_column = 1;
    };

    char get();
    char peek();

    Locatable loc();
    
    private:
    std::ifstream m_stream;

    std::string m_filename;
    size_t m_line;
    size_t m_column;
};
