# jinsp

jinsp provides a terminal-based interface (TUI) for quick inspection of JSON data.
Its hierarchical visualization was inspired by that of the [ranger file manager](https://github.com/ranger/ranger).

## Build

On Linux, simply run `make`.

## Usage

Launch the interface by running `./jinsp <json file>`.
Navigation can be done using the arrow keys or mouse.
Other operations:
* `q` or `esc`: quit
* `/` followed by a keyword and `return` key: case-sensitive search starting from current position (`esc` to abort input)
* `n`/`N`: navigate search results forwards and backwards, respectively

