<!--
Thanks for contributing! Please keep PRs focused — one logical change per PR.
See CONTRIBUTING.md for build, test, and style expectations.
-->

## What does this change?

<!-- A short description, and the issue it closes (e.g. "Closes #12"). -->

## Why?

<!-- The motivation or use case. For a bug fix, what was going wrong. -->

## How was it tested?

<!--
Which suites you ran, and on what. The unit suites run anywhere; the
TouchDesigner integration suite needs a local install. State your
TouchDesigner version if you ran it.
-->

## Checklist

- [ ] The project builds on my platform (`.\build.ps1` or `./build.sh`).
- [ ] The unit suites pass (`cmake --workflow --preset dev`).
- [ ] The TouchDesigner integration suite passes, or is not affected
      (`.\run_td_tests.ps1` / `./run_td_tests.sh`).
- [ ] New or changed behavior is covered by tests.
- [ ] `README.md` / `TESTING.md` updated for any change to parameters, the
      Python API, or the callbacks.
- [ ] `CHANGELOG.md` has an entry under `[Unreleased]`, if user-visible.

## Breaking changes

<!--
Operator parameter names and the Python API are meant to be stable — renaming a
parameter breaks every saved .toe that uses it. If this changes an existing
name or contract, describe the break and what users must do. Write "None"
otherwise.
-->

None
