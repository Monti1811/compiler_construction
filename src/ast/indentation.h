#pragma once

#include <iostream>

// Class implemented as singleton pattern to get the current identation to print the AST correctly
class IndentManager {
  public:
    static IndentManager& getInstance() {
        static IndentManager instance;
        return instance;
    }

    friend std::ostream& operator<<(std::ostream& stream, const IndentManager& ident);

    int getCurrIndentation();
    void setCurrIndentation(int value);
    void increaseCurrIndentation(int value);
    void decreaseCurrIndentation(int value);
    void printCurrIndentation(std::ostream& stream);

    int currIndent = 0;

  private:
    IndentManager() {}
    IndentManager(IndentManager const&);
    IndentManager& operator=(const IndentManager&);
};
