#include "statement.h"

std::ostream& operator<<(std::ostream& stream, const StatementPtr& stat) {
    stat->print(stream);
    return stream;
}
