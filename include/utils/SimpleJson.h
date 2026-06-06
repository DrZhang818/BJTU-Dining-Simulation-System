#pragma once

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace bdss::utils {

class JsonError : public std::runtime_error {
public:
    explicit JsonError(const std::string& message) : std::runtime_error(message) {}
};

class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool value);
    JsonValue(double value);
    JsonValue(int value);
    JsonValue(std::string value);
    JsonValue(const char* value);
    JsonValue(Array value);
    JsonValue(Object value);

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool asBool(bool fallback = false) const;
    double asNumber(double fallback = 0.0) const;
    int asInt(int fallback = 0) const;
    const std::string& asString() const;
    std::string asStringOr(std::string fallback) const;
    const Array& asArray() const;
    const Object& asObject() const;

    bool contains(const std::string& key) const;
    const JsonValue& at(const std::string& key) const;
    const JsonValue* find(const std::string& key) const;

    static JsonValue parse(const std::string& text);
    std::string dump(int indent = 2) const;

private:
    Storage storage_;
};

} // namespace bdss::utils
