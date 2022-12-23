//
// Created by oleksiy on 19.12.22.
//
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <csignal>
#include <set>
#include <vector>
#include <random>
#include "check.hpp"


namespace client_server_functions
{

    using namespace std;

    const unsigned short SERVER_PORT = 61337;

    inline ostream& operator << (ostream& stream, const sockaddr_in& addr)
    {
        union
        {
            in_addr_t x;
            char c[sizeof(in_addr)];
        } t{};

        t.x = addr.sin_addr.s_addr;
        return stream   << to_string(int(t.c[0]))
                 << "." << to_string(int(t.c[1]))
                 << "." << to_string(int(t.c[2]))
                 << "." << to_string(int(t.c[3]))
                 << ":" << to_string(ntohs(addr.sin_port));
    }

    inline int make_socket(int type)
    {
        switch(type)
        {
            case SOCK_STREAM:
                return socket(AF_INET, SOCK_STREAM, 0);
            case SOCK_DGRAM:
                return socket(AF_INET, SOCK_DGRAM, 0);
            case SOCK_SEQPACKET:
                return check(socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP));
            default:
                errno = EINVAL;
                return -1;
        }
    }

    inline sockaddr_in local_address(unsigned short port)
    {
        sockaddr_in address{};
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(port);
        address.sin_family = AF_INET;
        return address;
    }

    void log(string str, pthread_mutex_t* mutex)
    {
        pthread_mutex_lock(mutex);
        cout << str << endl;
        pthread_mutex_unlock(mutex);
    }

    int server_process()
    {
        // Preset START
        /*
         * define mutex controlling access to log
         * define listening server's address
         * and listening socket
        */
        pthread_mutex_t log_mutex;
        pthread_mutex_init(&log_mutex, 0);

        sockaddr_in server_address = local_address(SERVER_PORT);
        auto listening_socket = check(make_socket(SOCK_STREAM));
        int connected_socket = 0;

        check(bind(listening_socket, (sockaddr*)&server_address, sizeof(server_address)));
        check(listen(listening_socket, 4));

        // Prevent creation of a zombie process
        struct sigaction childsig_action;
        childsig_action.sa_flags = SA_NOCLDWAIT;
        childsig_action.sa_handler = SIG_DFL;
        sigaction(SIGCHLD, &childsig_action, nullptr);

        int maxlength = 16;
        // Preset END

        // Main listening process
        while(true)
        {
            //check for new connection in while loop
            sockaddr_in connected_address{};
            socklen_t addrlen = sizeof(connected_socket);

            // accept() call blocks current process until got connection
            connected_socket = check(accept(listening_socket, (sockaddr*)&connected_address, &addrlen));

            if (fork() == 0)
            {
                // when got the connection, create new process to handle it
                pthread_mutex_lock(&log_mutex);
                cout << "Connected from: " << connected_address << endl;
                pthread_mutex_unlock(&log_mutex);
                char request[5];
                string response;
                int request_size;
                bool game_run = false;
                bool leads;

                // the game itself:
                {   //START - HANDSHAKE
                    response = "Who's first?\n[me] [you]";
                    send(connected_socket, response.c_str(), response.size(), 0);
                    while(true)
                    {
                        request_size = recv(connected_socket, request, maxlength, 0);
                        cout << string_view(request, sizeof(request)) << endl;
                        if (errno == ECONNRESET or (request_size < 1 && errno == ENOTCONN))
                            // client has reset the connection
                            // exit
                            break;
                        if (request_size == 0)
                        {
                            response = "Request's length is too small. Try to send correct data";
                            send(connected_socket, response.c_str(), response.size(), 0);
                        }
                        else
                        {
                            if (string_view(request, request_size) == "you")
                            {
                                pthread_mutex_lock(&log_mutex);
                                cout << connected_address << ": started game(server leads)" << endl;
                                pthread_mutex_unlock(&log_mutex);
                                game_run = true;
                                leads = true;
                                break;
                            }
                            if (string_view(request, request_size) == "me")
                            {
                                pthread_mutex_lock(&log_mutex);
                                cout << connected_address << ": started game(player leads)" << endl;
                                pthread_mutex_unlock(&log_mutex);
                                game_run = true;
                                leads = false;
                                break;
                            }
                            else
                            {
                                response = "I don't get that answer. Send me \"you\" or \"me\"";
                                send(connected_socket, response.c_str(), response.size(), 0);
                            }
                        }
                    }
                }
                //MAIN GAME PROCESS
                while(game_run)
                {
                    if (leads)
                    {
                        srand(time(nullptr));

                        int x = rand() % 10 + 1;

                        pthread_mutex_lock(&log_mutex);
                        cout << connected_address << ": server guessed " << x << endl;
                        pthread_mutex_unlock(&log_mutex);

                        int guess;
                        response = "I guessed a number from 1 to 10. Try to guess it!";
                        send(connected_socket, response.c_str(), response.size(), 0);

                        while(true)
                        {
                            request_size = recv(connected_socket, request, maxlength, 0);

                            if (errno == ECONNRESET or (request_size < 1 && errno == ENOTCONN))
                                // client has reset the connection
                                // exit
                                break;
                            if (request_size == 0)
                            {
                                response = "Request's length is too small. Try to send correct data";
                                send(connected_socket, response.c_str(), response.size(), 0);
                            }
                            else
                            {
                                guess = atoi(request);

                                pthread_mutex_lock(&log_mutex);
                                cout << connected_address << ": client attempted " << guess << endl;
                                pthread_mutex_unlock(&log_mutex);

                                if (guess != x)
                                {
                                    response = "Nope! Try again";
                                    send(connected_socket, response.c_str(), response.size(), 0);
                                }
                                else
                                {
                                    response = "Yep that's it!";
                                    send(connected_socket, response.c_str(), response.size(), 0);
                                    leads = false;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        bool game_active = false;
                        set<int> attempts;
                        response = "Send me \"start\" when you're ready!";
                        send(connected_socket, response.c_str(), response.size(), 0);

                        pthread_mutex_lock(&log_mutex);
                        cout << connected_address << ": roles changed " << endl;
                        pthread_mutex_unlock(&log_mutex);

                        // wait for start
                        while(true)
                        {
                            request_size = recv(connected_socket, request, maxlength, 0);

                            if (errno == ECONNRESET or (request_size < 1 && errno == ENOTCONN))
                                // client has reset the connection
                                // exit
                                break;
                            if (request_size == 0)
                            {
                                response = "Request's length is too small. Try to send correct data";
                                send(connected_socket, response.c_str(), response.size(), 0);
                            }
                            else
                            {
                                if (string_view(request, request_size) == "start")
                                {
                                    game_active = true;

                                    pthread_mutex_lock(&log_mutex);
                                    cout << connected_address << ": started game(client leads)" << endl;
                                    pthread_mutex_unlock(&log_mutex);

                                    break;
                                }
                                else
                                {
                                    response = "Type \"start\" for start";
                                    send(connected_socket, response.c_str(), response.size(), 0);
                                }
                            }
                        }
                        // game
                        while(game_active)
                        {
                            if (attempts.size() == 10)
                            {
                                response = "That's not fair! >:(";
                                send(connected_socket, response.c_str(), response.size(), 0);
                                game_run = false;

                                pthread_mutex_lock(&log_mutex);
                                cout << connected_address << ": kicked from server for cheating " << endl;
                                pthread_mutex_unlock(&log_mutex);

                                break;
                            }

                            int attempt = 1 + (rand() % 10);
                            while(attempts.count(attempt))
                                attempt = 1 + (rand() % 10);
                            attempts.insert(attempt);

                            response = "Maybe it's " + to_string(attempt);
                            send(connected_socket, response.c_str(), response.size(), 0);

                            pthread_mutex_lock(&log_mutex);
                            cout << connected_address << ": server attempted " << attempt << endl;
                            pthread_mutex_unlock(&log_mutex);

                            while(true)
                            {
                                request_size = recv(connected_socket, request, maxlength, 0);

                                if (errno == ECONNRESET or (request_size < 1 && errno == ENOTCONN))
                                    // client has reset the connection
                                    // exit
                                    break;
                                if (request_size == 0)
                                {
                                    response = "Request's length is too small. Try to send correct data";
                                    send(connected_socket, response.c_str(), response.size(), 0);
                                }
                                else
                                {
                                    if (string_view(request, request_size)  == "yes")
                                    {
                                        game_active = false;
                                        leads = true;

                                        pthread_mutex_lock(&log_mutex);
                                        cout << connected_address << ": roles changed " << endl;
                                        pthread_mutex_unlock(&log_mutex);

                                        break;
                                    }
                                    if (string_view(request, request_size)  == "no")
                                        break;
                                    else
                                    {
                                        response = "Type \"yes\" or \"no\"";
                                        send(connected_socket, response.c_str(), response.size(), 0);
                                    }
                                }
                            }
                        }
                    }
                }
                pthread_mutex_lock(&log_mutex);
                cout << "Disconnected from " << connected_address << endl;
                pthread_mutex_unlock(&log_mutex);

                exit(0);
            }
            // keep listening
        }
    }

    int client_process()
    {
        char response_buffer[256];
        auto dest_address = local_address(SERVER_PORT);
        int sock_fd = check(make_socket(SOCK_STREAM));
        check(connect(sock_fd, (sockaddr*)&dest_address, sizeof(dest_address)));

        while(true)
        {
            string message = "";
            int size = check(recv(sock_fd, response_buffer, sizeof(response_buffer), 0));

            if (errno == ECONNRESET or (size < 1 && errno == ENOTCONN))
                // server has reset the connection
                // exit
                break;

            cout << string_view(response_buffer, size) << std::endl;
            while(message == "")
                getline(cin, message);
            if (message == "q")
                break;
            send(sock_fd, message.c_str(), message.size(), 0);
        }

        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        return 0;
    }
}

#ifndef LAB4_SERVER_CLIENT_ROUTINE_H
#define LAB4_SERVER_CLIENT_ROUTINE_H

#endif //LAB4_SERVER_CLIENT_ROUTINE_H
