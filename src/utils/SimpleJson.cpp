#include "utils/SimpleJson.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>

namespace bdss::utils {
namespace {

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    JsonValue parseDocument() {
        skipWs();
        JsonValue value = parseValue();
        skipWs();
        if (!eof()) {
            fail("unexpected trailing characters");
        }
        return value;
    }

private:
    bool eof() const { return pos_ >= text_.size(); }

    char peek() const {
        if (eof()) {
            return '\0';
        }
        return text_[pos_];
    }

    char get() {
        if (eof()) {
            fail("unexpected end of input");
        }
        return text_[pos_++];
    }

    void skipWs() {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
            ++pos_;
        }
    }

    void expect(char expected) {
        const char actual = get();
        if (actual != expected) {
            std::ostringstream oss;
            oss << "expected '" << expected << "' but found '" << actual << "'";
            fail(oss.str());
        }
    }

    bool consume(char c) {
        if (!eof() && peek() == c) {
            ++pos_;
            return true;
        }
        return false;
    }

    bool consumeLiteral(const char* literal) {
        std::size_t start = pos_;
        for (const char* p = literal; *p != '\0'; ++p) {
            if (eof() || get() != *p) {
                pos_ = start;
                return false;
            }
        }
        return true;
    }

    JsonValue parseValue() {
        skipWs();
        if (eof()) {
            fail("expected JSON value");
        }
        switch (peek()) {
            case 'n':
                if (consumeLiteral("null")) {
                    return JsonValue(nullptr);
                }
                break;
            case 't':
                if (consumeLiteral("true")) {
                    return JsonValue(true);
                }
                break;
            case 'f':
                if (consumeLiteral("false")) {
                    return JsonValue(false);
                }
                break;
            case '"':
                return JsonValue(parseString());
            case '[':
                return JsonValue(parseArray());
            case '{':
                return JsonValue(parseObject());
            default:
                if (peek() == '-' || std::isdigit(static_cast<unsigned char>(peek()))) {
                    return JsonValue(parseNumber());
                }
                break;
        }
        fail("invalid JSON value");
        return JsonValue();
    }

    static void appendUtf8(std::string& out, unsigned int codepoint) {
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    unsigned int parseHex4() {
        unsigned int cp = 0;
        for (int i = 0; i < 4; ++i) {
            if (eof()) {
                fail("incomplete unicode escape");
            }
            const char c = get();
            cp <<= 4;
            if (c >= '0' && c <= '9') {
                cp += static_cast<unsigned int>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                cp += static_cast<unsigned int>(10 + c - 'a');
            } else if (c >= 'A' && c <= 'F') {
                cp += static_cast<unsigned int>(10 + c - 'A');
            } else {
                fail("invalid unicode escape");
            }
        }
        return cp;
    }

    std::string parseString() {
        expect('"');
        std::string out;
        while (!eof()) {
            const char c = get();
            if (c == '"') {
                return out;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                fail("control character in string");
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (eof()) {
                fail("incomplete escape sequence");
            }
            const char esc = get();
            switch (esc) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': appendUtf8(out, parseHex4()); break;
                default: fail("invalid escape sequence");
            }
        }
        fail("unterminated string");
        return out;
    }

    double parseNumber() {
        const std::size_t start = pos_;
        consume('-');
        if (consume('0')) {
            // Leading zero is valid only as a single integer digit.
        } else {
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                fail("invalid number");
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
            }
        }
        if (consume('.')) {
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                fail("invalid fractional part");
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            ++pos_;
            if (peek() == '+' || peek() == '-') {
                ++pos_;
            }
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                fail("invalid exponent");
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
            }
        }
        try {
            return std::stod(text_.substr(start, pos_ - start));
        } catch (const std::exception&) {
            fail("number is out of range");
        }
        return 0.0;
    }

    JsonValue::Array parseArray() {
        expect('[');
        skipWs();
        JsonValue::Array array;
        if (consume(']')) {
            return array;
        }
        while (true) {
            array.push_back(parseValue());
            skipWs();
            if (consume(']')) {
                return array;
            }
            expect(',');
            skipWs();
        }
    }

    JsonValue::Object parseObject() {
        expect('{');
        skipWs();
        JsonValue::Object object;
        if (consume('}')) {
            return object;
        }
        while (true) {
            skipWs();
            if (peek() != '"') {
                fail("object keys must be strings");
            }
            std::string key = parseString();
            skipWs();
            expect(':');
            JsonValue value = parseValue();
            object[std::move(key)] = std::move(value);
            skipWs();
            if (consume('}')) {
                return object;
            }
            expect(',');
        }
    }

    [[noreturn]] void fail(const std::string& message) const {
        std::ostringstream oss;
        oss << "JSON parse error at byte " << pos_ << ": " << message;
        throw JsonError(oss.str());
    }

    const std::string& text_;
    std::size_t pos_ = 0;
};

std::string escapeString(const std::string& input) {
    std::ostringstream oss;
    oss << '"';
    for (const unsigned char c : input) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << static_cast<char>(c);
                }
        }
    }
    oss << '"';
    return oss.str();
}

