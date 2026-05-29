#include <catch2/catch_test_macros.hpp>

#include "config.h"

using asyncio_dat::parse_config_text;

TEST_CASE("empty config yields no paths and no callback", "[config]")
{
    const auto cfg = parse_config_text("");
    CHECK(cfg.paths.empty());
    CHECK_FALSE(cfg.callback_path.has_value());
}

TEST_CASE("paths are read from [main]", "[config]")
{
    const auto cfg = parse_config_text(R"(
[main]
paths = ["a", "b/c", "with space"]
)");
    REQUIRE(cfg.paths.size() == 3);
    CHECK(cfg.paths[0] == "a");
    CHECK(cfg.paths[1] == "b/c");
    CHECK(cfg.paths[2] == "with space");
    CHECK_FALSE(cfg.callback_path.has_value());
}

TEST_CASE("callback_module_path is read from [asyncio]", "[config]")
{
    const auto cfg = parse_config_text(R"(
[asyncio]
callback_module_path = "cb.py"
)");
    CHECK(cfg.paths.empty());
    REQUIRE(cfg.callback_path.has_value());
    CHECK(*cfg.callback_path == "cb.py");
}

TEST_CASE("both sections parse together", "[config]")
{
    const auto cfg = parse_config_text(R"(
[main]
paths = ["modules"]

[asyncio]
callback_module_path = "asyncio_dat_callbacks.py"
)");
    REQUIRE(cfg.paths.size() == 1);
    CHECK(cfg.paths[0] == "modules");
    REQUIRE(cfg.callback_path.has_value());
    CHECK(*cfg.callback_path == "asyncio_dat_callbacks.py");
}

TEST_CASE("unrelated sections are ignored", "[config]")
{
    const auto cfg = parse_config_text("[other]\nkey = 1\n");
    CHECK(cfg.paths.empty());
    CHECK_FALSE(cfg.callback_path.has_value());
}

TEST_CASE("malformed TOML throws", "[config]")
{
    CHECK_THROWS_AS(parse_config_text("not = = valid"), std::exception);
}

TEST_CASE("wrong type for paths throws", "[config]")
{
    CHECK_THROWS_AS(parse_config_text("[main]\npaths = 5\n"), std::exception);
}
