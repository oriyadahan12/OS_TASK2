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

// Function to print an error message and exit the program
void printErrorAndExit() {
    std::cerr << "Error\n";
    exit(1);
}

// Function to handle server-side TCP input
void handleServerInput(int port, int &input_fd) {
    // Create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        printErrorAndExit();
    }

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Set socket options to allow address reuse
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printErrorAndExit();
    }

    // Set up the address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printErrorAndExit();
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        printErrorAndExit();
    }

    // Accept a connection
    input_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (input_fd < 0) {
        printErrorAndExit();
    }
}

// Function to handle client-side TCP output
void handleClientOutput(const std::string &host, int port, int &output_fd) {
    // Create a socket
    output_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (output_fd < 0) {
        printErrorAndExit();
    }

    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Get the server's address by hostname
    server = gethostbyname(host.c_str());
    if (server == nullptr) {
        printErrorAndExit();
    }

    // Set up the address structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(output_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printErrorAndExit();
    }
}

// Function to redirect input and output
void redirectIO(int input_fd, int output_fd) {
    if (dup2(input_fd, STDIN_FILENO) < 0) {
        printErrorAndExit();
    }
    if (dup2(output_fd, STDOUT_FILENO) < 0) {
        printErrorAndExit();
    }
    if (dup2(output_fd, STDERR_FILENO) < 0) {
        printErrorAndExit();
    }
}

int main(int argc, char *argv[]) {
    // Ensure there are at least three arguments
    if (argc < 3) {
        printErrorAndExit();
    }

    std::string exec_command;
    int input_fd = -1, output_fd = -1;
    bool input_tcp = false, output_tcp = false;
    std::string input_host, output_host;
    int input_port = -1, output_port = -1;

    // Parse command-line arguments
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
                printErrorAndExit();
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
                    printErrorAndExit();
                }
            } else {
                printErrorAndExit();
            }
        } else if (arg == "-b" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 4) == "TCPS") {
                input_tcp = true;
                output_tcp = true;
                input_port = std::stoi(param.substr(4));
                output_port = input_port; // Use the same port for bi-directional
            } else {
                printErrorAndExit();
            }
        } else {
            printErrorAndExit();
        }
    }

    // Handle TCP input if specified
    if (input_tcp) {
        handleServerInput(input_port, input_fd);
    }

    // Handle TCP output if specified
    if (output_tcp) {
        handleClientOutput(output_host, output_port, output_fd);
    }

    // If no command to execute, transfer data between input and output
    if (exec_command.empty()) {
        if (input_tcp) {
            char buffer[1024];
            ssize_t bytes_read;
            while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
                write(STDOUT_FILENO, buffer, bytes_read);
            }
        } else if (output_tcp) {
            char buffer[1024];
            ssize_t bytes_read;
            while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
                write(output_fd, buffer, bytes_read);
            }
        } else {
            char buffer[1024];
            ssize_t bytes_read;
            while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
                write(STDOUT_FILENO, buffer, bytes_read);
            }
        }
    } else {
        // Fork a new process to execute the command
        pid_t pid = fork();
        if (pid < 0) {
            printErrorAndExit();
        } else if (pid == 0) {
            // Redirect input and output in the child process
            if (input_fd >= 0 && output_fd >= 0) {
                redirectIO(input_fd, output_fd);
            } else if (input_fd >= 0) {
                dup2(input_fd, STDIN_FILENO);
            } else if (output_fd >= 0) {
                dup2(output_fd, STDOUT_FILENO);
            }
            // Execute the command
            execl("/bin/sh", "sh", "-c", exec_command.c_str(), (char *)0);
            printErrorAndExit();
        } else {
            // Wait for the child process to finish
            wait(NULL);
        }
    }

    // Close the file descriptors
    if (input_fd >= 0) close(input_fd);
    if (output_fd >= 0) close(output_fd);

    return 0;
}
