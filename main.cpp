// Original code provided by Professor Ole-Christoffer Granmo (2024) in the course IKT457 Learning Systems at University of Agder:
// Copyright (c) 2024 Ole-Christoffer Granmo

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Modifications made by Jon A B Larssen to extend functionality for logging,
// generating CSV data, and analyzing unique games. New functionality includes:
// - Expanded board initialization and flexible dimension handling.
// - Game move logging, allowing for removal of the last N moves.
// - Generation and saving of board states in CSV format in both "coord" and regular formats.
// - Metadata analysis of games including winner statistics, unique game counting, and move removal logging.
// - Directory and file management to ensure data is saved correctly.


#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <unordered_set>    // For unique game detection
#include <string>
#include <sys/stat.h>       // For directory creation
#include <filesystem>       // Required for checking if file exists (C++17 and later)


class HexGame {
public:
    int BOARD_DIM;
    std::vector<int> board;
    std::vector<int> open_positions;
    int number_of_open_positions;
    std::vector<int> moves;
    std::vector<int> connected;
    std::vector<int> neighbors;

    HexGame(int dim) : BOARD_DIM(dim) {
        board.resize((BOARD_DIM+2)*(BOARD_DIM+2)*2);
        open_positions.resize(BOARD_DIM*BOARD_DIM);
        moves.clear();
        moves.resize(BOARD_DIM*BOARD_DIM);
        connected.resize((BOARD_DIM+2)*(BOARD_DIM+2)*2);
        neighbors = {-(BOARD_DIM+2) + 1, -(BOARD_DIM+2), -1, 1, (BOARD_DIM+2), (BOARD_DIM+2) - 1};
        init();
    }

    void init() {
        for (int i = 0; i < BOARD_DIM+2; ++i) {
            for (int j = 0; j < BOARD_DIM+2; ++j) {
                board[(i*(BOARD_DIM + 2) + j) * 2] = 0;
                board[(i*(BOARD_DIM + 2) + j) * 2 + 1] = 0;

                if (i > 0 && i < BOARD_DIM + 1 && j > 0 && j < BOARD_DIM + 1) {
                    open_positions[(i-1)*BOARD_DIM + j - 1] = i*(BOARD_DIM + 2) + j;
                }

                if (i == 0) {
                    connected[(i*(BOARD_DIM + 2) + j) * 2] = 1;
                } else {
                    connected[(i*(BOARD_DIM + 2) + j) * 2] = 0;
                }

                if (j == 0) {
                    connected[(i*(BOARD_DIM + 2) + j) * 2 + 1] = 1;
                } else {
                    connected[(i*(BOARD_DIM + 2) + j) * 2 + 1] = 0;
                }
            }
        }
        number_of_open_positions = BOARD_DIM*BOARD_DIM;
        moves.clear();
    }

    int connect(int player, int position) {
        connected[position*2 + player] = 1;

        if (player == 0 && position / (BOARD_DIM + 2) == BOARD_DIM) {
            return 1;
        }

        if (player == 1 && position % (BOARD_DIM + 2) == BOARD_DIM) {
            return 1;
        }

        for (int i = 0; i < 6; ++i) {
            int neighbor = position + neighbors[i];
            if (board[neighbor*2 + player] && !connected[neighbor*2 + player]) {
                if (connect(player, neighbor)) {
                    return 1;
                }
            }
        }
        return 0;
    }

    int winner(int player, int position) {
        for (int i = 0; i < 6; ++i) {
            int neighbor = position + neighbors[i];
            if (connected[neighbor*2 + player]) {
                return connect(player, position);
            }
        }
        return 0;
    }

