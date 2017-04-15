#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

//Globální proměnné potčbné pro správný chod funkce intListHandler
int sockfd_global;
pthread_t tid;
std::string name_global;
static volatile bool keepRunning = true;

/**
 * Odešle Log out, ukončí sekundární vlákno a komunikaci na socketu
 * @param int none
 */
void intListHandler(int none/*Nepoužitá poviná proměnná*/) {
    std::string message = std::string(name_global) + " logged out\r\n";
    send(sockfd_global, message.c_str(), message.length(), 0);
    pthread_cancel(tid);
    shutdown(sockfd_global, 2);
    exit(0);
}

/**
 * Odešle Log in, a vše co přečte odešle ve formátu UserName: řádek.
 * Ukončí se pokud je keep(keepRunning) false nebo je li ukončen zvenku.
 * Může použít globánlí proměnn nastavené před rozdělenm vláken.
 * @param keep
 * @return
 */
void* sender(void *keep)
{
    std::string message = std::string(name_global) + " logged in\r\n";
    ssize_t sendCount = send(sockfd_global, message.c_str(), message.length(), 0);
    if (sendCount < 0)
        fprintf(stderr, "ERROR: sendto");

    while ((bool*)keep) {
        getline(std::cin, message);
        if(message.compare("") !=0) {
            message = name_global + ": " + message + "\r\n";
            sendCount = send(sockfd_global, message.c_str(), message.length(), 0);
            if (sendCount <= 0){
                std::cerr <<"ERROR: message was not sent" << std::endl;
            }
        }
    }
    return NULL;
}

/**
 * @param int argc
 * @param char** argv
 * @return
 */
int main(int argc, char** argv) {
    // zpracuje argumenty
    if(argc < 5) {
        std::cerr <<"Invalid Arguments" << std::endl;
        return EXIT_FAILURE;
    }

    bool isI = false, isU = false;
    std::string serverName;

    for(int i=1; i < argc; i++) {
        if( strcmp(argv[i], "-i") == 0) {
            serverName = std::string(argv[i+1]);
            isI = true;
        }

        if( strcmp(argv[i], "-u") == 0) {
            name_global = std::string(argv[i+1]);
            isU = true;
        }
    }
    if(!isI || !isU) {
        std::cerr <<"Invalid Arguments" << std::endl;
        return EXIT_FAILURE;
    }

    // vytvoří socket a nastaví adresu serveru
    ssize_t sendCount;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd_global = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd_global < 0) {
        std::cerr << "ERROR opening socket" << std::endl;
        exit(1);
    }

    server = gethostbyname(serverName.c_str());

    if (server == NULL) {
        std::cerr << "ERROR, no such host\n" << std::endl;
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(server->h_addr, (char *) &serv_addr.sin_addr.s_addr, (size_t) server->h_length);
    serv_addr.sin_port = htons(21011);

    // ustanoví spojení
    if (connect(sockfd_global, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }

    // neblokující spojení
    fcntl(sockfd_global, F_SETFL, O_NONBLOCK);
    // ctr+c handler
    signal(SIGINT, intListHandler);

    // v druhém vláknu pustí sender
    int err = pthread_create(&tid, NULL, &sender, (void*)&keepRunning);
    if (err != 0)
        fprintf(stderr, "can't create thread :[%s]", strerror(err));


    fd_set read_fd_set;
    while (1) {
        char item[2] = {'\0', '\0'};
        ssize_t numberRecv = recv(sockfd_global, item, 1,0);
        if (numberRecv < 0) { // není co přijmout
            FD_ZERO (&read_fd_set);
            FD_SET (sockfd_global, &read_fd_set);
            if (select(sockfd_global + 1, &read_fd_set, NULL, NULL, NULL) < 0) { // čeká na další data
                pthread_cancel(tid);
                exit(EXIT_FAILURE);
            }
        } else {
            std::cout << item;
        }
    }
}

