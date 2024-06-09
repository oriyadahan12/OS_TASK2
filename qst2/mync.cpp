#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

void printErrorAndExit() {
    std::cerr << "Error\n";
    exit(1);
}

std::vector<char*> parseCommand(const std::string& command) {
    std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    std::vector<char*> args;
    for (auto& t : tokens) {
        args.push_back(&t[0]);
    }
    args.push_back(nullptr);
    return args;
}

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "-e") {
        printErrorAndExit();
    }

    std::string command = argv[2];
    std::vector<char*> args = parseCommand(command);

    pid_t pid = fork();
    if (pid == -1) {
        printErrorAndExit();
    } else if (pid == 0) {
        // Child process
        if (execvp(args[0], args.data()) == -1) {
            printErrorAndExit();
        }
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            printErrorAndExit();
        }
    }

    return 0;
}
