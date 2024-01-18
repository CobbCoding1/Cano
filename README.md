# Cano
Cano (kay-no) is a text editor written in C using the ncurses library.
It is a modal-based editor, based heavily on VIM. 

![Cano icon](cano.png) \
Icon from [LocalTexan](https://github.com/LocalTexan)

## Quick Start
Dependencies: CC, Make, ncurses
```sh
make -B
./main
```

## Modes
Normal - For motions and deletion \
Insert - For inserting text \
Visual - For selecting text and performing actions on them \
Search - For searching of text in the current buffer \
Command - For executing commands

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
|Normal| i        | Enter insert mode                               |
|Normal| I        | Go to beginning of line                         |
|Normal| a        | Insert mode on next char                        |
|Normal| A        | Insert mode at end of line                      |
|Normal| v        | Enter visual mode                               |
|Normal| V        | Enter visual mode by line                       |
|Normal| /        | Enter search mode                               |
|Normal| n        | Jump to next search                             |
|Normal| Ctrl + S | Save and exit                                   |
|Normal| r        | Replace current char with next char inputted    |
|Normal| R + (n)  | Repeat next motion n times                      |

Visual mode works the same as Normal mode, except works on the entire selection, instead of character by character.

## Commands 
| Command               | Action                    |
|-----------------------|---------------------------|
| set-output            | change output file        |
| q                     | Quit                      |
| we                    | Write and quit            |
| e                     | Write without quitting    |
| set-var (var) (value) | Change a config variable  |
| !(command)            | Execute a shell command   |

## Config file
The config file is stored in ~/.config/cano/config.cano by default, or can be set at runtime like so:
```sh
./cano --config <config_file>
```

The format of the file is the same as commands, it is line separated, which is important.
Example:
```sh
set-var syntax 1 
set-var indent 2 
```

# Installing
Currently, the only way to install is to build the package, or use the AUR.

If using yay, do the following:
```sh
yay -S cano-git
```

## Contributing
Cano is open to contributors. That does not mean that your pull request will be merged. \
Please structure your pull request as something reasonably small, \
so that is is possible to look over in a reasonable amount of time. \
For the style, try to keep it as close to the code written as possible. \
A pull request will not be rejected for a style conflict.
