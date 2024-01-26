# Lighthouse Launcher

User themes are configured in C and dynamically loaded (and hot swapable in the future). See `src/themes/default_ui.c` & how to build in `build.sh`. Right now there is no way to change theme except in `main.c`.

Plugins are stored in `src/search_plugins`. See `src/search_plugins/application_files` as an example. Plugins will be loaded dynamically with their .so files. As of now there is only a half baked application file parser. 

WIP launcher for linux currently in it's baby state. It supports parsing and reading from desktop files currently with a simple UI and Trie search (to  be changed.)

![screenshot](https://i.imgur.com/lp4Pru7.png)
