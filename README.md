# Cano
Cano (kay-no) is a text editor written in C using the ncurses library.
It is a modal-based editor, based heavily on VIM. 

![Cano icon](cano.png) \
Icon from [LocalTexan](https://github.com/LocalTexan)

## Quick Start
Dependencies: GCC, Make, ncurses
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
|Mode  | Keybind        | Action                                          |
|------|----------------|-------------------------------------------------|
|Global| Ctrl + Q       | Quit (regardless of mode)                       |
|Global| Esc            | Enter Normal Mode                               |
|Normal| h              | Move left                                       |
|Normal| j              | Move down                                       |
|Normal| k              | Move up                                         |
|Normal| l              | Move right                                      |
|Normal| x              | Delete character                                |
|Normal| d + d          | Delete entire line                              |
|Normal| d + w          | Deletes current word                            |
|Normal| d + g          | Delete current to first line                    |
|Normal| d + G          | Delete current to last line                     |
|Normal| y + y          | Yanks the current line                          |
|Normal| g              | Go to first line                                |
|Normal| G              | Go to last line                                 |
|Normal| 0              | Go to beginning of line                         |
|Normal| $              | Go to end of line                               |
|Normal| w              | Go to next word                                 |
|Normal| b              | Go to last word                                 |
|Normal| e              | Go to end of next word                          |
|Normal| o              | Create line below current                       |
|Normal| O              | Create line above current                       |
|Normal| ctrl + o       | Create line below current without changing mode |
|Normal| %              | Go to corresponding brace                       |
|Normal| i              | Enter insert mode                               |
|Normal| I              | Go to beginning of line                         |
|Normal| a              | Insert mode on next char                        |
|Normal| A              | Insert mode at end of line                      |
|Normal| v              | Enter visual mode                               |
|Normal| V              | Enter visual mode by line                       |
|Normal| u              | Undo                                            |
|Normal| U              | Redo                                            |
|Normal| /              | Enter search mode                               |
|Normal| n              | Jump to next search                             |
|Normal| Ctrl + S       | Save and exit                                   |
|Normal| r              | Replace current char with next char inputted    |
|Normal| (n) + motion   | Repeat next motion n times                      |

## Visual
Visual mode works the same as Normal mode, except works on the entire selection, instead of character by character.

## Search
Search mode takes a string and finds it in the file.
if prepended with 's/' then it will replace the first substring with the second.
```sh
s/hello/goodbye
```
Will replace hello with goodbye.

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

There is a secondary config file, which is for custom syntax highlighting. It is stored in the same folder as the regular config, but uses a different naming format.
An example is ~/.config/cano/c.cyntax (spelled cyntax, with a c). The c can be replaced with whatever the file extension of your language is, such as go.cyntax for Golang.
Here is an example of a cyntax file:
```sh
k,170,68,68,
auto,struct,break,else,switch,case,
enum,register,typedef,extern,return,
union,continue,for,signed,void,do,
if,static,while,default,goto,sizeof,
volatile,const,unsigned.
t,255,165,0,
double,size_t,int,
long,char,float,short.
w,128,160,255.
```
There's a bit to unpack here, basically the single characters represent the type of the keywords:
k - Keyword
t - Type
w - Word
The type is then followed by the RGB values, all comma separated without spaces. After the RGB values, there is the actual keywords. End each type with a dot '.' as seen above, to indicate to Cano that the list is finished. The words are meant to be left blank, as it will highlight any words not found in the keywords above with the chosen RGB color.
If you wish to only set the color, you can provide no keywords to any, and it will fill in the keywords with C keywords by default.

## Config Variables
```sh
relative # toggle relative line numbers
auto-indent # toggle auto indentation on-off
syntax # toggle syntax highlighting on-off
indent # set indent
undo-size # size of undo history 
```

# Installing

## Arch

A package is provided within this [AUR](https://aur.archlinux.org).
You can install it using your preferred aur helper:

For instance, if using yay, do the following:
```sh
yay -S cano-git
```

## Nix / NixOS

If you are using NixOS or have installed the [nix package manager](https://nixos.org),
you can run cano using the following command:

```sh
nix run github:CobbCoding1/Cano
```

A flake a provided within the repository, including the package and a devShell.


## Contributing
Cano is open to contributors. That does not mean that your pull request will be merged. \
Please structure your pull request as something reasonably small, \
so that is is possible to look over in a reasonable amount of time. \
For the style, try to keep it as close to the code written as possible. \
A pull request will not be rejected for a style conflict.
