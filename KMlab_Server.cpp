#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <random>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 1043;
int secretNumber;

// Function to generate a random four-digit number
int generateSecretNumber() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1000, 9999);
    return dist(gen);
}

// Log actions to a file
void logAction(const std::string& action) {
    std::ofstream logFile("server_log.txt", std::ios::app);
    if (!logFile) {
        std::cerr << "Error opening log file!" << std::endl;
        return;
    }
    std::time_t now = std::time(0);
    logFile << std::ctime(&now) << " - " << action << std::endl;
}

// Send a response to the client and check for errors
bool sendResponse(SOCKET client_socket, const std::string& message) {
    int bytesSent = send(client_socket, message.c_str(), message.size() + 1, 0); // +1 for null-terminator
    if (bytesSent <= 0) {
        logAction("Failed to send response. Closing connection.");
        return false;
    }
    return true;
}

// Handle each client connection
void handleClient(SOCKET client_socket) {
    char buffer[8];  // Command buffer size of 5 for "GUESS", "QUIT", "SHOW"
    secretNumber = generateSecretNumber();
    logAction("New game started. Secret number: " + std::to_string(secretNumber));

    while (true) {
        // Receive command from client (fixed length for "GUESS", "QUIT", or "SHOW")
        int bytesReceived = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            logAction("Connection lost or client disconnected.");
            break;
        }

        buffer[bytesReceived] = '\0';  // Null-terminate the received string
        std::string command(buffer);

        if (command == "GUESS") {
            int guess;
            // Receive the guessed number from the client
            if (recv(client_socket, reinterpret_cast<char*>(&guess), sizeof(guess), 0) <= 0) {
                logAction("Failed to receive guess. Closing connection.");
                break;
            }
            guess = ntohl(guess);  // Convert the guess from network byte order to host byte order

            // Check if the guess is correct or incorrect
            std::string response = (guess == secretNumber) ? "Correct" : "Incorrect";
            logAction("Client guessed: " + std::to_string(guess) + " - " + response);

            if (!sendResponse(client_socket, response)) {
                break;  // Close connection on send failure
            }

            // If guessed correctly, end the game
            if (response == "Correct") {
                break;
            }
        }
        else if (command == "QUIT") {
            logAction("Client ended the session.");
            break;
        }
        else if (command == "SHOW") {
            // Send the secret number to the client
            std::string response = "The secret number is: " + std::to_string(secretNumber);
            logAction("Client requested the secret number.");
            if (!sendResponse(client_socket, response)) {
                break;
            }
        }
        else {
            logAction("Unknown command received. Closing connection.");
            sendResponse(client_socket, "ERROR: Unknown command");
            break;
        }
    }
    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    listen(server_socket, 5);
    std::cout << "Server is running and waiting for connections..." << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed!" << std::endl;
            continue;
        }
        logAction("New client connected");
        handleClient(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
