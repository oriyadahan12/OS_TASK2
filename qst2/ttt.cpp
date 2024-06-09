#include "ttt.hpp"

void printErrorAndExit() {
    std::cout << "Error\n";
    exit(1);
}

bool isValidStrategy(const std::string &strategy) {
    if (strategy.size() != 9) return false;
    std::vector<bool> digits(10, false);
    for (char c : strategy) {
        if (c < '1' || c > '9' || digits[c - '0']) return false;
        digits[c - '0'] = true;
    }
    return true;
}

void printBoard(const std::vector<char> &board) {
    for (int i = 0; i < 9; i += 3) {
        std::cout << " " << board[i] << " | " << board[i + 1] << " | " << board[i + 2] << "\n";
        if (i < 6) {
            std::cout << "---|---|---\n";
        }
    }
}

bool checkWin(const std::vector<char> &board, char player) {
    // Array of all possible winning combinations
    const int winningCombos[8][3] = {
        {0, 1, 2}, // Row 1
        {3, 4, 5}, // Row 2
        {6, 7, 8}, // Row 3
        {0, 3, 6}, // Column 1
        {1, 4, 7}, // Column 2
        {2, 5, 8}, // Column 3
        {0, 4, 8}, // Diagonal from top-left to bottom-right
        {2, 4, 6}  // Diagonal from top-right to bottom-left
    };

    // Check each winning combination
    for (const auto &combo : winningCombos) {
        // Check if all positions in the current combination have the player's symbol
        if (board[combo[0]] == player && board[combo[1]] == player && board[combo[2]] == player) {
            return true; // Player wins
        }
    }
    
    return false; // No winning combination found
}

int main(int argc, char *argv[]) {


    if (argc != 2) {
        printErrorAndExit();
    }

    std::string strategy = argv[1];
    if (!isValidStrategy(strategy)) {
        printErrorAndExit();
    }

    std::vector<char> board(9, ' ');
  
   
    while (true) { 
         printBoard(board);
        // Program's turn
        int programMove = -1;
        for (char c : strategy) {
            int slot = c - '1';
            if (board[slot] == ' ') {
                programMove = slot;
                break;
            }
        }
        if (std::count(board.begin(), board.end(), ' ') == 1) {
            for (char c : strategy) {
                int slot = c - '1';
                if (board[slot] == ' ') {
                    programMove = slot;
                    break;
                }
            }
        }

        board[programMove] = 'X';
        std::cout << programMove + 1 << "\n";
        if (checkWin(board, 'X')) {
            printBoard(board);
            std::cout << "I win\n";
            break;
        }
        if (std::count(board.begin(), board.end(), ' ') == 0) {
            printBoard(board);
            std::cout << "DRAW\n";
            break;
        }

        // Player's turn
        int playerMove;
        std::cin >> playerMove;
        --playerMove; // Adjust for 0-based index
        if (playerMove < 0 || playerMove >= 9 || board[playerMove] != ' ') {
            printErrorAndExit();
        }
        board[playerMove] = 'O';
        if (checkWin(board, 'O')) {
            printBoard(board);
            std::cout << "I lost\n";
            break;
        }
        if (std::count(board.begin(), board.end(), ' ') == 0) {
            std::cout << "DRAW\n";
            break;
        }
    }

    return 0;
}
