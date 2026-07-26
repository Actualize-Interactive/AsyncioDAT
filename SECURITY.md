# Security Policy

## Supported versions

AsyncioDAT is pre-1.0 and is developed on a single line. Security fixes are
applied to the latest release only; there are no maintained backport branches.

| Version | Supported |
| --- | --- |
| 0.4.x | ✅ |
| < 0.4 | ❌ |

## Reporting a vulnerability

**Please do not report security issues through public GitHub issues.**

Report privately through GitHub's
[private vulnerability reporting](https://github.com/Actualize-Interactive/AsyncioDAT/security/advisories/new).
The form is the only reporting channel; it lets us discuss and fix the issue
with you before anything becomes public, and it requires nothing more than a
GitHub account.

Please include:

- the affected version or commit, and your TouchDesigner version,
- a description of the issue and its impact,
- the steps, script, or minimal `.toe` needed to reproduce it,
- and any suggested fix, if you have one.

You can expect an acknowledgement within a few business days. We will keep you
informed as we investigate, and will credit you in the release notes when the
fix ships unless you prefer otherwise.

## Scope

AsyncioDAT is a native operator loaded into TouchDesigner's process. It has no
network or process boundary of its own: it drives an asyncio event loop on
behalf of the Python code in the hosting project. The issues most relevant here
are memory-safety and lifetime problems in the operator itself, such as:

- crashes or memory corruption reachable from the Python API (`add_task`,
  `create_task`, `run_coroutine`, the plugin registry) with unusual but
  legitimate arguments,
- reference-counting errors on Python objects that lead to use-after-free,
- crashes triggered by the operator's own error paths, by a malformed
  `config.toml`, or by shutting the loop down while tasks are in flight.

### Out of scope by design

AsyncioDAT runs Python coroutines inside TouchDesigner's interpreter and loads
callback modules from disk — see the `config.toml` `callback_module_path` and
`[main].paths` settings. Treat `.toe` files and any configured callback modules
or `sys.path` entries as **trusted code**: a malicious project file can run
arbitrary Python, exactly as a native TouchDesigner project can. A report whose
premise is that an untrusted `.toe` or callback module executes code is
describing the intended design, not a vulnerability.

Because the operator trusts its hosting project by design, a report that
depends on the project passing deliberately corrupt state is likely to be
treated as a normal bug rather than a vulnerability. Report it as a regular
issue and we will still fix it.
