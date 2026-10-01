---
paths:
  - "**/*.json"
---

# JSON Rules

Most JSON files in this repo are configuration files, for example `cspell.json`, `.prettierrc.json` and `.devcontainer/devcontainer.json`. Prettier controls their layout.

- Add a new word to the `words` list in `cspell.json` in alphabetical order. Add only words that are spelled correctly, such as product names and technical terms.
- Use camelCase for keys in a hand-authored JSON file, unless the tool that reads the file sets the key names.
