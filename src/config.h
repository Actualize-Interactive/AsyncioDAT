#pragma once

#include <optional>
#include <string>
#include <vector>

// Pure configuration parsing for AsyncioDAT, kept free of any Python or
// TouchDesigner dependencies so it can be unit-tested on its own (tests/cpp).
namespace asyncio_dat {

struct Config {
    // Paths to prepend to sys.path ([main].paths). Empty if unspecified.
    std::vector<std::string> paths;
    // Callback module path ([asyncio].callback_module_path). Unset if absent.
    std::optional<std::string> callback_path;
};

// Parse AsyncioDAT TOML configuration from in-memory text.
// Throws toml11's parse/type exceptions (derived from std::exception) on
// malformed input or wrong value types.
Config parse_config_text(const std::string& toml_text);

} // namespace asyncio_dat
