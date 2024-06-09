#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

void printErrorAndExit();
bool isValidStrategy(const std::string &strategy);
void printBoard(const std::vector<char> &board);
bool checkWin(const std::vector<char> &board, char player);
