#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <algorithm>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <vector>
#include <fstream>
#include <future> // async & feature
#include "MapCreator.h"

#define YELLOW  "\033[33m"
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"

const int PORT = 1721;
const int PACKAGE_MAX_LENGTH = 128;
const int MAX_THREADS = 10;
const std::string VERSION = "0.1";
const std::string WHITESPACE = " \n\r\t\f\v";
const std::string EXTENSION_GPSTRACKER = ".gpstracker";
const std::string EXTENSION_MAP = ".html";

std::vector<std::string> IP_LIST;
std::vector<std::string> PACKAGES;
std::string SELECTED_IP = "";
std::string SELECTED_PORT = "";
bool EXIT_SOCKET_LISTNER;

pthread_t threads [MAX_THREADS];
pthread_t timeout_thread;
int threadno = 0, fd;

// Handle ctrl+c to close server socket before terminating the server
void signalHandler(int signal) {
    if (signal == SIGINT) {
        exit (1);
    }
}

void socketExitHandler(int signal) {
    if (signal == SIGINT) {
        close(fd);
        EXIT_SOCKET_LISTNER = true;
    }
}

// Structure to hold the necessary parameters to pass into the threaded
struct request {
    int des;
    char str[PACKAGE_MAX_LENGTH];
    socklen_t addlen;
    sockaddr_in clientaddr;
};

// Function to reverse the string and send back the package
void* scanHandler (void* r) {
    request req = *((request*)r); // Type casting
    std::string ip(inet_ntoa(req.clientaddr.sin_addr));
    std::string port = std::to_string(ntohs(req.clientaddr.sin_port));
    std::string device = ip + ":" + port;
    
    // If the device is new
    if (std::find(IP_LIST.begin(), IP_LIST.end(), device) == IP_LIST.end()) {
        IP_LIST.push_back(device);
        std::cout << YELLOW << "Device was found: " << ip << ":" << port << RESET << std::endl;
        std::cout << "Do you want to stop the scan? (Y/n)";
        std::string input;
        getline(std::cin, input);
        if(input == "Y" || input == "y") {
            EXIT_SOCKET_LISTNER = true;
            close(fd);
        } else {
            std::cout << "Server continuous listen on PORT: " << PORT << std::endl;
        }
    }
    
    return NULL;
}

void* recordPackageHandler(void* r) {
    request req = *((request*) r);
    
    if(strcmp(req.str, "") != 0) {
        PACKAGES.push_back(req.str);
    }
    return NULL;
}

void listen() {
    IP_LIST.clear();
    
    // Define server & client
    sockaddr_in serveraddr;
    sockaddr_in clientaddr;
    
    socklen_t addrlen = sizeof(clientaddr);
    ssize_t receivedlen; // ssize_t = int
    char buffer [PACKAGE_MAX_LENGTH]; // buffer sent in upd package
    
    // Create Socket
    /* socket( // reates an unbound socket in a communications domain, and returns a file descriptor that can be used in later function calls that operate on sockets
     int domain, // Specifies the communications domain in which a socket is to be created.
     int type, // Specifies the type of socket to be created.
     int protocol // Specifies a particular protocol to be used with the socket. Specifying a protocol of 0 causes socket() to use an unspecified default protocol appropriate for the requested socket type.
     )
     */
    if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
        std::cout << RED << "Socket creation failed.." << RESET << std::endl;
    } else {
        std::cout << CYAN << "**Socket created.. " << RESET << std::endl;
        
        // Sets the first num bytes of the block of memory pointed by ptr to the specified value (interpreted as an unsigned char)
        // Clear the memory
        memset ((sockaddr*)&serveraddr, 0, sizeof (serveraddr));
        serveraddr.sin_family = AF_INET;   // IPv4
        serveraddr.sin_addr.s_addr = htonl (INADDR_ANY);  // htonl converts the unsigned integer hostlong from host byte order to network byte order // give the local mashine an address
        serveraddr.sin_port = htons (PORT); // htons converts the unsigned short integer hostshort from host byte order to network byte order.
        
        // when bind using namespace std; doesn't work ..
        // Bind the IP address and the port number to create the socket
        if (bind (fd, (sockaddr*)&serveraddr, sizeof (serveraddr)) == -1) {
            std::cout << RED << "**Binding failed.." << RESET << std::endl;
        } else {
            std::cout << CYAN << "**Binding succesful" << RESET << std::endl;
            
            /* Signal catching */
            signal(SIGINT, socketExitHandler);
            signal(SIGTSTP, socketExitHandler);
            
            std::cout << CYAN << "**Waiting on port: " << ntohs(serveraddr.sin_port) << RESET << std::endl;
            std::cout << YELLOW << "To kill the listner press `Ctrl + c`" << RESET << std::endl;
            
            while (true) {
                // waiting to recieve the requests from client at port
                receivedlen = recvfrom (fd, buffer, PACKAGE_MAX_LENGTH, 0, (sockaddr*) &clientaddr, &addrlen);
                
                // Filling the parameter values
                request *req = new request; // Allocate
                bzero(req, sizeof (request));  // Clear memory
                req->addlen = addrlen;
                req->clientaddr = clientaddr;
                req->des = fd;
                strcpy(req->str, buffer);
                
                // Creating thread to execute the reqeust paralelly
                // int pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg);
                pthread_create(&threads [threadno++], NULL, scanHandler, (void *) req);
                if(threadno == MAX_THREADS) threadno = 0;
                
                // Fill block of memory // clear memory
                memset(buffer, 0, sizeof(buffer));
                
                if(EXIT_SOCKET_LISTNER) {
                    break;
                }
            }
            close(fd);
        }
    }
}

