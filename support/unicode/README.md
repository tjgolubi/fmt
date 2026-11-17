# Unicode table generation

The checked-in tables use Unicode 18.0.0. To download the inputs, run these
commands from the repository root.

```sh
cd support/unicode
curl --fail --location --remote-name https://www.unicode.org/Public/18.0.0/ucd/UnicodeData.txt
curl --fail --location --remote-name https://www.unicode.org/Public/18.0.0/ucd/EastAsianWidth.txt
```

When updating Unicode, use the same release in both URLs.

Run `make` in this directory to regenerate `UnicodeData.hpp` and
`EastAsianWidth.hpp` under `include/tjg/color/bits/`.
