#include "stream.h"

unsigned char LocatableStream::getOrEof() {
    unsigned char c = this->_stream.get();

    if (this->_stream.eof()) {
        return '\0';
    }

    return c;
}

unsigned char LocatableStream::get() {
    char c = getOrEof();

    if (c == '\n') {
        this->_column = 1;
        this->_line++;
    } else if (c == '\r') {
        if (this->_stream.peek() == '\n') {
            this->_stream.get();
        }
        c = '\n';
        this->_column = 1;
        this->_line++;
    } else {
        this->_column++;
    }

    return c;
}

std::string LocatableStream::getStr(size_t length) {
    std::string str(length, '\0');
    for (size_t i = 0; i < length; i++) {
        str[i] = get();
    }
    return str;
}

std::string LocatableStream::getLine() {
    std::string str;
    char c = get();
    while (c != '\n' && c != '\0') {
        str += c;
        c = get();
    }
    return str;
}

unsigned char LocatableStream::peek(size_t offset) {
    for (size_t i = 0; i < offset; i++) {
        getOrEof();
    }
    char c = getOrEof();
    for (size_t i = 0; i <= offset; i++) {
        this->_stream.unget();
    }
    return c;
}

std::string LocatableStream::peekStr(size_t length) {
    std::string str(length, '\0');
    this->_stream.read(&str[0], length);
    for (size_t i = 0; i < length; i++) {
        this->_stream.unget();
    }
    return str;
}

Locatable LocatableStream::loc() {
    return Locatable(this->_file_name, this->_line, this->_column);
}