void recordDeviceActivity(int _packages) {
    if(SELECTED_IP == "" || SELECTED_PORT == "") {
        std::cout << RED << "ERR: Something went wrong ..\n\t\tplease try again later." << RESET << std::endl;
    } else {
        std::cout << CYAN << "**Selected Device with IP: " << SELECTED_IP << " and PORT: " << SELECTED_PORT << RESET << std::endl;
        
        sockaddr_in serveraddr;
        sockaddr_in clientaddr;
        
        socklen_t addrlen = sizeof(clientaddr);
        ssize_t receivedlen;
        char buffer [PACKAGE_MAX_LENGTH]; // buffer sent in upd package
        
        if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
            std::cout << RED << "Socket creation failed.." << RESET << std::endl;
        } else {
            // Sets the first num bytes of the block of memory pointed by ptr to the specified value (interpreted as an unsigned char)
            // Clear the memory
            memset ((sockaddr*)&serveraddr, 0, sizeof (serveraddr));
            serveraddr.sin_family = AF_INET;   // IPv4
            serveraddr.sin_addr.s_addr = htonl (INADDR_ANY);  // htonl converts the unsigned integer hostlong from host byte order to network byte order // give the local mashine an address
            serveraddr.sin_port = htons (PORT); // htons converts the unsigned short integer hostshort from host byte order to network byte order.
            
            // when bind using namespace std; doesn't work ..
            // Bind the IP address and the port number to create the socket
            if (bind (fd, (sockaddr*)&serveraddr, sizeof (serveraddr)) == -1) {
                std::cout << RED << "Binding failed.." << RESET << std::endl;
            } else {
                std::cout << CYAN << "**Binding succesful" << RESET << std::endl;
                
                /* Signal catching */
                signal(SIGINT, socketExitHandler);
                signal(SIGTSTP, socketExitHandler);
                
                std::cout << CYAN << "**Waiting for (" << _packages << ") packages" << RESET << std::endl;
                std::cout << YELLOW << "To kill the listner press `Ctrl + c`" << RESET << std::endl;
                
                int counterPackages = 0;
                while (true) {
                    // waiting to recieve the requests from client at port
                    receivedlen = recvfrom (fd, buffer, PACKAGE_MAX_LENGTH, 0, (sockaddr*) &clientaddr, &addrlen);
                    
                    std::string _ip(inet_ntoa(clientaddr.sin_addr));
                    if(_ip == SELECTED_IP && std::to_string(ntohs(clientaddr.sin_port)) == SELECTED_PORT) {
                        std::cout << "Received package from " << SELECTED_IP << ":" << SELECTED_PORT << ".." << std::endl;
                        
                        // Filling the parameter values
                        request *req = new request; // Allocate
                        bzero(req, sizeof (request));  // Clear memory
                        req->addlen = addrlen;
                        req->clientaddr = clientaddr;
                        req->des = fd;
                        strcpy(req->str, buffer);
                        
                        // Creating thread to execute the reqeust paralelly
                        // int pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg);
                        pthread_create(&threads [threadno++], NULL, recordPackageHandler, (void *) req);
                        if(threadno == MAX_THREADS) threadno = 0;
                        
                        // Fill block of memory // clear memory
                        memset(buffer, 0, sizeof(buffer));
                    }
                    
                    if(counterPackages + 1 >= _packages) {
                        EXIT_SOCKET_LISTNER = true;
                        std::cout << CYAN << "All " << _packages << " package(s) received!" << RESET << std::endl;
                    }
                    if(EXIT_SOCKET_LISTNER) {
                        break;
                    }
                    counterPackages++;
                }
                close(fd);
            }
        }
    }
}