    int place_piece_randomly(int player) {
        int random_empty_position_index = rand() % number_of_open_positions;

        int empty_position = open_positions[random_empty_position_index];

        board[empty_position * 2 + player] = 1;

        // Convert expanded board index to logical grid index
        int logical_row = (empty_position / (BOARD_DIM + 2)) - 1;  // Convert row index
        int logical_col = (empty_position % (BOARD_DIM + 2)) - 1;  // Convert col index
        int logical_position = logical_row * BOARD_DIM + logical_col;  // Convert to logical 1D index

        moves.push_back(logical_position);  // Store the logical index for the move

        open_positions[random_empty_position_index] = open_positions[number_of_open_positions - 1];
        number_of_open_positions--;

        return empty_position;
    }

    bool full_board() {
        return number_of_open_positions == 0;
    }

    // Remove the last `n` moves from the board and return the removed moves
    std::vector<int> remove_last_n_moves(int n) {
        std::vector<int> removed_moves;
        for (int i = 0; i < n; ++i) {
            int last_move_position = moves.back();  // Get the last logical move
            removed_moves.push_back(last_move_position);  // Track the removed move
            moves.pop_back();  // Remove it from the move list

            // Convert logical index back to expanded board index to clear
            int logical_row = last_move_position / BOARD_DIM;
            int logical_col = last_move_position % BOARD_DIM;
            int expanded_index = (logical_row + 1) * (BOARD_DIM + 2) + (logical_col + 1);

            // Clear the board at that expanded position
            board[expanded_index * 2] = 0;      // Clear player X
            board[expanded_index * 2 + 1] = 0;  // Clear player O
        }
        return removed_moves;  // Return the removed logical moves
    }

    // Function to generate a board string in the specified format
    std::string board_to_string() {
        std::ostringstream board_string;
        for (int i = 0; i < BOARD_DIM; ++i) {
            for (int j = 0; j < BOARD_DIM; ++j) {
                if (board[((i+1)*(BOARD_DIM+2) + j + 1)*2] == 1) {
                    board_string << "X";
                } else if (board[((i+1)*(BOARD_DIM+2) + j + 1)*2 + 1] == 1) {
                    board_string << "O";
                } else {
                    board_string << " ";
                }
            }
        }
        return board_string.str();
    }

    // Function to generate a vector of board values for coord format
    std::vector<int> board_to_coord() {
        std::vector<int> coord_values;
        for (int i = 0; i < BOARD_DIM; ++i) {
            for (int j = 0; j < BOARD_DIM; ++j) {
                int cell_x = board[((i+1)*(BOARD_DIM+2) + j + 1)*2];      // player X
                int cell_o = board[((i+1)*(BOARD_DIM+2) + j + 1)*2 + 1];  // player O
                int cell_value = cell_x == 1 ? 1 : (cell_o == 1 ? -1 : 0);    // 1 for X, -1 for O, 0 for empty
                coord_values.push_back(cell_value);
            }
        }
        return coord_values;
    }

    // Function to write a game to CSV in either "coord" or regular format
    void write_game_to_csv(std::ofstream &outfile, const std::string &format, const std::pair<std::string, int>& result) {
        if (format == "coord") {
            // This should not be called for coord format
            return;
        }
        // Write the board string
        outfile << result.first << "," << result.second << "\n";


    }

    void write_coord_game_to_csv(std::ofstream &outfile, const std::vector<int>& board_values, int starting_player, int winner) {
        for (int value : board_values) {
            outfile << value << ",";
        }
        // Write the winner at the end
        outfile << starting_player << "," << winner << "\n";
    }

    void print() {
        for (int i = 0; i < BOARD_DIM; ++i) {
            for (int j = 0; j < i; j++) {
                std::cout << " ";
            }

            for (int j = 0; j < BOARD_DIM; ++j) {
                if (board[((i+1)*(BOARD_DIM+2) + j + 1)*2] == 1) {
                    std::cout << " X";
                } else if (board[((i+1)*(BOARD_DIM+2) + j + 1)*2 + 1] == 1) {
                    std::cout << " O";
                } else {
                    std::cout << " .";
                }
            }
            std::cout << std::endl;
        }
    }
};

