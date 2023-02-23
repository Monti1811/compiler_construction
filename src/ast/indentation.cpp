#include "indentation.h"

int IndentManager::getCurrIndentation() {
    return this->currIndent;
}

void IndentManager::setCurrIndentation(int value) {
    this->currIndent = value;
}

void IndentManager::increaseCurrIndentation(int value) {
    this->currIndent += value;
}

void IndentManager::decreaseCurrIndentation(int value) {
    this->currIndent = std::max(this->currIndent - value, 0);
}

void IndentManager::printCurrIndentation(std::ostream& stream) {
    for (int i = 0; i < this->currIndent; i++)
        stream << '\t';
}