void commandList() {
    if(IP_LIST.size() == 0) {
        std::cout << YELLOW << "No device available.." << RESET << std::endl;
    } else {
        std::cout << GREEN << "#" << IP_LIST.size() << " Device(s) found" << RESET << std::endl;
        
        std::cout << "--------------------- Scanned Devices --------------------" << std::endl;
        for(int i = 0; i < IP_LIST.size(); i++) {
            std::string _ip   = IP_LIST.at(i).substr(0, IP_LIST.at(i).find(":"));
            std::string _port = IP_LIST.at(i).substr((IP_LIST.at(i).find(":") + 1));
            std::cout << YELLOW << "Device #" << (i + 1) << "\tIP:" << _ip << "\tPORT:" << _port << RESET << std::endl;
        }
        std::cout << "------------------- END:Scanned Devices ------------------" << std::endl;
    }
}

void commandHelp() {
    std::cout << YELLOW << "GPS Tracker | Version " << VERSION << std::endl;
    std::cout << "These shell commands are defined internaly." << std::endl;
    std::cout << "\nCONSOLE COMMANDS" << std::endl;
    std::cout << "Type `help` to see this list." << std::endl;
    std::cout << "Type `--v` or `--version` to see the version of the program." << std::endl;
    std::cout << "\nSCAN COMMANDS" << std::endl;
    std::cout << "Type `scan -devices` to inspect for devices." << std::endl;
    std::cout << "Type `scan -device -[number] -[packages]` listen only the selected device." << std::endl;
    std::cout << "\nSHOW COMMANDS" << std::endl;
    std::cout << "Type `show -list -devices` to list all found devices." << std::endl;
    std::cout << "Type `show -list -packages` to see the temporare saved list with packages" << std::endl;
    std::cout << "\nSAVE & CREATE COMMANDS" << std::endl;
    std::cout << "Type `save -list -n [name without extention]` save the temporare list" << std::endl;
    std::cout << "Type `create -map -list [name without extention]` create web map with the temporare list data" << std::endl;
    std::cout << "\nEXIT COMMANDS" << std::endl;
    std::cout << "Press Ctrl + `c` to kill a listner" << std::endl;
    std::cout << "Type `exit` or `quit` to exit the program" << RESET << std::endl << std::endl;
}

void commandVersion() {
    std::cout << YELLOW << "GPS Tracker | Version "<< VERSION << RESET <<std::endl;
}

void commandScan() {
    listen();
    signal(SIGINT, signalHandler);
    signal(SIGTSTP, signalHandler);
    EXIT_SOCKET_LISTNER = false;
    std::cout << std::endl << YELLOW << "We found #" << IP_LIST.size() << " Device(s)" << RESET << std::endl;
}

void* commandRecord(int _deviceNumber, int _packages) {
    if(IP_LIST.size() == 0) {
        std::cout << YELLOW << "Empty List" << RESET << std::endl;
        return NULL;
    }
    if(_deviceNumber > IP_LIST.size() || _deviceNumber <=  0) {
        std::cout << YELLOW << "Invalid device number" << RESET << std::endl;
        return NULL;
    }
    if(_packages <= 0) {
        std::cout << YELLOW << "Invalid package number" << RESET << std::endl;
        return NULL;
    }
    
    try {
        std::string selectedDevice = IP_LIST.at(_deviceNumber - 1);
        SELECTED_IP = selectedDevice.substr(0, selectedDevice.find(":"));
        SELECTED_PORT = selectedDevice.substr(selectedDevice.find(":") + 1);
        std::cout << "Recording .." << std::endl;
        recordDeviceActivity(_packages);
    } catch(const std::exception&) {
        std::cout << "Device number undefined" << std::endl;
    }
    
    return NULL;
}

