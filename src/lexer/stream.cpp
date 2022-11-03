#include "stream.h"

char LocatableStream::get() {
    char c = m_stream.get();
    // TODO handle \r\n and \r
    if (c == '\n') {
        m_column = 1;
        m_line++;
    } else {
        m_column++;
    }
    return c;
}

std::string LocatableStream::get_str(int length) {
    std::string str(length, '\0');
    for (int i = 0; i < length; i++) {
        str.append(1, get());
    }
    return str;
}

std::string LocatableStream::get_line() {
    std::string str;
    m_stream.getline(&str[0], 512);
    // FIXME: Broken
    m_column++;
    return str;
}

char LocatableStream::peek(size_t offset) {
    for (size_t i = 0; i < offset; i++) {
        m_stream.get();
    }
    char c = m_stream.peek();
    for (size_t i = 0; i < offset; i++) {
        m_stream.unget();
    }
    return c;
}

std::string LocatableStream::peek_str(int length) {
    std::string str(length, '\0');
    m_stream.read(&str[0], length);
    for (int i = 0; i < length; i++) {
        m_stream.unget();
    }
    return str;
}

Locatable LocatableStream::loc() {
    return Locatable(m_filename, m_line, m_column);
}
