#include "arithmetic_steps.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#ifndef BOOST_ALL_NO_LIB
#define BOOST_ALL_NO_LIB
#endif
#include <boost/json.hpp>

namespace mathsdk
{

namespace
{


std::int64_t checked_mul(std::int64_t a, std::int64_t b)
{
    if (a != 0 && b != 0) {
        const auto result = a * b;
        if (result / a != b) {
            throw ArithmeticStepsError(
                "Intermediate value too large for exact arithmetic in this version"
            );
        }
        return result;
    }
    return 0;
}

std::int64_t checked_add(std::int64_t a, std::int64_t b)
{
    const auto result = a + b;
    if (((a < 0) == (b < 0)) && ((result < 0) != (a < 0))) {
        throw ArithmeticStepsError(
            "Intermediate value too large for exact arithmetic in this version"
        );
    }
    return result;
}

struct Fraction {
    std::int64_t num = 0;
    std::int64_t den = 1;

    static std::int64_t gcd(std::int64_t a, std::int64_t b)
    {
        if (a < 0) a = -a;
        if (b < 0) b = -b;
        while (b != 0) {
            const auto t = b;
            b = a % b;
            a = t;
        }
        return a == 0 ? 1 : a;
    }

    static Fraction of(std::int64_t n, std::int64_t d = 1)
    {
        if (d == 0) {
            throw ArithmeticStepsError("Division by zero");
        }
        if (d < 0) {
            n = -n;
            d = -d;
        }
        const auto g = gcd(n, d);
        return Fraction{n / g, d / g};
    }

    std::string str() const
    {
        if (den == 1) {
            return std::to_string(num);
        }
        return std::to_string(num) + "/" + std::to_string(den);
    }

    bool is_integer() const { return den == 1; }
};

Fraction operator+(const Fraction &a, const Fraction &b)
{
    return Fraction::of(
        checked_add(checked_mul(a.num, b.den), checked_mul(b.num, a.den)),
        checked_mul(a.den, b.den)
    );
}

Fraction operator-(const Fraction &a, const Fraction &b)
{
    return Fraction::of(
        checked_add(checked_mul(a.num, b.den), checked_mul(-b.num, a.den)),
        checked_mul(a.den, b.den)
    );
}

Fraction operator*(const Fraction &a, const Fraction &b)
{
    return Fraction::of(checked_mul(a.num, b.num), checked_mul(a.den, b.den));
}

Fraction operator/(const Fraction &a, const Fraction &b)
{
    if (b.num == 0) {
        throw ArithmeticStepsError("Division by zero");
    }
    return Fraction::of(checked_mul(a.num, b.den), checked_mul(a.den, b.num));
}

Fraction pow_fraction(const Fraction &base, std::int64_t exponent)
{
    if (exponent < 0) {
        if (base.num == 0) {
            throw ArithmeticStepsError("Division by zero");
        }
        return pow_fraction(Fraction::of(base.den, base.num), -exponent);
    }
    Fraction result = Fraction::of(1);
    for (std::int64_t i = 0; i < exponent; ++i) {
        result = result * base;
    }
    return result;
}

Fraction sqrt_fraction(const Fraction &value)
{
    if (value.num < 0) {
        throw ArithmeticStepsError("Expression is undefined");
    }
    const auto root_of = [](std::int64_t v) -> std::int64_t {
        const auto r = static_cast<std::int64_t>(std::llround(std::sqrt(static_cast<double>(v))));
        for (auto candidate : {r - 1, r, r + 1}) {
            if (candidate >= 0 && candidate * candidate == v) {
                return candidate;
            }
        }
        throw ArithmeticStepsError("sqrt of a non-perfect square is not supported yet");
    };
    return Fraction::of(root_of(value.num), root_of(value.den));
}

enum class NodeKind { Number, Negate, Sqrt, Add, Sub, Mul, Div, Pow };

struct Node {
    NodeKind kind;
    Fraction value;                 
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;     
};

using NodePtr = std::unique_ptr<Node>;

NodePtr make_number(Fraction value)
{
    auto node = std::make_unique<Node>();
    node->kind = NodeKind::Number;
    node->value = value;
    return node;
}

NodePtr make_unary(NodeKind kind, NodePtr operand)
{
    auto node = std::make_unique<Node>();
    node->kind = kind;
    node->left = std::move(operand);
    return node;
}

NodePtr make_binary(NodeKind kind, NodePtr left, NodePtr right)
{
    auto node = std::make_unique<Node>();
    node->kind = kind;
    node->left = std::move(left);
    node->right = std::move(right);
    return node;
}

class Parser {
public:
    explicit Parser(const std::string &input) : input_(input) {}

    NodePtr parse()
    {
        skip_spaces();
        auto node = parse_expr();
        skip_spaces();
        if (pos_ != input_.size()) {
            throw ArithmeticStepsError("Unexpected character at position " + std::to_string(pos_));
        }
        return node;
    }

private:
    const std::string &input_;
    std::size_t pos_ = 0;

    void skip_spaces()
    {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }

    char peek()
    {
        skip_spaces();
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }

    char advance() { return input_[pos_++]; }

    bool match_word(const std::string &word)
    {
        skip_spaces();
        if (input_.compare(pos_, word.size(), word) == 0) {
            pos_ += word.size();
            return true;
        }
        return false;
    }

    NodePtr parse_expr()
    {
        auto left = parse_term();
        for (;;) {
            const auto c = peek();
            if (c == '+') {
                advance();
                left = make_binary(NodeKind::Add, std::move(left), parse_term());
            } else if (c == '-') {
                advance();
                left = make_binary(NodeKind::Sub, std::move(left), parse_term());
            } else {
                return left;
            }
        }
    }