void commandShowList() {
    if(PACKAGES.size() == 0) {
        std::cout << YELLOW << "No saved coordinates" << RESET << std::endl;
    } else {
        std::cout << "--------------------- Saved Packages --------------------" << std::endl;
        for(int i = 0; i < PACKAGES.size(); i++) {
            std::cout << BLUE << "[Package]: " << PACKAGES.at(i) << RESET << std::endl;
        }
        std::cout << "------------------- END:Saved Packages -------------------" << std::endl;
    }
}

std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

void* commandSaveList(std::string _filename) {
    std::string filename = trim(_filename);
    
    if(filename.length() == 0) {
        std::cout << RED << "No file name given" << RESET << std::endl;
        return NULL;
    }
    
    if(PACKAGES.size() == 0) {
        std::cout << YELLOW << "No saved coordinates" << RESET << std::endl;
    } else {
        std::cout << YELLOW << "Package saving! Please wait.." << RESET << std::endl;
        
        std::ofstream myfile;
        myfile.open (filename + EXTENSION_GPSTRACKER);
        if(myfile.is_open()) {
            for(int i = 0; i < PACKAGES.size(); i++) {
                myfile << PACKAGES.at(i) << "\n";
            }
            myfile.close();
            std::cout << GREEN << "Data was successfully saved!" << RESET << std::endl;
        } else {
            std::cout << RED << "File opening failed! Please try again later.." << RESET << std::endl;
        }
    }
    return NULL;
};

void* commandCreateMapFromList(std::string _filename) {
    std::string filename = trim(_filename);
    
    if(filename.length() == 0) {
        std::cout << RED << "No file name given" << RESET << std::endl;
        return NULL;
    }
    
    if(PACKAGES.size() == 0) {
        std::cout << YELLOW << "No saved coordinates" << RESET << std::endl;
    } else {
        std::ofstream myfile;
        myfile.open (filename + EXTENSION_MAP);
        if(myfile.is_open()) {
            for(int i = 0; i < PACKAGES.size(); i++) {
                std::cout << i << ": " << PACKAGES.at(i) << std::endl;
            }
            std::string html = getMap(PACKAGES);
            
            myfile << html;
            myfile.close();
            std::cout << GREEN << "Map was successfully created!" << RESET << std::endl;
        } else {
            std::cout << RED << "File opening failed! Please try again later.." << RESET << std::endl;
        }
    }
    return NULL;
};

int main(int argc, const char * argv[]) {
    std::cout << "Welcome to GPSTracker!" << std::endl;
    std::cout << "Type `help` to see all commands" << std::endl;
    std::string input;
    
    do {
        std::cout << "~bash: ";
        getline(std::cin, input);
        
        if(input == "help") {
            commandHelp();
        } else if(input == "--v" || input == "--version") {
            commandVersion();
        } else if(input == "scan -devices") {
            commandScan();
        } else if(input.find("scan -device -") != std::string::npos) {
            std::string options = input.substr(input.find("-device -") + 8);
            if(std::count(options.begin(), options.end(), '-') != 2) {
                std::cout << YELLOW "You need 2 Options -[device] -[packages]" << RESET << std::endl;
            } else {
                try {
                    int deviceID = stoi(options.substr(1, options.find(' ')));
                    int packages = stoi(options.substr(options.find(' ') + 2));
                    commandRecord(deviceID, packages);
                } catch (const std::exception&) {
                    std::cout << RED << "Invalid options" << RESET << std::endl;
                }
            }
            /*try {
             commandRecord(stoi(input.substr(input.find("-device -") + 9)));
             } catch (const std::exception&) {
             std::cout << RED << "Invalid device number" << RESET << std::endl;
             }*/
        } else if(input == "show -list -devices") {
            commandList();
        } else if(input == "show -list -packages") {
            commandShowList();
        } else if(input.find("save -list -n ") != std::string::npos) {
            try {
                commandSaveList(input.substr(input.find("-n") + 3));
            } catch (const std::exception&) {
                std::cout << RED << "Invalid name" << RESET << std::endl;
            }
        } else if(input.find("create -map -list ") != std::string::npos) {
            try {
                commandCreateMapFromList(input.substr(input.find("-list ") + 6));
            } catch (const std::exception&) {
                std::cout << RED << "Invalid name" << RESET << std::endl;
            }
        } else if(input == "exit" || input == "quit") {
            exit(1);
        } else {
            std::cout << RED << "-bash: " << input << ": command not found" << RESET << std::endl;
        }
    } while(true);
    return 0;
}
