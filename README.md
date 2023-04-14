# Menga Entertainment System

MES is a cartridge-based video game console. For a tutorial and
documentation see our [wiki](https://github.com/menga-team/MES/wiki)

## Installation

### Clone the repository with submodules:

```shell
# via SSH
$ git clone --recursive git@github.com:menga-team/MES.git
# via HTTPS
$ git clone --recursive https://github.com/menga-team/MES.git
```

If you already cloned the repository you can initialize the submodules
with following command:`git submodule update --init`

### Make libopencm3

```shell
    $ cd libopencm3
    $ make
```

## Hacking

Thank you for considering to contribute to MES!
Here are some things before you can get started:

### Style

This repository contains a `.clang-format` file. Please format your
patches with this style guide in mind.

### Revisions

Every CPU firmware has a revision number which is incremented on every
change that concerns the firmware.  This revision number can be then
used in games (e.g. For checking if the game is compatible because
of a bug or API change).

Because its easy to forget incrementing it, we provide a git hook that
will do it for you.  To install it simply run:

```shell
$ cp hooks/post-commit .git/hooks/post-commit
$ chmod +x .git/hooks/post-commit
```

## License

```
MES
Copyright (c) 2022, 2023 menga-team

TinyMT
Copyright (c) 2011, 2013 Mutsuo Saito, Makoto Matsumoto,
Hiroshima University and The University of Tokyo.

udynlink
Copyright (c) 2016 Bogdan Marinescu
Copyright (c) 2022, 2023 menga-team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
```

