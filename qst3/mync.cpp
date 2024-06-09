#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <csignal>
#include <sys/wait.h> 

void printErrorAndExit(const std::string &message) {
    std::cerr << "Error: " << message << std::endl;
    exit(1);
}

void handleServerInput(int port, int &input_fd) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printErrorAndExit("Failed to create server socket");
    }

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printErrorAndExit("Failed to set socket options");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printErrorAndExit("Failed to bind server socket");
    }

    if (listen(server_fd, 3) < 0) {
        printErrorAndExit("Failed to listen on server socket");
    }

    input_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (input_fd < 0) {
        printErrorAndExit("Failed to accept connection on server socket");
    }
}

void handleClientOutput(const std::string &host, int port, int &output_fd) {
    output_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (output_fd < 0) {
        printErrorAndExit("Failed to create client socket");
    }

    struct sockaddr_in serv_addr;
    struct hostent *server;

    server = gethostbyname(host.c_str());
    if (server == nullptr) {
        printErrorAndExit("Failed to get host by name");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(output_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printErrorAndExit("Failed to connect to server");
    }
}

void redirectIO(int input_fd, int output_fd) {
    if (dup2(input_fd, STDIN_FILENO) < 0) {
        printErrorAndExit("Failed to redirect standard input");
    }
    if (dup2(output_fd, STDOUT_FILENO) < 0) {
        printErrorAndExit("Failed to redirect standard output");
    }
    if (dup2(output_fd, STDERR_FILENO) < 0) {
        printErrorAndExit("Failed to redirect standard error");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printErrorAndExit("Insufficient arguments");
    }

    std::string exec_command;
    int input_fd = -1, output_fd = -1;
    bool input_tcp = false, output_tcp = false;
    std::string input_host, output_host;
    int input_port = -1, output_port = -1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-e" && i + 1 < argc) {
            exec_command = argv[++i];
        } else if (arg == "-i" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 4) == "TCPS") {
                input_tcp = true;
                input_port = std::stoi(param.substr(4));
            } else {
                printErrorAndExit("Invalid input parameter");
            }
        } else if (arg == "-o" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 4) == "TCPC") {
                output_tcp = true;
                size_t pos = param.find(',');
                if (pos != std::string::npos) {
                    output_host = param.substr(4, pos - 4);
                    output_port = std::stoi(param.substr(pos + 1));
                } else {
                    printErrorAndExit("Invalid output parameter");
                }
            } else {
                printErrorAndExit("Invalid output parameter");
            }
        } else if (arg == "-b" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 4) == "TCPS") {
                input_tcp = true;
                output_tcp = true;
                input_port = std::stoi(param.substr(4));
                output_port = input_port; // Use the same port for bi-directional
            } else {
                printErrorAndExit("Invalid bi-directional parameter");
            }
        } else {
            printErrorAndExit("Invalid parameter");
        }
    }

    if (exec_command.empty()) {
        printErrorAndExit("No executable command specified");
    }

    std::cout << "Executing command: " << exec_command << std::endl;

    if (input_tcp) {
        std::cout << "Handling server input on port: " << input_port << std::endl;
        handleServerInput(input_port, input_fd);
    }

    if (output_tcp) {
        std::cout << "Handling client output to host: " << output_host << " port: " << output_port << std::endl;
        handleClientOutput(output_host, output_port, output_fd);
    }

    pid_t pid = fork();
    if (pid < 0) {
        printErrorAndExit("Failed to fork process");
    } else if (pid == 0) {
        if (input_fd >= 0 && output_fd >= 0) {
            redirectIO(input_fd, output_fd);
        } else if (input_fd >= 0) {
            dup2(input_fd, STDIN_FILENO);
        } else if (output_fd >= 0) {
            dup2(output_fd, STDOUT_FILENO);
        }
        execl("/bin/sh", "sh", "-c", exec_command.c_str(), (char *)0);
        printErrorAndExit("Failed to exec command");
    } else {
        wait(NULL);
    }

    if (input_fd >= 0) close(input_fd);
    if (output_fd >= 0) close(output_fd);

    return 0;
}
