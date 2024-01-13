# Cano
Cano (kay-no) is a text editor written in C using the ncurses library.
It is a modal-based editor, based heavily on VIM. 

![Cano icon](cano.png) \
Icon from [LocalTexan](https://github.com/LocalTexan)

## Quick Start
Dependencies: CC, Make, ncurses
```sh
mkdir build
make -B
./main
```

## Keybinds
|Mode  | Keybind  | Action                                          |
|------|----------|-------------------------------------------------|
|Global| Ctrl + Q | Quit (regardless of mode)                       |
|Global| Esc      | Enter Normal Mode                               |
|Normal| h        | Move left                                       |
|Normal| j        | Move down                                       |
|Normal| k        | Move up                                         |
|Normal| l        | Move right                                      |
|Normal| x        | Delete character                                |
|Normal| d        | Delete entire line                              |
|Normal| i        | Enter insert mode                               |
|Normal| I        | Go to beginning of line                         |
|Normal| a        | Insert mode on next char                        |
|Normal| A        | Insert mode at end of line                      |
|Normal| g        | Go to first line                                |
|Normal| G        | Go to last line                                 |
|Normal| 0        | Go to beginning of line                         |
|Normal| $        | Go to end of line                               |
|Normal| w        | Go to next word                                 |
|Normal| b        | Go to last word                                 |
|Normal| e        | Go to end of next word                          |
|Normal| o        | Create line below current                       |
|Normal| O        | Create line above current                       |
|Normal| ctrl + o | Create line below current without changing mode |
|Normal| %        | Go to corresponding brace                       |
|Normal| /        | Enter serach mode                               |
|Normal| n        | Jump to next search                             |
|Normal| Ctrl + S | Save and exit                                   |
|Normal| r + (n)  | Repeat next motion n times                      |

## Commands 
| Command    | Action                    |
|------------|---------------------------|
| set-output | change output file        |
| quit       | Quit                      |
| wquit      | Write and quit            |
| w          | Write without quitting    |
| relative   | Toggle relative line nums |
| !(command) | Execute a shell command   |

## Contributing
Cano is open to contributors. That does not mean that your pull request will be merged. \
Please structure your pull request as something reasonably small, \
so that is is possible to look over in a reasonable amount of time. \
For the style, try to keep it as close to the code written as possible. \
A pull request will not be rejected for a style conflict.
