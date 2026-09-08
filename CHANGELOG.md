# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
While the major version is `0`, breaking changes may land in a minor release.

## [Unreleased]

## [0.4.1] - 2026-09-08

Fixes the macOS release build, which TouchDesigner reported as corrupted.

### Fixed

- The v0.4.0 `AsyncioDAT.plugin` failed to load on every Mac. The release
  runner is macOS 26 on Apple silicon, and nothing in the build pinned a
  deployment target or an architecture, so the bundle inherited the runner's
  macOS 26 as its minimum OS and arm64 as its only architecture. TouchDesigner
  itself runs on macOS 13 and later, universal. The plugin is now built for
  `arm64` and `x86_64` with a minimum of macOS 13.3 (the earliest libc++ that
  provides the `std::format` the status messages use).
- The bundle carried only the linker's implicit ad-hoc signature, which covers
  the Mach-O but not the bundle, so `codesign --verify` rejected it. The whole
  bundle is now ad-hoc signed after linking. It is still not notarized, so a
  downloaded copy still needs `xattr -dr com.apple.quarantine` (see the README
  install steps).
- CI and the release gate now verify the macOS plugin links no Python library,
  is universal, has the expected minimum OS, and passes `codesign --verify`, so
  this class of binary cannot be published again.

### Changed

- README: the macOS build is universal, not arm64 only, and requires macOS 13.3
  or newer.

## [0.4.0] - 2026-07-26

The release that opens the repository to the public. One crash fix, and the
supporting material a public project needs: a changelog, issue and pull request
templates, and a release pipeline that will not publish a build it has not
tested.

### Fixed

- A crash when the event loop failed to initialize. `execute()` reported the
  failure by calling `strcmp` on `m_warning`, but only one of the seven failure
  paths in `initializeAsyncio()` sets it — the rest set `m_error` and leave
  `m_warning` null. So any real failure (asyncio failing to import, the loop
  failing to construct) dereferenced a null pointer inside TouchDesigner
  instead of showing the error. The remaining failure is now surfaced through
  the status table, null-checked, and still deduplicated across frames.
- The macOS bundle's `CFBundleName` was empty: `Info.plist.in` substitutes
  `MACOSX_BUNDLE_BUNDLE_NAME`, which CMake was never given.
- The project version was `1.0.0` while releases were tagged `v0.x`, so the
  macOS bundle reported a version that did not correspond to any release. It
  now tracks the tag.

### Added

- `CHANGELOG.md`, issue forms, a pull request template, and a Dependabot
  configuration for the workflow actions and the pytest requirements.
- README documentation for installing a release build without compiling, for
  the plugin registry API (`set_plugin`, `get_plugin`, `has_plugin`,
  `del_plugin`, `clear_plugins`) and `get_callback_count`, and for clearing the
  macOS quarantine attribute on a downloaded `.plugin`.

### Changed

- Releases are now cut by pushing a `v*.*.*` tag rather than by publishing a
  release in the GitHub UI. The workflow builds, **tests**, and packages on both
  platforms before anything is published; previously the release build ran no
  tests at all. It also refuses to publish a tag that is not an ancestor of
  `main`, and takes the release notes from this file's entry for the tag.
- Both workflows declare least-privilege `permissions`, and pin the same
  toolchain as the other Actualize repositories (CMake 4.4.0 via
  `lukka/get-cmake`).
- Dropped the `FORCE_JAVASCRIPT_ACTIONS_TO_NODE24` workaround from both
  workflows. It was there for the Node 24 cutover on 2026-06-02, which has
  passed.
- Corrected the stale "Debug Information" sections in README.md and TESTING.md,
  which described an output that does not exist ("Execute count", "Available
  methods list") and an "Auto Process Events" parameter that is named **Auto
  Poll**. They now document the status table and the four Info CHOP channels the
  operator actually publishes.

## [0.3.0] - 2026-05-30

Prepared the repository for publication: MIT license, contributor guide,
security policy, and third-party attribution for the bundled TouchDesigner SDK
headers. CPython was removed from the repository — the build now resolves
Python 3.11 through a uv-managed interpreter instead of vendored headers and
import libraries. Added the Catch2 and pytest unit suites that run in CI, and
the TouchDesigner integration harness that runs locally. Fixed
`FillDATPluginInfo` to honor the result of `setAPIVersion`, which SDK v4 marks
as failable.

## [0.2.2] - 2025-07-03

macOS support: the operator builds as a `.plugin` bundle, and the release
workflow publishes a macOS asset alongside the Windows DLL. No GitHub release
was published for this tag.

## [0.2.1] - 2025-06-01

Release workflow fixes.

## [0.2.0] - 2025-06-01

TOML configuration: a `config.toml` beside the `.toe` can prepend entries to
`sys.path` (`[main].paths`) and point the operator at a callback module
(`[asyncio].callback_module_path`).

## [0.1.0] - 2025-06-01

Initial release: the AsyncioDAT operator with a managed asyncio event loop
polled once per TouchDesigner frame, the Python API for scheduling coroutines
(`add_task`, `create_task`, `run_coroutine`, `get_event_loop`), the plugin
registry, the lifecycle callbacks, and the CI and release workflows.

<!-- Entries for 0.1.0 through 0.3.0 are summarized retrospectively; this file
     was introduced in 0.4.0. -->

[Unreleased]: https://github.com/Actualize-Interactive/AsyncioDAT/compare/v0.4.1...HEAD
[0.4.1]: https://github.com/Actualize-Interactive/AsyncioDAT/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/Actualize-Interactive/AsyncioDAT/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/Actualize-Interactive/AsyncioDAT/compare/v0.2.2...v0.3.0
[0.2.2]: https://github.com/Actualize-Interactive/AsyncioDAT/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/Actualize-Interactive/AsyncioDAT/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/Actualize-Interactive/AsyncioDAT/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Actualize-Interactive/AsyncioDAT/releases/tag/v0.1.0
