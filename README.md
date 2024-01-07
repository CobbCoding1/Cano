# Cano
Cano is a text editor written in C using the ncurses library.
It is a modal-based editor, based heavily on VIM. 

![Cano icon](cano.png) \
Icon from [LocalTexan](https://github.com/LocalTexan)

## Quick Start
Dependencies: CC, ncurses
```sh
./build.sh
./main
```

## Keybinds
| Keybind  | Action                    |
|----------|---------------------------|
| Ctrl + Q | Quit (regardless of mode) |
| h        | Move left                 |
| j        | Move down                 |
| k        | Move up                   |
| l        | Move right                |
| x        | Delete character          |
| d        | Delete entire line        |
| i        | Enter insert mode         |
| Ctrl + S | Save and exit             |


### Insert Mode
'esc' - Exit insert mode, return to normal mode.
The rest of it works like a regular editor as you would expect.

## Contributing
Cano is open to contributors. That does not mean that your pull request will be merged. \
Please structure your pull request as something reasonably small, \
so that is is possible to look over in a reasonable amount of time. \
For the style, try to keep it as close to the code written as possible. \
A pull request will not be rejected for a style conflict.