// Function to generate a unique timestamp for the filename
std::string generate_timestamp(bool detailed = false) {
    // Get current time
    std::time_t t = std::time(nullptr);
    std::tm* time_ptr = std::localtime(&t);

    std::ostringstream oss;

    if (detailed) {
        // If detailed timestamp is requested: include year, month, day, hour, minute, second
        oss << std::put_time(time_ptr, "%Y%m%d:%H%M%S");

        // Add milliseconds for more precision
        auto now = std::chrono::system_clock::now();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        oss << "." << std::setw(3) << std::setfill('0') << milliseconds.count();
    } else {
        // Simpler timestamp with hour, minute, second
        oss << std::put_time(time_ptr, "%H%M%S");  // Hour, minute, second
    }

    return oss.str();
}

// Function to analyze the dataset and return metadata for documentation
std::tuple<int, int, int, int> analyze_game_file(const std::string &filename, const std::string &format) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Failed to open the file: " << filename << std::endl;
        return {0, 0, 0, 0};
    }

    std::string line;
    int wins_player_X = 0;
    int wins_player_O = 0;
    int total_games = 0;

    // Use an unordered_set to track unique games
    std::unordered_set<std::string> unique_games;

    // Read and process the file
    bool first_line = true;  // Skip the first line (header)
    while (std::getline(infile, line)) {
        if (first_line) {
            first_line = false;
            continue;
        }

        std::string board_state;
        std::string winner_str;
        if (format == "coord") {
            // Extract the board state and winner
            std::stringstream ss(line);
            std::string cell;
            board_state.clear();

            // Concatenate all cell values for uniqueness
            while (std::getline(ss, cell, ',')) {
                board_state += cell;
            }
            winner_str = board_state.back();  // Last value is the winner
            board_state.pop_back();  // Remove the winner from the state

        } else {
            // Non-coord format: extract board state and winner
            std::stringstream ss(line);
            std::getline(ss, board_state, ',');
            std::getline(ss, winner_str, ',');
        }

        // Add the board state to the set of unique games
        unique_games.insert(board_state);

        // Increment the winner count
        int winner = std::stoi(winner_str);
        if (winner == 0) {
            wins_player_X++;
        } else if (winner == 1) {
            wins_player_O++;
        }

        total_games++;
    }

    infile.close();

    int unique_games_count = unique_games.size();

    // Return total games, unique games, wins for player X and O
    return {total_games, unique_games_count, wins_player_X, wins_player_O};
}

// Function to save metadata including removed moves to a separate CSV file
void save_metadata_with_removed_moves(const std::string &metadata_filename, const std::string &dataset_filename,
                                      int board_dim, int total_games, int unique_games, int wins_player_X,
                                      int wins_player_O, const std::string &format,
                                      const std::vector<std::vector<int>>& removed_moves_per_game, int moves_before_end) {
    std::ofstream outfile(metadata_filename);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open metadata file: " << metadata_filename << std::endl;
        return;
    }

    // Write metadata header
    outfile << "Filename,Board Dimension,Total Games,Unique Games,Player X Wins,Player O Wins,Format,Timestamp,Moves Before End,Removed Moves\n";

    // Write metadata content
    outfile << dataset_filename << ","
            << board_dim << "x" << board_dim << ","
            << total_games << ","
            << unique_games << ","
            << wins_player_X << ","
            << wins_player_O << ","
            << format << ","
            << moves_before_end << ",";

    // Write removed moves for each game
    outfile << "{";
    for (const auto& removed_moves : removed_moves_per_game) {
        outfile << "{";
        for (size_t i = 0; i < removed_moves.size(); ++i) {
            outfile << removed_moves[i];  // Already in correct logical index form
            if (i != removed_moves.size() - 1) {
                outfile << ",";
            }
        }
        outfile << "}";
        if (&removed_moves != &removed_moves_per_game.back()) {
            outfile << ",";
        }
    }
    outfile << "}\n";

    outfile.close();
}

