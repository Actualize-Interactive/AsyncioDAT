# Contributing to AsyncioDAT

Thanks for your interest in improving AsyncioDAT! Contributions of all kinds —
bug reports, fixes, docs, and features — are welcome.

## Reporting issues

Open a GitHub issue using the bug report form, which asks for:

- Your TouchDesigner version (e.g. 2025.32820) and OS.
- What you expected to happen vs. what happened.
- A minimal `.toe` or script that reproduces the problem, if possible.
- Relevant output from the AsyncioDAT operator's status table / textport.

Security issues go through [private reporting](SECURITY.md) instead, not a
public issue.

## Development setup

AsyncioDAT is a C++ TouchDesigner Custom Operator (a DAT) that embeds an
asyncio event loop in TouchDesigner's CPython 3.11 interpreter.

### Prerequisites

- **CMake** 3.25+
- **Windows:** Visual Studio 2019/2022 (Desktop C++ workload)
- **macOS:** Xcode Command Line Tools
- **Python 3.11**, provided via [uv](https://docs.astral.sh/uv/):

  ```bash
  uv python install 3.11
  ```

  TouchDesigner embeds CPython 3.11, so the operator must be built against 3.11
  for ABI compatibility. We deliberately do **not** vendor Python — a
  uv-managed CPython ships the headers and import library we need. (A bare
  virtualenv does not contain headers; they live in the base interpreter that
  the venv points at, which is why CMake is pointed at the base prefix.)

### Build

CMake auto-detects the uv-managed Python 3.11 (see `CMakePresets.json`):

```bash
cmake --preset dev
cmake --build --preset dev --target asyncio_dat
```

`build.ps1` (Windows) and `build.sh` (macOS/Linux) wrap these steps. The built
operator is copied to `tests/td/Plugins/` automatically.

## Testing

See [TESTING.md](TESTING.md). In short:

- `tests/cpp/` (Catch2) and `tests/python/` (pytest) cover the asyncio / plugin /
  config logic without TouchDesigner and run in CI. Run them all with one
  command: `cmake --workflow --preset dev`.
- The integration suite in `tests/td/` (`asyncio_test.py`, driven by
  `td_test_runner.py`) and the harness (`run_td_tests.ps1` / `run_td_tests.sh`)
  launch TouchDesigner, run the suite, and report results — they require a local
  TouchDesigner install and cannot run in cloud CI (TouchDesigner needs a
  license and a GPU).

Please make sure the project builds on your platform and that any behavior you
changed is covered by or verified against the test scripts before opening a PR.

## Pull requests

- Branch off `main` and keep PRs focused.
- Use clear, imperative commit messages with a type prefix (`fix:`, `feat:`,
  `docs:`, `test:`, `chore:`, `ci:`).
- Match the existing code style (tabs in C++ sources, existing naming).
- Update `README.md` / `TESTING.md` when you change behavior, parameters, or the
  Python API.
- Add an entry under `[Unreleased]` in [CHANGELOG.md](CHANGELOG.md) for any
  user-visible change, and call out breaking changes explicitly.
- Describe how you tested the change.

**Operator parameter names and the Python API are stable.** A parameter's
internal name (`Autopoll`, `Maxstatusrows`, …) is what saved `.toe` files store,
so renaming one silently drops the user's setting on load. Prefer additive
changes; if a break is unavoidable, say so in the PR and in the changelog.

## Releasing

Releases are cut from `main` by pushing a tag:

1. In a PR, bump `project(AsyncioDAT VERSION …)` in `CMakeLists.txt` and move
   the `[Unreleased]` changelog entries under a `## [x.y.z] - YYYY-MM-DD`
   heading, adding the compare link at the bottom of the file.
2. After it merges, tag the merge commit on `main` and push the tag:

   ```bash
   git tag vx.y.z && git push origin vx.y.z
   ```

The Release workflow builds, tests, and packages on Windows and macOS, refuses
the tag if it is not an ancestor of `main`, and publishes the release using the
changelog entry for that version as the release notes. It fails if no such entry
exists.

## License

By contributing, you agree that your contributions are licensed under the
[MIT License](LICENSE) that covers this project.
