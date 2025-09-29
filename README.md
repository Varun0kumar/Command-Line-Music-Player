# Command Line Music Player

A fully functional command line music player built using **C language** for Windows.  
Supports playlists, playback controls, history, and shuffle functionality.

## Features
- Create, switch, and delete playlists
- Add, remove, search, and display songs
- Play a playlist, specific songs, or shuffle play
- Display playback history
- Interactive controls: pause/resume, next, previous, stop
- Lightweight and fast

## How to Compile & Run
### Requirements
- Windows OS
- GCC Compiler (MinGW recommended)

### Compilation
```bash
gcc "CL Music Player.c" -o CLMusicPlayer.exe -lwinmm
### running
"""./CLMusicPlayer.exe"""

Tech Stack

-C Programming
-Windows API (windows.h, mciSendString)
-Command Line Interface

Author

Varun Kumar