// Helper function to create a directory if it doesn't exist
bool ensure_directory_exists(const std::string &directory) {
    struct stat info;
    if (stat(directory.c_str(), &info) != 0) {
        // Directory does not exist, so create it
        std::cout << "Creating directory: " << directory << std::endl;
        #ifdef _WIN32
            mkdir(directory.c_str());
        #else
            mkdir(directory.c_str(), 0777);  // UNIX-style
        #endif
        return true;
    }
    return false;
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);  // Disable output buffering
    srand(time(nullptr));

    // Ensure 'data' and 'metadata' directories exist
    ensure_directory_exists("B:\\TsetlinModels\\data");
    ensure_directory_exists("B:\\TsetlinModels\\metadata");

    std::string format = "coord";


    int total_games_list[] = {2000, 20000, 200000};
    int min_board_dim = 5;
    int max_board_dim = 15;
    float open_pos_list[] = {0.1,0.2,0.3,0.4};
    int mbf_list[] = {0,2,5};

    for (int board_dim = min_board_dim; board_dim <= max_board_dim; ++board_dim) {

        for (int open_pos_index = 0; open_pos_index < 5; open_pos_index++) {
            float n_open_pos = open_pos_list[open_pos_index];

            for (int total_games_index = 0; total_games_index < 3; ++total_games_index) {
                int total_games = total_games_list[total_games_index];

                for (int mbf_index = 0; mbf_index < 3; mbf_index++) {
                    int moves_before_end = mbf_list[mbf_index];

                    HexGame hg(board_dim);
                    int open_pos = board_dim * board_dim * n_open_pos;

                    // Measure time before saving the file
                    auto start = std::chrono::high_resolution_clock::now();

                    bool file_created = false;
                    std::string filename;

                    // Create the filename ONCE per combination of board_dim, total_games, n_open_pos, etc.
                    filename = "B:\\TsetlinModels\\data\\";
                    filename += std::to_string(board_dim);
                    filename += "x" + std::to_string(board_dim);
                    filename += "_" + std::to_string(total_games);
                    filename += "_" + std::to_string(static_cast<int>(n_open_pos * 100));
                    filename += "_" + std::to_string(moves_before_end);
                    filename += ".csv";
                    std::cout << "Constructed filename: " << filename;

                    if (std::filesystem::exists(filename)) {
                        std::cout << " - File exists, skipping..." << std::endl;
                        continue;  // Skip to the next iteration if file exists
                    }

                    // Create and open the file once for writing header
                    std::ofstream outfile(filename);
                    if (outfile.is_open()) {
                        if (format == "coord") {
                            for (int i = 0; i < board_dim; ++i) {
                                for (int j = 0; j < board_dim; ++j) {
                                    outfile << "cell" << i << "_" << j << ",";
                                }
                            }
                            outfile << "starting_player,winner" << std::endl;
                        } else {
                            outfile << "board,starting_player,winner\n";
                        }
                        outfile.close();
                        file_created = true;
                    } else {
                        std::cerr << "Error opening file: " << filename << std::endl;
                        continue;  // Skip this combination if the file could not be created
                    }

                    int valid_games = 0;
                    int batch_size = total_games / 1;
                    int empty_runs = 0;
                    std::vector<std::pair<std::string, std::pair<int, int>>> game_results_string;
                    std::vector<std::pair<std::vector<int>, std::pair<int, int>>> game_results_coord;
                    std::vector<std::vector<int>> removed_moves_per_game;

                    // Process the games
                    while (valid_games < total_games) {
                        hg.init();
                        int starting_player = rand() % 2;  // 0 for Player X, 1 for Player O
                        int player = starting_player;
                        int winner = -1;

                        // Simulate the game
                        while (!hg.full_board()) {
                            int position = hg.place_piece_randomly(player);

                            if (hg.winner(player, position)) {
                                winner = player;
                                break;
                            }
                            player = 1 - player;
                        }

                        if (hg.number_of_open_positions >= open_pos) {
                            // Valid game, remove last moves and store results
                            std::vector<int> removed_moves = hg.remove_last_n_moves(moves_before_end);
                            removed_moves_per_game.push_back(removed_moves);  // Track removed moves

                            std::pair<int, int> outcome = {starting_player, winner};

                            if (format == "coord") {
                                game_results_coord.emplace_back(hg.board_to_coord(), outcome);
                            } else {
                                game_results_string.emplace_back(hg.board_to_string(), outcome);
                            }

                            valid_games++;

                            // Write results to file in batches
                            if (format == "coord" && game_results_coord.size() >= batch_size) {
                                std::ofstream outfile(filename, std::ios::app);
                                for (const auto& result : game_results_coord) {
                                    hg.write_coord_game_to_csv(outfile, result.first, result.second.first, result.second.second);
                                }
                                game_results_coord.clear();
                                outfile.close();
                                std::cout << " - Writing to " << board_dim << "x" << board_dim;

                                // Measure time after writing
                                auto end = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double> elapsed = end - start;

                                // Calculate hours, minutes, and seconds
                                int hours = static_cast<int>(elapsed.count() / 3600);
                                int minutes = static_cast<int>((elapsed.count() - (hours * 3600)) / 60);
                                int seconds = static_cast<int>(elapsed.count()) % 60;

                                // Get the current system time
                                auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                                std::tm* time_info = std::localtime(&current_time);

                                std::cout << " - " << std::setw(2) << std::setfill('0') << time_info->tm_hour
                                          << ":" << std::setw(2) << std::setfill('0') << time_info->tm_min
                                          << ":" << std::setw(2) << std::setfill('0') << time_info->tm_sec;

                                // Format and display elapsed time and current time
                                std::cout << " - " << std::setw(2) << std::setfill('0') << hours
                                          << ":" << std::setw(2) << std::setfill('0') << minutes
                                          << ":" << std::setw(2) << std::setfill('0') << seconds << std::endl;



                            } else if (game_results_string.size() >= batch_size) {
                                std::ofstream outfile(filename, std::ios::app);
                                for (const auto& result : game_results_string) {
                                    hg.write_game_to_csv(outfile, format, {result.first, result.second.second});  // Only winner passed
                                    outfile << result.second.first << "," << result.second.second << "\n";  // Append starting player
                                }
                                game_results_string.clear();
                                outfile.close();
                                std::cout << "2 Writing to " << board_dim << "x" << board_dim << std::endl;
                            }
                        } else {
                            empty_runs++;
                        }
                    }

                    // Write remaining results at the end
                    if (!game_results_coord.empty() && format == "coord") {
                        std::ofstream outfile(filename, std::ios::app);
                        for (const auto& result : game_results_coord) {
                            hg.write_coord_game_to_csv(outfile, result.first, result.second.first, result.second.second);
                        }
                        outfile.close();
                        std::cout << "3 Writing to " << board_dim << "x" << board_dim << std::endl;
                    } else if (!game_results_string.empty()) {
                        std::ofstream outfile(filename, std::ios::app);
                        for (const auto& result : game_results_string) {
                            hg.write_game_to_csv(outfile, format, {result.first, result.second.second});
                            outfile << result.second.first << "," << result.second.second << "\n";
                        }
                        outfile.close();
                        std::cout << "4 Writing to " << board_dim << "x" << board_dim << std::endl;
                    }

                    // Analyze the file to get metadata
                    if (!filename.empty()) {
                        auto [total_games, unique_games, wins_player_X, wins_player_O] = analyze_game_file(filename, format);
                        std::string metadata_filename = "B:\\TsetlinModels\\metadata\\metadata_" + filename.substr(filename.find_last_of("\\") + 1);
                        std::cout << "Metadata filename: " << metadata_filename << std::endl;

                        //std::string detailed_timestamp = generate_timestamp(true);
                        save_metadata_with_removed_moves(metadata_filename, filename, board_dim, total_games, unique_games, wins_player_X, wins_player_O, format, removed_moves_per_game, moves_before_end);
                    }
                }
            }
        }
    }
    return 0;
}
