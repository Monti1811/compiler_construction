#include <fstream>
#include <string>
#include "diagnostic.h"

struct LocatableStream {
    public:
    LocatableStream(std::string filename);

    char get();
    char peek();

    Locatable loc();
    
    private:
    std::ifstream m_stream;

    std::string m_filename;
    size_t m_line;
    size_t m_column;
};
