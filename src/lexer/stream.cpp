#include "stream.h"

char LocatableStream::get() {
    char c = m_stream.get();
    if (c == '\n') {
        m_column = 1;
        m_line++;
    } else {
        m_column++;
    }
    return c;
}

std::string LocatableStream::getline() {
    std::string str;
    m_stream.getline(&str[0], 512);
    m_column++;
    return str;
}

char LocatableStream::peek() {
    return m_stream.peek();
}

char LocatableStream::peek_twice() {
    m_stream.get();
    char c = m_stream.peek();
    m_stream.unget();
    return c;
}

std::string LocatableStream::read(int length) {
    std::string str(length, '\0');
    for (int i = 0; i < length; i++) {
        str.append(1, get());
    }
    return str;
}

std::string LocatableStream::peek_forward(int length) {
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
