#include "config.h"

#include <sstream>
#include <toml.hpp>

namespace asyncio_dat {

Config parse_config_text(const std::string& toml_text)
{
    Config cfg;

    std::istringstream is(toml_text);
    const auto data = toml::parse(is, "config.toml");

    // [main].paths
    if (data.contains("main")) {
        const auto& main = toml::find(data, "main");
        if (main.contains("paths")) {
            cfg.paths = toml::find<std::vector<std::string>>(main, "paths");
        }
    }

    // [asyncio].callback_module_path
    if (data.contains("asyncio")) {
        const auto& asyncio = toml::find(data, "asyncio");
        if (asyncio.contains("callback_module_path")) {
            cfg.callback_path = toml::find<std::string>(asyncio, "callback_module_path");
        }
    }

    return cfg;
}

} // namespace asyncio_dat
