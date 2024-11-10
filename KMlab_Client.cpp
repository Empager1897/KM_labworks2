#include <iostream>
#include <fstream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 1043;
const std::string SERVER_IP = "127.0.0.1";

// Log actions to a file
void logAction(const std::string& action) {
    std::ofstream logFile("client_log.txt", std::ios::app);
    if (!logFile) {
        std::cerr << "Error opening log file!" << std::endl;
        return;
    }
    std::time_t now = std::time(0);
    logFile << std::ctime(&now) << " - " << action << std::endl;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &server_addr.sin_addr);

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed!" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    while (true) {
        std::string command;
        std::cout << "Enter a four-digit guess, SHOW to reveal the secret number, or QUIT to exit: ";
        std::cin >> command;

        if (command == "QUIT") {
            send(client_socket, "QUIT", 4, 0);
            logAction("Ended session.");
            break;
        }
        else if (command == "SHOW") {
            // Send "SHOW" command to get the secret number
            if (send(client_socket, "SHOW", 4, 0) <= 0) {
                std::cerr << "Failed to send command. Ending session." << std::endl;
                break;
            }

            // Receive response from the server (secret number)
            char response[100];
            int bytesReceived = recv(client_socket, response, sizeof(response) - 1, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Failed to receive response. Ending session." << std::endl;
                break;
            }

            response[bytesReceived] = '\0';  // Null-terminate the received string
            std::cout << "Server response: " << response << std::endl;
        }
        else if (command.size() == 4 && std::all_of(command.begin(), command.end(), ::isdigit)) {
            int guess = std::stoi(command);

            // Send "GUESS" command to the server
            if (send(client_socket, "GUESS", 5, 0) <= 0) {  // Send "GUESS" as a 5-byte command
                std::cerr << "Failed to send command. Ending session." << std::endl;
                break;
            }

            // Send the guessed number in network byte order
            int network_guess = htonl(guess);
            if (send(client_socket, reinterpret_cast<char*>(&network_guess), sizeof(network_guess), 0) <= 0) {
                std::cerr << "Failed to send guess. Ending session." << std::endl;
                break;
            }

            // Receive response from the server
            char response[100];
            int bytesReceived = recv(client_socket, response, sizeof(response) - 1, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Failed to receive response. Ending session." << std::endl;
                break;
            }

            response[bytesReceived] = '\0';  // Null-terminate the received string
            std::cout << "Server response: " << response << std::endl;

            if (std::string(response) == "Correct") {
                std::cout << "Congratulations! You guessed the number!" << std::endl;
                break;  // End the game if the guess was correct
            }
        }
        else {
            std::cerr << "Invalid input. Please enter a four-digit number, SHOW, or QUIT." << std::endl;
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
