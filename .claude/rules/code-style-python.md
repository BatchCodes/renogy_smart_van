---
paths:
  - "**/*.py"
---

# Python Rules

Python in this repo is for developer tools only, for example `tools/fonts/generate_fonts.py`. The firmware does not use Python.

- Follow PEP 8: four-space indents, snake_case functions and variables, PascalCase classes, SHOUT_CASE constants.
- Start each file with `#!/usr/bin/env python3` and the line `# SPDX-License-Identifier: GPL-3.0-or-later`.
- Give each tool a module docstring that states what it does, how to run it and what it needs, for example a Debian package.
- Use only the standard library and packages that Debian ships, for example `python3-pil`. Name each extra package in the docstring and in `CONTRIBUTING.md`.
- Put file-level constants, such as paths and sizes, near the top of the file. Resolve repo paths from `Path(__file__)`, not from the current directory.
- Put the logic in functions, and call `main()` from an `if __name__ == "__main__":` guard.
- Add type hints to function parameters and return values.
