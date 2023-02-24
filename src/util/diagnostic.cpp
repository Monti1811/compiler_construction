#include "diagnostic.h"

Locatable::Locatable(const std::string& file_name, size_t line, size_t column)
    : file_name(file_name)
    , line(line)
    , column(column) {}

Locatable::Locatable(const Locatable& loc)
    : file_name(loc.file_name)
    , line(loc.line)
    , column(loc.column) {}

Locatable::~Locatable(void) {}

Locatable& Locatable::operator=(const Locatable& loc) {
    this->line = loc.line;
    this->column = loc.column;
    return *this;
}

std::ostream& operator<<(std::ostream& stream, const Locatable& loc) {
    stream << loc.file_name << ':' << loc.line << ':' << loc.column << ":";
    return stream;
}
