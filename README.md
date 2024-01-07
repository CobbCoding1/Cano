# Cano
Cano is a text editor written in C using the ncurses library.
It is a modal-based editor, based heavily on VIM. 

## Quick Start
Dependencies: CC, ncurses
```sh
./build.sh
./main
```

## Keybinds
'ctrl + q' - Exit, global keybind, regardless of mode.
### Normal Mode
'h', 'j', 'k', 'l' - Movement. \
'x' - Delete character. \
'd' - Delete entire line. \
'i' - Enter insert mode. \
'ctrl + s' - Save and exit. \

## Insert Mode
'esc' - Exit insert mode, return to normal mode.
The rest of it works like a regular editor as you would expect.

## Contributing
Cano is open to contributors. That does not mean that your pull request will be merged. \
Please structure your pull request as something reasonably small, \
so that is is possible to look over in a reasonable amount of time. \
For the style, try to keep it as close to the code written as possible. \
A pull request will not be rejected for a style conflict.
