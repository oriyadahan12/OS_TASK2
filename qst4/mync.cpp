#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <csignal>
#include <sys/wait.h>
#include <sys/socket.h>

/**
 * Prints an error message to stderr and exits the program with a failure status.
 * @param message The error message to print.
 */
void printErrorAndExit(const std::string &message) {
    std::cerr << "Error: " << message << std::endl;
    exit(EXIT_FAILURE);
}

/**
 * Signal handler for timeout. Prints an error message and exits.
 * @param sig The signal number.
 */
void handleTimeout(int sig) {
    (void)sig; // To avoid unused parameter warning
    printErrorAndExit("Timeout reached, exiting.");
}

/**
 * Sets up the server side to handle input from a TCP or UDP client.
 * @param port The port number to listen on.
 * @param input_fd Reference to the input file descriptor.
 * @param is_udp Flag to indicate if the server should use UDP (true) or TCP (false).
 */
void handleServerInput(int port, int &input_fd, bool is_udp) {
    struct sockaddr_in server_addr;

    if (is_udp) {
        input_fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        input_fd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (input_fd < 0) {
        printErrorAndExit("Failed to create socket: " + std::string(strerror(errno)));
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(input_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printErrorAndExit("Failed to bind socket: " + std::string(strerror(errno)));
    }

    if (!is_udp) {
        if (listen(input_fd, 1) < 0) {
            printErrorAndExit("Failed to listen on socket: " + std::string(strerror(errno)));
        }
        int client_fd = accept(input_fd, nullptr, nullptr);
        if (client_fd < 0) {
            printErrorAndExit("Failed to accept connection: " + std::string(strerror(errno)));
        }
        close(input_fd);
        input_fd = client_fd;
    }
}

/**
 * Sets up the client side to send output to a TCP or UDP server.
 * @param host The server hostname or IP address.
 * @param port The port number to connect to.
 * @param output_fd Reference to the output file descriptor.
 * @param is_udp Flag to indicate if the client should use UDP (true) or TCP (false).
 */
void handleClientOutput(const std::string &host, int port, int &output_fd, bool is_udp) {
    struct sockaddr_in server_addr;
    struct hostent *server = gethostbyname(host.c_str());

    if (server == nullptr) {
        printErrorAndExit("Failed to get host by name");
    }

    if (is_udp) {
        output_fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        output_fd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (output_fd < 0) {
        printErrorAndExit("Failed to create socket");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    if (connect(output_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printErrorAndExit("Failed to connect to server");
    }
}

/**
 * Redirects the standard input (stdin) to the given file descriptor.
 * @param fd The file descriptor to redirect stdin to.
 */
void redirectInput(int fd) {
    if (dup2(fd, STDIN_FILENO) < 0) {
        printErrorAndExit("Failed to redirect input");
    }
}

/**
 * Redirects the standard output (stdout) to the given file descriptor.
 * @param fd The file descriptor to redirect stdout to.
 */
void redirectOutput(int fd) {
    if (dup2(fd, STDOUT_FILENO) < 0) {
        printErrorAndExit("Failed to redirect output");
    }
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printErrorAndExit("Invalid number of arguments");
    }

    std::string executable;
    bool input_tcp = false, output_tcp = false, input_udp = false, output_udp = false;
    int input_port = -1, output_port = -1, timeout = -1;
    int input_fd = -1, output_fd = -1;
    std::string output_host;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-e" && i + 1 < argc) {
            executable = argv[++i];
        } else if (arg == "-i" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 4) == "TCPS") {
                input_tcp = true;
                input_port = std::stoi(param.substr(4));
            } else if (param.substr(0, 4) == "UDPS") {
                input_udp = true;
                input_port = std::stoi(param.substr(4));
            } else {
                printErrorAndExit("Invalid input parameter");
            }
        } else if (arg == "-o" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 4) == "TCPC") {
                output_tcp = true;
                size_t comma = param.find(',', 4);
                if (comma == std::string::npos) {
                    printErrorAndExit("Invalid TCP client parameter");
                }
                output_host = param.substr(4, comma - 4);
                output_port = std::stoi(param.substr(comma + 1));
            } else if (param.substr(0, 4) == "UDPC") {
                output_udp = true;
                size_t comma = param.find(',', 4);
                if (comma == std::string::npos) {
                    printErrorAndExit("Invalid UDP client parameter");
                }
                output_host = param.substr(4, comma - 4);
                output_port = std::stoi(param.substr(comma + 1));
            } else {
                printErrorAndExit("Invalid output parameter");
            }
        } else if (arg == "-b" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 4) == "TCPS") {
                input_tcp = true;
                output_tcp = true;
                input_port = std::stoi(param.substr(4));
                output_port = input_port;
            } else {
                printErrorAndExit("Invalid bi-directional parameter");
            }
        } else if (arg == "-t" && i + 1 < argc) {
            timeout = std::stoi(argv[++i]);
        } else {
            printErrorAndExit("Invalid parameter");
        }
    }

    if (executable.empty()) {
        printErrorAndExit("Executable not specified");
    }

    if (input_tcp || input_udp) {
        handleServerInput(input_port, input_fd, input_udp);
    }

    if (output_tcp || output_udp) {
        handleClientOutput(output_host, output_port, output_fd, output_udp);
    }

    if (input_tcp || input_udp) {
        redirectInput(input_fd);
    }

    if (output_tcp || output_udp) {
        redirectOutput(output_fd);
    }

    if (timeout > 0) {
        signal(SIGALRM, handleTimeout);
        alarm(timeout);
    }

    int pid = fork();
    if (pid < 0) {
        printErrorAndExit("Failed to fork process");
    } else if (pid == 0) {
        execl("/bin/sh", "sh", "-c", executable.c_str(), nullptr);
        printErrorAndExit("Failed to execute program");
    } else {
        int status;
        wait(&status);
        if (input_fd > 0) close(input_fd);
        if (output_fd > 0) close(output_fd);
    }

    return 0;
}
