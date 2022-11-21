#include "expression.h"
 
std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Expression>& expr) {
    expr->print(stream);
    return stream;
}

void IdentExpression::print(std::ostream& stream) {
    stream << (*this->_ident);
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
    stream << '(';
    this->_expression->print(stream);
    stream << '[';
    this->_index->print(stream),
    stream << "])";
}

void CallExpression::print(std::ostream& stream) {
    stream << '(';
    this->_expression->print(stream);
    stream << '(';
    const int length = this->_arguments.size();
    for (int i = 0; i < length; i++) {
        this->_arguments.at(i)->print(stream);
        if (i != length - 1) {
            stream << ',';
        }
    }
    stream << "))";
}

void DotExpression::print(std::ostream& stream) {
    stream << '(';
    this->_expression->print(stream);
    stream << '.';
    this->_ident->print(stream);
    stream << ')';
}

void ArrowExpression::print(std::ostream& stream) {
    stream << '(';
    this->_expression->print(stream);
    stream << "->";
    this->_ident->print(stream);
    stream << ')';
}

void SizeofExpression::print(std::ostream& stream) {
    stream << '(';
    stream << "sizeof ";
    this->_inner->print(stream);
    stream << ')';
}

void SizeofTypeExpression::print(std::ostream& stream) {
    stream << '(';
    stream << "sizeof ";
    this->_type->print(stream);
    stream << ')';
}

void ReferenceExpression::print(std::ostream& stream) {
    stream << "&(" << this->_inner << ")";
}

void PointerExpression::print(std::ostream& stream) {
    stream << "*(" << this->_inner << ")";
}

void NegationExpression::print(std::ostream& stream) {
    stream << "-(" << this->_inner << ")";
}

void LogicalNegationExpression::print(std::ostream& stream) {
    stream << "!(" << this->_inner << ")";
}

void MultiplyExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " * " << this->_right << ")";
}

void AddExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " + " << this->_right << ")";
}

void SubstractExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " - " << this->_right << ")";
}

void LessThanExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " < " << this->_right << ")";
}

void EqualExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " == " << this->_right << ")";
}

void UnequalExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " != " << this->_right << ")";
}

void AndExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " && " << this->_right << ")";
}

void OrExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " || " << this->_right << ")";
}

void TernaryExpression::print(std::ostream& stream) {
    stream << "(" << this->_condition << " ? " << this->_left << " : " << this->_right << ")";
}

void AssignExpression::print(std::ostream& stream) {
    stream << "(" << this->_left << " = " << this->_right << ")";
}
