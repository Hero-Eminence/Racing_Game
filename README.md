# Mini Racing Game

Python generates a simple three-lane course, C++ runs the game, and Java
displays the scores saved by the game.

## 1. Generate a course

From this folder, run:

```powershell
python track_generator.py
```

Optional: choose a track length and repeatable random seed:

```powershell
python track_generator.py --length 150 --seed 12
```

Tracks default to 1,000 sections. Change `--length` to make a shorter or
longer race, then regenerate before launching the game.
The game generates random obstacle positions and lanes for every race,
including replays. The generated track determines the race length.

## 2. Build and play

With MinGW g++:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic race.cpp -o race.exe
.\race.exe
```

Type your driver name, then press Space or Enter at the `[ START ]` prompt.
Steer with the Left/Right arrow keys or `A`/`D`.
The car moves forward automatically, and You do not need to press Enter while playing. The play view uses extra-wide
lanes and tall, boxed obstacles, with a large car at the bottom of the road.
The car accelerates gradually as time passes during the race; the current speed
is shown in the game display.
The color display highlights the car in green, obstacles in red, and the road
and score in contrasting colors. Obstacles scroll smoothly toward the car rather
than jumping a full section at once. The `^` marker shows forward direction toward
the top of the road. Press `Q` to end a race. After each race,
press `Y` to start another race without restarting the program or `N` to exit.
You do not need to press Enter for this prompt either. Each race's score is
saved in `scores.txt`.

## 3. View the leaderboard (requires JDK 16 or newer)

```powershell
javac Leaderboard.java
java Leaderboard
```

The Java program reads the `scores.txt` file created by the C++ game. Run all
commands from this folder so the programs use the same track and score files.
