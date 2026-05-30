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

TEST_CASE("comments and blank lines are ignored", "[config]")
{
    const auto cfg = parse_config_text(R"(
# a leading comment

[main]
paths = ["m"]   # trailing comment

[asyncio]
callback_module_path = "cb.py"  # another
)");
    REQUIRE(cfg.paths.size() == 1);
    CHECK(cfg.paths[0] == "m");
    REQUIRE(cfg.callback_path.has_value());
    CHECK(*cfg.callback_path == "cb.py");
}

TEST_CASE("empty paths array yields no paths", "[config]")
{
    const auto cfg = parse_config_text("[main]\npaths = []\n");
    CHECK(cfg.paths.empty());
    CHECK_FALSE(cfg.callback_path.has_value());
}

TEST_CASE("[main] without paths key yields no paths", "[config]")
{
    const auto cfg = parse_config_text("[main]\nother = 1\n");
    CHECK(cfg.paths.empty());
}

TEST_CASE("empty-string path elements are preserved", "[config]")
{
    const auto cfg = parse_config_text(R"(
[main]
paths = ["", "x"]
)");
    REQUIRE(cfg.paths.size() == 2);
    CHECK(cfg.paths[0].empty());
    CHECK(cfg.paths[1] == "x");
}

TEST_CASE("the README example config parses", "[config]")
{
    const auto cfg = parse_config_text(R"(
[main]
paths = [
    "path/to/your/modules",
    "another/path/with spaces",
    ".venv/Lib/site-packages",
]

[asyncio]
callback_module_path = "path/to/your/callback.py"
)");
    REQUIRE(cfg.paths.size() == 3);
    CHECK(cfg.paths[0] == "path/to/your/modules");
    CHECK(cfg.paths[1] == "another/path/with spaces");
    CHECK(cfg.paths[2] == ".venv/Lib/site-packages");
    REQUIRE(cfg.callback_path.has_value());
    CHECK(*cfg.callback_path == "path/to/your/callback.py");
}

TEST_CASE("malformed TOML throws", "[config]")
{
    CHECK_THROWS_AS(parse_config_text("not = = valid"), std::exception);
}

TEST_CASE("wrong type for paths throws", "[config]")
{
    CHECK_THROWS_AS(parse_config_text("[main]\npaths = 5\n"), std::exception);
}

TEST_CASE("mixed-type paths array throws", "[config]")
{
    CHECK_THROWS_AS(parse_config_text("[main]\npaths = [\"a\", 5]\n"), std::exception);
}

TEST_CASE("wrong type for callback_module_path throws", "[config]")
{
    CHECK_THROWS_AS(parse_config_text("[asyncio]\ncallback_module_path = 42\n"), std::exception);
}

TEST_CASE("duplicate keys throw", "[config]")
{
    CHECK_THROWS_AS(
        parse_config_text("[asyncio]\ncallback_module_path = \"a\"\ncallback_module_path = \"b\"\n"),
        std::exception);
}
