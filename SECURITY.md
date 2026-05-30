# Security Policy

## Supported versions

AsyncioDAT is maintained on a rolling basis. Security fixes target the latest
release and the `main` branch.

## Reporting a vulnerability

Please report security issues privately rather than opening a public issue.

- Use GitHub's **"Report a vulnerability"** (Security → Advisories) on this
  repository, or
- Email **keith@actualize.vision** with details and reproduction steps.

We will acknowledge your report and keep you updated on remediation. Thank you
for helping keep AsyncioDAT and its users safe.

## Scope notes

AsyncioDAT runs Python coroutines inside TouchDesigner's interpreter and can
load callback modules from disk (see the `config.toml` `callback_module_path`
and `[main].paths` settings). Treat `.toe` files and any configured callback
modules / `sys.path` entries as trusted code: a malicious project file can run
arbitrary Python, just as a native TouchDesigner project can.
