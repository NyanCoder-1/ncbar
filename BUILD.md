# Build

## Prerequisites

First of all, prepare wlr-protocols

```sh
cd thirdparty/wlr-protocols
./prepare.sh
cd ../..
```
\* For this you should have installed wayland-scanner and have the [wlr-protocols](https://gitlab.freedesktop.org/wlroots/wlr-protocols) and [wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols) packages (or cloned repos, but in that case you should edit `thirdparty/wlr-protocols/prepare.sh`  and fix the paths to xml files)

## Build

```sh
cd build
cmake ..
make
cd ..
```

## Run
```sh
cd bin
./ncbar
```
