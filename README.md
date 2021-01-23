# RpgBookOnline

Cross-platform server for [RpgBookOnline](#the-rbo-project), an online role-play editor and game.

## Table of contents

* [RpgBookOnline](#rpgbookonline)
    * [Table of contents](#table-of-contents)
    * [The Rbo project](#the-rbo-project)
    * [The server](#the-server)
        * [Install](#install)
            * [Prerequisites](#prerequisites)
            * [Steps](#steps)
            * [Project options](#project-options)
        * [Quick and Easy install (For Windows)](#quick-and-easy-install-for-windows)
        * [Run](#run)

## The Rbo project

RpgBookOnline or *Rbo* is the global project to which this server belongs. You can basically make your own classic paper role-play, save it and share it to play online with other players.

Creation design is inspired from the French *You Are The Hero Books*, where you travel between pages to progress (or sometimes regress) into the adventure.

You can find the Rbo client [just here](https://github.com/ThisALV/RboClient), yet it's still a WIP. The RPGs editor development will begin as soon as the client development will have finished.

For more details about how to play this game, check the [wiki](https://github.com/ThisALV/RpgBookOnline/wiki).

## The server

### Install

#### Prerequisites

You need to get these dependencies to build the project :

```shell
sudo apt install libboost-all-dev
sudo apt install libspdlog-dev
sudo apt install nlohmann-json3-dev
sudo apt install liblua5.1-0-dev
```

You will also need to get sol2, which is not on Ubuntu repositories :

```shell
git clone https://github.com/ThePhD/sol2
cd sol2
cmake .
sudo cmake --install .
```


#### Steps

1. Clone this repo with `git clone https://github.com/ThisALV/RpgBookOnline`

2. Move to your downloaded repo's root and move into a directory that will create with `mkdir build && cd build`

3. Generate CMake project with `cmake -DCMAKE_BUILD_TYPE=Release ..` (see [here](#project-options) for more details)

4. Build the generated CMake project with `cmake --build . -- -jN` *(replace N by the number of cpu cores to use for the parallel build)*

5. Copy the game/ and instructions/ directories into your build/ directory (so the two folders with be next to your built executable)

6. Done. You can now check the [wiki](https://github.com/ThisALV/RpgBookOnline/wiki) and play.

#### Project options

There are few option you can use at installation *step 3* to modify the generated CMake project.

- `-DRBO_TESTS_ENABLED` which enables building of unit tests executables

- `-DRBO_LOGGING_HEADER_ONLY` which enables header-only mode for [spdlog](https://github.com/gabime/spdlog), logging library used by this server *(Note that some compilers and environments with link-time issues like lld with MinGW might need this option to build the project)*

- `-DRBO_REQUIRED_SPDLOG` which enables a minimal version check for the logging library *(A recent version might be required to have header-only spdlog fix working)*

- `-DRBO_STATIC_WINDOWS_LIBRARIES` which enables static linking with ws2_32 and wsock32 *(Might be necessary if you're building with MinGW, please note that it doesn't actually perform a static-linkage with a part of the WinAPI, here libws2_32 and libwsock32 are more like a pipeline to access dynamically loaded Windows libraries)*

### Quick and Easy install (For Windows)

*Coming soon...*

### Run

*Don't forget that instructions/ and game/ directories contain game information and must be in same folder than your executable, else the RPG will not be loaded.*

In a terminal, next to your built executable, syntax is :

    ./server <ip_version> <port> <preparation_countdown_ms>

Or, for Windows :

    server.exe <ip_version> <port> <preparation_countdown_ms>

Parameters are :

- `ip_version` for the protocol version, might be `ipv4` or `ipv6`

- `port` for the local opened server connection port

- `preparation_countdown_ms` for the countdown before preparation when all lobby members are ready
