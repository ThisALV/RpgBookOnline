# RpgBookOnline

Cross-platform server for [RpgBookOnline](#the-rbo-project), an online role-play editor and game.

## Table of contents

* [RpgBookOnline](#rpgbookonline)
    * [The Rbo project](#the-rbo-project)
        * [How it works](#how-it-works)
            * [Gameplay](#gameplay)
                * [Game proceedings](#game-proceedings)
                * [Stats](#stats)
                * [Inventories](#inventories)
                * [Death and game's end conditions](#death-and-games-end-conditions)
                * [Enemies and groups](#enemies-and-groups)
            * [RPG creation](#rpg-creation)
    * [The server](#the-server)
        * [Prerequisites](#prerequisites)
        * [Install](#install)
            * [Project options](#project-options)
        * [Quick and Easy install (For Windows)](#quick-and-easy-install-for-windows)
        * [Run](#run)
        * [Load any RPG](#load-any-rpg)


## The Rbo project

RpgBookOnline or *Rbo* is the global project to which this server belongs. You can basically make your own classic paper-rpg, save it and share it to play among the other players.

Creation design is inspired from the French *You Are The Hero Books*, where you can switch between pages to progress (or maybe regress) across the adventure.

You can find the Rbo client [just here](https://github.com/ThisALV/RboClient), yet it's still a WIP. The RPGs editor development will begin as soon as the client development will have finished.

### How it works

#### Gameplay

##### Game proceedings

The server hosts a lobby that players join, so they become lobby members. Once every member is ready, a countdown is started and when it's over, the server asks the room master (member with the lowest ID) for game configuration.

When game configuration is done, a session is started by the lobby and then the game begins.

##### Stats

Each player and each enemy has its own stats that might be blocked with **min** and **max** values, but there are also stats which are shared among all players, known as **global stats**.

##### Inventories

Each player also has its own inventories. An inventory, identified by its name, might be capped with a limited count of items, this limit is the **capacity** of the inventory.
An inventory has a list of items name which represents the names of each item that can belong to the inventory.
Every item in the inventory is associated with a **quantity**, the item's **count**.

##### Death and game's end conditions

An RPG may define some conditions that will make a player die or make the game stop. The two categories of conditions are separated into game.json.

An exemple of a player death condition would be `HP =< 0` and an exemple of game's end condition would be `"Victory points" >= 30`.

##### Enemies and groups

An enemy is created by using its character sheet defined in game.json. It has an **unique name** (or contextual name), a **generic name** which is the name of its character sheet and two stats, **hp** ans **skill**.

Enemies can only be manipulated by using enemies groups. These groups themselves can only be created using enemies group sheet loaded from game.json.
An enemies group is basically a **queue** of enemies designed to switch **current target** from one enemy to the next in purpose to make easier a round-by-round battle system.

##### Events

To modify player characteristics (inventories or stats), the end-user way is to call an event defined and named in game.json which describe the modifications of items quantity and stats value.

#### RPG creation

The server provides a Lua API used by the functions defined in the `Rbo` Lua table in the instructions/ directory. Each file in this directory is executed so each function is loaded.

Each of these functions is called an **instruction**. This is what the end-user uses to make RPGs. An instruction might have a return value, in which case it will be used as an ID of the next scene to go, or might not have any return value, in which case the server continue to the next instruction. If the end of the scene (or page) is reached and none of the instructions in it returned any values, then the game is stopped.

Soon, an editor will allow the end-user to create RPGs without having to edit by hand game.json and scenes.lua.
Until that point, you can see an exemple of game.json, scenes.lua and an exemple of an instruction implemented with a mod in the sample game provided by this repository. [(more details about the game data files)](#load-any-rpg)

## The server

### Prerequisites

You need to get these dependencies to build the project :

```shell
sudo apt install libboost-all-dev
sudo apt install libspdlog-dev
sudo apt install nlohmann-json3-dev
sudo apt install liblua5.1-0-dev
```

You will also need to get sol2, which is not in Ubuntu repositories :

```shell
git clone https://github.com/ThePhD/sol2
cd sol2
cmake .
sudo cmake --install .
```

### Install

1. Clone this repo with `git clone https://github.com/ThisALV/RpgBookOnline`

2. Move to your downloaded repo's root and move into a directory that will create with `mkdir build && cd build`

3. Generate CMake project with `cmake -DCMAKE_BUILD_TYPE=Release ..` (see [here](#project-options) for more details)

4. Build the generated CMake project with `cmake --build . -- -jN` *(replace N by the number of cpu cores to use for the parallel build)*

5. Copy the game/ and instructions/ directories into your build/ directory (so the two folders with be next to your built executable)

6. Done. You can now [run](#run) the server and [load any rpg](#load-any-rpg) you want.

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

### Load any RPG

Here is how game data is stored inside directories :

- game/
    - game.json // Basic data like enemies, events, stats, inventories, etc.
    - scenes.lua // Script of the RPG, contains all the scenes
    - chkpts.json // Contains every checkpoint saved
- instructions/
    - Base.lua // Contains basic game instructions, called by the scenes script
    - ... // Might contains other .lua files

If your game need additional instructions, it's preferred to create new .lua files into instructions/ so you can share these files when sharing your game easily. This is how you can mod your game.

You also need to give game.json and scenes.lua if you want to share your RPG.
