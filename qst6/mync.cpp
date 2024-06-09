#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <csignal>
#include <netinet/in.h>
#include <sys/wait.h>

// Constants for socket types
const int TYPE_TCP = 1;
const int TYPE_UDP = 2;
const int TYPE_UDS_STREAM = 3;
const int TYPE_UDS_DGRAM = 4;

/**
 * Prints an error message to stderr and exits the program with a failure status.
 * @param message The error message to print.
 */
void printErrorAndExit(const std::string &message) {
    std::cerr << "Error: " << message << std::endl;
    exit(EXIT_FAILURE);
}

/**
 * Sets up the server side to handle input from a TCP, UDP, or Unix domain socket client.
 * @param type The type of socket (TCP, UDP, Unix domain stream, or Unix domain datagram).
 * @param path The path for Unix domain sockets.
 * @param input_fd Reference to the input file descriptor.
 */
void handleServerInput(int type, const std::string &path, int &input_fd) {
    struct sockaddr_un server_addr_un;

    if (type == TYPE_UDS_STREAM || type == TYPE_UDS_DGRAM) {
        // Setup Unix domain socket server
        input_fd = socket(AF_UNIX, (type == TYPE_UDS_STREAM) ? SOCK_STREAM : SOCK_DGRAM, 0);
        if (input_fd < 0) {
            printErrorAndExit("Failed to create Unix domain socket");
        }

        memset(&server_addr_un, 0, sizeof(server_addr_un));
        server_addr_un.sun_family = AF_UNIX;
        strncpy(server_addr_un.sun_path, path.c_str(), sizeof(server_addr_un.sun_path) - 1);

        unlink(path.c_str());  // Remove existing socket file

        if (bind(input_fd, (struct sockaddr *)&server_addr_un, sizeof(server_addr_un)) < 0) {
            printErrorAndExit("Failed to bind Unix domain socket");
        }

        if (type == TYPE_UDS_STREAM && listen(input_fd, 1) < 0) {
            printErrorAndExit("Failed to listen on Unix domain stream socket");
        }

        if (type == TYPE_UDS_STREAM) {
            int client_fd = accept(input_fd, nullptr, nullptr);
            if (client_fd < 0) {
                printErrorAndExit("Failed to accept connection on Unix domain stream socket");
            }
            close(input_fd);
            input_fd = client_fd;
        }
    } else {
        printErrorAndExit("Invalid socket type");
    }
}

/**
 * Sets up the client side to send output to a TCP, UDP, or Unix domain socket server.
 * @param type The type of socket (TCP, UDP, Unix domain stream, or Unix domain datagram).
 * @param path The path for Unix domain sockets.
 * @param output_fd Reference to the output file descriptor.
 */
void handleClientOutput(int type, const std::string &path, int &output_fd) {
    struct sockaddr_un server_addr_un;

    if (type == TYPE_UDS_STREAM || type == TYPE_UDS_DGRAM) {
        // Setup Unix domain socket client
        output_fd = socket(AF_UNIX, (type == TYPE_UDS_STREAM) ? SOCK_STREAM : SOCK_DGRAM, 0);
        if (output_fd < 0) {
            printErrorAndExit("Failed to create Unix domain socket");
        }

        memset(&server_addr_un, 0, sizeof(server_addr_un));
        server_addr_un.sun_family = AF_UNIX;
        strncpy(server_addr_un.sun_path, path.c_str(), sizeof(server_addr_un.sun_path) - 1);

        if (connect(output_fd, (struct sockaddr *)&server_addr_un, sizeof(server_addr_un)) < 0) {
            printErrorAndExit("Failed to connect to Unix domain socket server");
        }
    } else {
        printErrorAndExit("Invalid socket type");
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

/**
 * Signal handler for timeout. Prints an error message and exits.
 * @param sig The signal number.
 */
void handleTimeout(int sig) {
    (void)sig; // To avoid unused parameter warning
    printErrorAndExit("Timeout reached, exiting.");
}

int main(int argc, char *argv[]) {
    for (unsigned int i = 0; i < static_cast<unsigned int>(argc); i++) {
        std::cout << argv[i];
    }
    std::cout << std::endl;

    if (argc < 3) {
        printErrorAndExit("Invalid number of arguments");
    }

    std::string executable;
    int input_type = -1, output_type = -1;
    std::string input_path, output_path;
    int timeout = -1;
    int input_fd = -1, output_fd = -1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-e" && i + 1 < argc) {
            executable = argv[++i];
        } else if (arg == "-i" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 5) == "UDSSD") {
                input_type = TYPE_UDS_DGRAM;
                input_path = param.substr(5); // Skip "UDSSD"
            } else if (param.substr(0, 5) == "UDSSS") {
                input_type = TYPE_UDS_STREAM;
                input_path = param.substr(5); // Skip "UDSSS"
            } else {
                printErrorAndExit("Invalid input parameter");
            }
        } else if (arg == "-o" && i + 1 < argc) {
            std::string param = argv[++i];
            if (param.substr(0, 5) == "UDSCD") {
                output_type = TYPE_UDS_DGRAM;
                output_path = param.substr(5); // Skip "UDSCD"
            } else if (param.substr(0, 5) == "UDSCS") {
                output_type = TYPE_UDS_STREAM;
                output_path = param.substr(5); // Skip "UDSCS"
            } else {
                printErrorAndExit("Invalid output parameter");
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

    if (input_type != -1) {
        handleServerInput(input_type, input_path, input_fd);
    }

    if (output_type != -1) {
        handleClientOutput(output_type, output_path, output_fd);
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