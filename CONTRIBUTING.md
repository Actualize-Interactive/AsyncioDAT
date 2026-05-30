# Contributing to AsyncioDAT

Thanks for your interest in improving AsyncioDAT! Contributions of all kinds —
bug reports, fixes, docs, and features — are welcome.

## Reporting issues

Open a GitHub issue and include:

- Your TouchDesigner version (e.g. 2025.32820) and OS.
- What you expected to happen vs. what happened.
- A minimal `.toe` or script that reproduces the problem, if possible.
- Relevant output from the AsyncioDAT operator's status table / textport.

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
- Match the existing code style (tabs in C++ sources, existing naming).
- Update `README.md` / `TESTING.md` when you change behavior, parameters, or the
  Python API.
- Describe how you tested the change.

## License

By contributing, you agree that your contributions are licensed under the
[MIT License](LICENSE) that covers this project.
