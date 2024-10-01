# jinsp

![screenshot](assets/screenshot1.png)

jinsp is a terminal-based interface (TUI) for quick inspection of JSON data (on Linux only at present).
It allows hierarchical navigation through the JSON contents, in a manner inspired by that of the [ranger file manager](https://github.com/ranger/ranger).

## Build

Run `make` in the root directory.

## Usage

Launch the interface by running `./jinsp <json file>`.
Navigation can be done using the arrow keys or mouse.
Other operations:
* `q` or `esc`: quit
* `/` followed by a keyword and `return` key: case-sensitive search starting from current position (`esc` to abort input)
* `n`/`N`: navigate search results forwards and backwards, respectively