    NodePtr parse_term()
    {
        auto left = parse_power();
        for (;;) {
            const auto c = peek();
            if (c == '*') {
                advance();
                left = make_binary(NodeKind::Mul, std::move(left), parse_power());
            } else if (c == '/') {
                advance();
                left = make_binary(NodeKind::Div, std::move(left), parse_power());
            } else {
                return left;
            }
        }
    }

    NodePtr parse_power()
    {
        auto base = parse_unary();
        if (peek() == '^') {
            advance();
            auto exponent = parse_unary();
            return make_binary(NodeKind::Pow, std::move(base), std::move(exponent));
        }
        return base;
    }

    NodePtr parse_unary()
    {
        if (peek() == '-') {
            advance();
            return make_unary(NodeKind::Negate, parse_unary());
        }
        return parse_primary();
    }

    NodePtr parse_primary()
    {
        skip_spaces();
        if (peek() == '(') {
            advance();
            auto node = parse_expr();
            if (peek() != ')') {
                throw ArithmeticStepsError("Missing closing parenthesis");
            }
            advance();
            return node;
        }
        if (match_word("sqrt")) {
            skip_spaces();
            if (peek() != '(') {
                throw ArithmeticStepsError("Expected '(' after sqrt");
            }
            advance();
            auto node = parse_expr();
            if (peek() != ')') {
                throw ArithmeticStepsError("Missing closing parenthesis");
            }
            advance();
            return make_unary(NodeKind::Sqrt, std::move(node));
        }
        skip_spaces();
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            throw ArithmeticStepsError("Expected a number at position " + std::to_string(pos_));
        }
        const auto start = pos_;
        while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
        const auto text = input_.substr(start, pos_ - start);
        return make_number(Fraction::of(std::stoll(text)));
    }
};


std::string node_to_string(const Node &node);

std::string binary_symbol(NodeKind kind)
{
    switch (kind) {
    case NodeKind::Add: return "+";
    case NodeKind::Sub: return "-";
    case NodeKind::Mul: return "*";
    case NodeKind::Div: return "/";
    case NodeKind::Pow: return "^";
    default: return "?";
    }
}

std::string node_to_string(const Node &node)
{
    switch (node.kind) {
    case NodeKind::Number:
        return node.value.str();
    case NodeKind::Negate:
        return "-" + node_to_string(*node.left);
    case NodeKind::Sqrt:
        return "sqrt(" + node_to_string(*node.left) + ")";
    default:
        return node_to_string(*node.left) + " " + binary_symbol(node.kind)
            + " " + node_to_string(*node.right);
    }
}

const char *rule_name(NodeKind kind)
{
    switch (kind) {
    case NodeKind::Negate: return "negate";
    case NodeKind::Sqrt: return "square_root";
    case NodeKind::Add: return "add";
    case NodeKind::Sub: return "subtract";
    case NodeKind::Mul: return "multiply";
    case NodeKind::Div: return "divide";
    case NodeKind::Pow: return "power";
    default: return "reduce";
    }
}

class Reducer {
public:
    boost::json::array steps;

    Fraction reduce(const Node &node)
    {
        if (node.kind == NodeKind::Number) {
            return node.value;
        }

        if (node.kind == NodeKind::Negate || node.kind == NodeKind::Sqrt) {
            const auto operand = reduce(*node.left);
            const auto before = describe_unary(node.kind, operand);
            const auto result = node.kind == NodeKind::Negate
                ? Fraction::of(-operand.num, operand.den)
                : sqrt_fraction(operand);
            record_step(before, result.str(), rule_name(node.kind));
            return result;
        }

        const auto left = reduce(*node.left);
        const auto right = reduce(*node.right);
        const auto before = left.str() + " " + binary_symbol(node.kind) + " " + right.str();
        const auto result = apply_binary(node.kind, left, right);
        record_step(before, result.str(), rule_name(node.kind));
        return result;
    }

private:
    static std::string describe_unary(NodeKind kind, const Fraction &operand)
    {
        return kind == NodeKind::Negate
            ? "-" + operand.str()
            : "sqrt(" + operand.str() + ")";
    }

    static Fraction apply_binary(NodeKind kind, const Fraction &left, const Fraction &right)
    {
        switch (kind) {
        case NodeKind::Add: return left + right;
        case NodeKind::Sub: return left - right;
        case NodeKind::Mul: return left * right;
        case NodeKind::Div: return left / right;
        case NodeKind::Pow:
            if (!right.is_integer()) {
                throw ArithmeticStepsError("Non-integer exponents are not supported yet");
            }
            return pow_fraction(left, right.num);
        default:
            throw ArithmeticStepsError("Unsupported operation");
        }
    }

    void record_step(const std::string &before, const std::string &after, const char *rule)
    {
     
        if (before == after) {
            return;
        }
        boost::json::object step;
        step["before"] = before;
        step["after"] = after;
        step["rule"] = rule;
        steps.push_back(std::move(step));
    }
};

} // namespace

std::string arithmetic_steps_to_json(
    const std::string &expression,
    std::string &out_final_result
)
{
    Parser parser(expression);
    const auto ast = parser.parse();

    Reducer reducer;
    const auto final_value = reducer.reduce(*ast);
    out_final_result = final_value.str();

    if (reducer.steps.empty()) {
        boost::json::object step;
        step["before"] = expression;
        step["after"] = final_value.str();
        step["rule"] = "identity";
        reducer.steps.push_back(std::move(step));
    }

    return boost::json::serialize(reducer.steps);
}

} // namespace mathsdk
