# Menga Entertainment System

is a cartridge-based video game console. For a tutorial and documentation see our [wiki](https://github.com/menga-team/MES/wiki)





### Installation
1.  Clone the repository with submodules:
    SSH:
        $ git clone --recursive git@github.com:menga-team/MES.git
    HTTPS:
        $ git clone --recursive https://github.com/menga-team/MES.git
    If you already cloned the repository you can initialize the submodules with following command:
        $ git submodule update --init

2. Make libopencm3:
    $ cd libopencm3
    $ make
