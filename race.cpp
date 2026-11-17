#include <algorithm>
#include <cctype>
#include <chrono>
#include <conio.h>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

namespace {
constexpr int kLaneCount = 3;
constexpr int kVisibleSections = 8;
constexpr int kRowsPerSection = 3;
constexpr int kStartingFrameDurationMs = 130;
constexpr int kFastestFrameDurationMs = 45;
constexpr double kAccelerationDurationSeconds = 120.0;
constexpr const char* kReset = "\x1b[0m";
constexpr const char* kRoadColor = "\x1b[90m";
constexpr const char* kObstacleColor = "\x1b[1;31m";
constexpr const char* kCarColor = "\x1b[1;32m";
constexpr const char* kHudColor = "\x1b[1;36m";
constexpr const char* kHighlightColor = "\x1b[1;33m";

void enableVirtualTerminal() {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (output == INVALID_HANDLE_VALUE || !GetConsoleMode(output, &mode) ||
        !SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        throw std::runtime_error("Run the game in a Windows terminal.");
    }
}

char readCommand() {
    if (!_kbhit()) {
        return '\0';
    }

    int key = _getch();
    if (key == 0 || key == 224) {
        key = _getch();
        if (key == 75) {
            return 'a';
        }
        if (key == 77) {
            return 'd';
        }
        return '\0';
    }
    return static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
}

int frameDurationMs(std::chrono::steady_clock::duration elapsed) {
    const double progress = std::min(
        1.0, std::chrono::duration<double>(elapsed).count() /
                 kAccelerationDurationSeconds);
    return kStartingFrameDurationMs -
           static_cast<int>((kStartingFrameDurationMs -
                             kFastestFrameDurationMs) *
                            progress);
}

std::vector<int> generateRaceTrack(std::size_t length,
                                   std::mt19937& randomEngine) {
    std::bernoulli_distribution obstacleAppears(0.65);
    std::uniform_int_distribution<int> laneDistribution(0, kLaneCount - 1);
    std::vector<int> raceTrack;
    raceTrack.reserve(length);
    for (std::size_t section = 0; section < length; ++section) {
        raceTrack.push_back(obstacleAppears(randomEngine)
                                ? laneDistribution(randomEngine)
                                : -1);
    }
    return raceTrack;
}

bool waitForStart() {
    std::cout << "\x1b[2J\x1b[H"
              << kHudColor
              << "  __  __ _____ _   _ ___   ____  _____ ____ _____ _   _  ____ \n"
              << " |  \\/  |_   _| \\ | |_ _| |  _ \\| ____/ ___| ____| \\ | |/ ___|\n"
              << " | |\\/| | | | |  \\| || |  | |_) |  _|| |   |  _| |  \\| | |  _ \n"
              << " | |  | | | | | |\\  || |  |  _ <| |__| |___| |___| |\\  | |_| |\n"
              << " |_|  |_| |_| |_| \\_|___| |_| \\_\\_____\\____|_____|_| \\_|\\____|\n"
              << kReset << "\n"
              << "               " << kCarColor << ">>> READY TO RACE <<<" << kReset << "\n\n"
              << "                  " << kHighlightColor << "[  START  ]" << kReset << "\n"
              << "          Press SPACE or ENTER to start, Q to quit.\n"
              << std::flush;
    while (true) {
        const int key = _getch();
        if (key == 13 || key == ' ') {
            return true;
        }
        if (std::tolower(static_cast<unsigned char>(key)) == 'q') {
            return false;
        }
    }
}

std::vector<int> loadTrack(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open " + path +
                                 ". Run track_generator.py first.");
    }

    std::vector<int> track;
    int lane;
    while (file >> lane) {
        if (lane < -1 || lane >= kLaneCount) {
            throw std::runtime_error("Track contains an invalid lane number.");
        }
        track.push_back(lane);
    }
    if (!file.eof()) {
        throw std::runtime_error("Track file must contain only lane numbers.");
    }
    if (track.empty()) {
        throw std::runtime_error("Track file is empty.");
    }
    return track;
}

