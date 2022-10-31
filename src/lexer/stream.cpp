
LocatableStream(std::string filename) : m_filename(filename) {
    m_stream = std::ifstream(filename);

    m_line = 1;
    m_column = 1;
}

char LocatableStream::get() {
    char c = m_stream.get();
    if (c == '\n') {
        m_column = 1;
        m_line++;
    } else {
        m_column++;
    }
}

char LocatableStream::peek() {
    return m_stream.peek();
}

Locatable LocatableStream::loc() {
    return Locatable(m_filename, m_line, m_column);
}
