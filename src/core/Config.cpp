#include "core/Config.h"

#include <format>
#include <fstream>

namespace bdss::core {

std::expected<Config, std::string> Config::loadFromFile(const std::filesystem::path& filepath) {
    if (!std::filesystem::exists(filepath)) {
        return std::unexpected(
            std::format("配置文件加载失败：找不到文件路径'{}'", filepath.string()));
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        return std::unexpected(
            std::format("配置文件加载失败：无法读取文件'{}'", filepath.string()));
    }

    // TODO：第三方 JSON 库，解析配置文件内容并填充 Config 结构体

    Config defaultConfig;
    // 目前先返回默认配置，后续实现文件解析后再返回实际配置

    return defaultConfig;
}

}  // namespace bdss::core