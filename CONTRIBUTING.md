# Contributing

Thanks for helping! Bug reports, test results with other boards or consoles, translations and code are all welcome.

## Reporting problems

Use the issue templates. Never post Wi-Fi passwords, MAC/Bluetooth addresses or IP addresses – replace them with
`AA:BB:CC:DD:EE:FF` and `192.168.x.x`.

## Development

See [docs/technical/building.md](docs/technical/building.md) and
[docs/technical/how-it-works.md](docs/technical/how-it-works.md). Before opening a pull request:

```sh
tests/run.sh
python tools/privacy_scan.py
cd firmware && idf.py build && python ../tools/verify_build.py
```

Guidelines:

- Keep the radio code (`third_party/`, `firmware/softbr_patched/`) unchanged unless the change is the point of the
  pull request; if you change `br_c6_radio.c`, update the patch file and the hashes in `tools/verify_build.py` and
  explain the change.
- C code: C11, ESP-IDF style (4 spaces, `snake_case`), no dynamic allocation in the wake path.
- Commit messages: short imperative summary line, details in the body.

## Translations

- **Panel texts** live in the `T` object in `firmware/main/portal.html`, one entry per language. English (`en`) is the
  reference; every language must have every key.
- **User guides** live in `docs/<language>/user-guide.md`. When the English guide changes, update the others or open
  an issue listing what needs translating.
- Adding a language: add it to `T` and to the language list in the panel, create `docs/<code>/user-guide.md`, and
  add it to the README table and to `web/index.html`.

Native speakers reviewing existing translations are especially welcome – some were written without native review.

## License

By contributing you agree that your contribution is licensed under the [MIT License](LICENSE).