void drawTrack(const std::vector<int>& track, std::size_t position, int carLane,
               int frameOffset, int speedPercent) {
    std::ostringstream frame;
    frame << "\x1b[H";
    frame << kHudColor << "\n  MINI RACING  " << kReset
              << kHighlightColor << "  |  SECTION " << position << " / "
              << track.size() << "  |  SPEED " << speedPercent << "%"
              << kReset << "\n";
    frame << kCarColor << "  CAR DIRECTION: FORWARD ^" << kReset << "\n";
    const char* obstacleRows[] = {
        "   +-----+   ",
        "   | ### |   ",
        "   +-----+   ",
    };
    const char* carRows[] = {
        "      ^      ",
        "     /A\\     ",
        "    /___\\    ",
    };
    const std::string emptyCell(13, ' ');
    frame << kRoadColor << "+-------------+-------------+-------------+" << kReset << "\n";
    const std::size_t end = std::min(track.size(), position + kVisibleSections);
    for (int row = 0; row < kVisibleSections * kRowsPerSection; ++row) {
        std::vector<std::string> laneContents(kLaneCount, emptyCell);
        for (std::size_t section = position; section < end; ++section) {
            if (track[section] < 0) {
                continue;
            }
            const int sectionRow =
                (kVisibleSections - 1 - static_cast<int>(section - position)) *
                    kRowsPerSection +
                frameOffset;
            const int obstacleRow = row - sectionRow;
            if (obstacleRow >= 0 && obstacleRow < kRowsPerSection) {
                laneContents[track[section]] = obstacleRows[obstacleRow];
            }
        }

        frame << kRoadColor << "|" << kReset;
        for (const std::string& laneContent : laneContents) {
            if (laneContent == emptyCell) {
                frame << "      :      ";
            } else {
                frame << kObstacleColor << laneContent << kReset;
            }
            frame << kRoadColor << "|" << kReset;
        }
        frame << "\n";
    }
    frame << kRoadColor << "+-------------+-------------+-------------+" << kReset
          << "\n";
    for (int row = 0; row < 3; ++row) {
        frame << kRoadColor << "|" << kReset;
        for (int lane = 0; lane < kLaneCount; ++lane) {
            if (carLane == lane) {
                frame << kCarColor << carRows[row] << kReset;
            } else {
                frame << emptyCell;
            }
            frame << kRoadColor << "|" << kReset;
        }
        frame << "\n";
    }
    frame << kRoadColor << "+-------------+-------------+-------------+" << kReset
          << "\n";
    frame << kCarColor << "  A = your car  " << kReset
              << kObstacleColor << "  # = obstacle" << kReset
              << "  " << kHighlightColor << "^ = forward" << kReset << "\n";
    frame << "  Steer: " << kHighlightColor << "LEFT / RIGHT"
              << kReset << " or " << kHighlightColor << "A / D" << kReset
              << "     Quit: " << kHighlightColor << "Q" << kReset << "\n";
    frame << kRoadColor
              << "  The car moves automatically; steer any time without pressing Enter."
              << kReset << "\n\x1b[J";
    std::cout << frame.str() << std::flush;
}

void saveScore(const std::string& name, std::size_t score) {
    std::ofstream file("scores.txt", std::ios::app);
    if (!file) {
        throw std::runtime_error("Could not open scores.txt to save your score.");
    }
    file << name << "," << score << "\n";
    if (!file) {
        throw std::runtime_error("Failed while writing your score.");
    }
}
}  // namespace

int main() {
    try {
        enableVirtualTerminal();
        const std::vector<int> track = loadTrack("track.txt");
        std::string name;
        std::cout << "=== MINI RACING GAME ===\nDriver name: ";
        std::getline(std::cin, name);
        if (name.empty()) {
            name = "Driver";
        }
        std::replace(name.begin(), name.end(), ',', ' ');

        std::mt19937 randomEngine(std::random_device{}());
        bool playAgain = true;
        while (playAgain) {
            if (!waitForStart()) {
                break;
            }

            std::vector<int> raceTrack =
                generateRaceTrack(track.size(), randomEngine);
            int carLane = 1;
            std::size_t position = 0;
            bool crashed = false;
            const auto raceStart = std::chrono::steady_clock::now();

            while (position < track.size()) {
                bool quit = false;
                for (int frameOffset = 0; frameOffset < kRowsPerSection;
                     ++frameOffset) {
                    const int frameMs = frameDurationMs(
                        std::chrono::steady_clock::now() - raceStart);
                    const int speedPercent =
                        kStartingFrameDurationMs * 100 / frameMs;
                    drawTrack(raceTrack, position, carLane, frameOffset,
                              speedPercent);
                    const auto nextFrame =
                        std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(frameMs);
                    while (std::chrono::steady_clock::now() < nextFrame) {
                        const char command = readCommand();
                        if (command == 'q') {
                            quit = true;
                            break;
                        }

                        if (command == 'a' && carLane > 0) {
                            --carLane;
                        } else if (command == 'd' &&
                                   carLane < kLaneCount - 1) {
                            ++carLane;
                        }
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(5));
                    }
                    if (quit) {
                        break;
                    }
                }
                if (quit) {
                    break;
                }

                if (raceTrack[position] == carLane) {
                    crashed = true;
                    break;
                }
                ++position;
            }

            const std::size_t score = position;
            if (crashed) {
                std::cout << "\nCrash! You cleared " << score << " sections.\n";
            } else if (position == raceTrack.size()) {
                std::cout << "\nFinish! You cleared all " << score << " sections.\n";
            } else {
                std::cout << "\nRace ended. You cleared " << score << " sections.\n";
            }
            saveScore(name, score);
            std::cout << "Score saved to scores.txt. Run Leaderboard.java to view scores.\n";

            if (!playAgain) {
                break;
            }
            while (true) {
                std::cout << "Play again? (Y/N, no Enter): " << std::flush;
                char answer = '\0';
                while (answer == '\0') {
                    answer = readCommand();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                std::cout << answer << "\n";
                if (answer == 'y') {
                    break;
                }
                if (answer == 'n') {
                    playAgain = false;
                    break;
                }
                std::cout << "Enter Y or N.\n";
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
