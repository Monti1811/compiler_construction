#include "expression.h"
 
std::ostream& operator<<(std::ostream& stream, const ExpressionPtr& expr) {
    expr->print(stream);
    return stream;
}

void IdentExpression::print(std::ostream& stream) {
    stream << (*this->_ident);
}

std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<IdentExpression>& expr) {
    expr->print(stream);
    return stream;
}

void IntConstantExpression::print(std::ostream& stream) {
    stream << this->_value;
}

void CharConstantExpression::print(std::ostream& stream) {
    stream << this->_value;
}

void StringLiteralExpression::print(std::ostream& stream) {
    stream << this->_value;
}

void IndexExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << '[' << this->_index << "])";
}

void CallExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << '(';

    const int length = this->_arguments.size();
    for (int i = 0; i < length; i++) {
        this->_arguments.at(i)->print(stream);
        if (i != length - 1) {
            stream << ", ";
        }
    }

    stream << "))";
}

void DotExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << '.' << this->_ident << ')';
}

void ArrowExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << "->" << this->_ident << ')';
}

void UnaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_op_str << this->_inner << ')';
}

void SizeofTypeExpression::print(std::ostream& stream) {
    stream << "(sizeof(" << this->_type << "))";
}

void BinaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_left << ' ' << this->_op_str << ' ' << this->_right << ')';
}

void TernaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_condition << " ? " << this->_left << " : " << this->_right << ')';
}