void dumpValue(const JsonValue& value, std::ostringstream& oss, int indent, int level) {
    const std::string pad(static_cast<std::size_t>(level * indent), ' ');
    const std::string nextPad(static_cast<std::size_t>((level + 1) * indent), ' ');

    if (value.isNull()) {
        oss << "null";
    } else if (value.isBool()) {
        oss << (value.asBool() ? "true" : "false");
    } else if (value.isNumber()) {
        oss << std::setprecision(12) << value.asNumber();
    } else if (value.isString()) {
        oss << escapeString(value.asString());
    } else if (value.isArray()) {
        const auto& array = value.asArray();
        if (array.empty()) {
            oss << "[]";
            return;
        }
        oss << "[\n";
        for (std::size_t i = 0; i < array.size(); ++i) {
            oss << nextPad;
            dumpValue(array[i], oss, indent, level + 1);
            if (i + 1 < array.size()) {
                oss << ',';
            }
            oss << '\n';
        }
        oss << pad << ']';
    } else {
        const auto& object = value.asObject();
        if (object.empty()) {
            oss << "{}";
            return;
        }
        oss << "{\n";
        std::size_t index = 0;
        for (const auto& [key, member] : object) {
            oss << nextPad << escapeString(key) << ": ";
            dumpValue(member, oss, indent, level + 1);
            if (++index < object.size()) {
                oss << ',';
            }
            oss << '\n';
        }
        oss << pad << '}';
    }
}

} // namespace

JsonValue::JsonValue() : storage_(nullptr) {}
JsonValue::JsonValue(std::nullptr_t) : storage_(nullptr) {}
JsonValue::JsonValue(bool value) : storage_(value) {}
JsonValue::JsonValue(double value) : storage_(value) {}
JsonValue::JsonValue(int value) : storage_(static_cast<double>(value)) {}
JsonValue::JsonValue(std::string value) : storage_(std::move(value)) {}
JsonValue::JsonValue(const char* value) : storage_(std::string(value)) {}
JsonValue::JsonValue(Array value) : storage_(std::move(value)) {}
JsonValue::JsonValue(Object value) : storage_(std::move(value)) {}

bool JsonValue::isNull() const { return std::holds_alternative<std::nullptr_t>(storage_); }
bool JsonValue::isBool() const { return std::holds_alternative<bool>(storage_); }
bool JsonValue::isNumber() const { return std::holds_alternative<double>(storage_); }
bool JsonValue::isString() const { return std::holds_alternative<std::string>(storage_); }
bool JsonValue::isArray() const { return std::holds_alternative<Array>(storage_); }
bool JsonValue::isObject() const { return std::holds_alternative<Object>(storage_); }

bool JsonValue::asBool(bool fallback) const {
    return isBool() ? std::get<bool>(storage_) : fallback;
}

double JsonValue::asNumber(double fallback) const {
    return isNumber() ? std::get<double>(storage_) : fallback;
}

int JsonValue::asInt(int fallback) const {
    if (!isNumber()) {
        return fallback;
    }
    const double value = std::get<double>(storage_);
    if (value > static_cast<double>(std::numeric_limits<int>::max()) ||
        value < static_cast<double>(std::numeric_limits<int>::min())) {
        return fallback;
    }
    return static_cast<int>(value);
}

const std::string& JsonValue::asString() const {
    if (!isString()) {
        throw JsonError("JSON value is not a string");
    }
    return std::get<std::string>(storage_);
}

std::string JsonValue::asStringOr(std::string fallback) const {
    return isString() ? std::get<std::string>(storage_) : std::move(fallback);
}

const JsonValue::Array& JsonValue::asArray() const {
    if (!isArray()) {
        throw JsonError("JSON value is not an array");
    }
    return std::get<Array>(storage_);
}

const JsonValue::Object& JsonValue::asObject() const {
    if (!isObject()) {
        throw JsonError("JSON value is not an object");
    }
    return std::get<Object>(storage_);
}

bool JsonValue::contains(const std::string& key) const {
    if (!isObject()) {
        return false;
    }
    const auto& object = std::get<Object>(storage_);
    return object.find(key) != object.end();
}

const JsonValue& JsonValue::at(const std::string& key) const {
    const JsonValue* value = find(key);
    if (!value) {
        throw JsonError("missing object key: " + key);
    }
    return *value;
}

const JsonValue* JsonValue::find(const std::string& key) const {
    if (!isObject()) {
        return nullptr;
    }
    const auto& object = std::get<Object>(storage_);
    const auto it = object.find(key);
    return it == object.end() ? nullptr : &it->second;
}

JsonValue JsonValue::parse(const std::string& text) {
    return Parser(text).parseDocument();
}

std::string JsonValue::dump(int indent) const {
    std::ostringstream oss;
    dumpValue(*this, oss, std::max(0, indent), 0);
    return oss.str();
}

} // namespace bdss::utils
