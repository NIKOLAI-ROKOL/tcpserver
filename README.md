# tcpserver
The client and server test applications for client's requests limited processig demonstration

1. How to open tcpserver repository from GitHub.
1.1 Create root repo directory via "mkdir repo" console command,
1.2 Go to root direrctory via "cd repo" console command.
1.3 execute "git clone https://github.com/NIKOLAI-ROKOL/tcpserver.git tcpserver" console command to clone repository from GitHub.

2. How to compile TCP server and client applications
2.1 Go to tcpserver reposidory root directory
2.2 Run "make all" console command to compile TCP server and TCP client applications
2.3 Run "make tcpclient" console command to compile TCP client only application
2.4 Run "make tcpserver" console command to compile TCP server only application
2.5 For cleanup please run "make clean" console command.

3. Functional test example
// Raun at console 1 TCP server test application
./tcpserver test_configuration.cfg

// Run at console 2 TCP client test application
./tcpclient 10 80 12

4, Load test example
// Run at console 1 TCP server test application at single CPU core 0.
taskset --cpu-list 0-0 ./tcpserver test_config_load.cfg

// RUN at console 2 TCP client test application at CPU cores from 1 till 3
taskset --cpu-list 1-3 ./tcpclient 10 1 50

// Opend third console and run 'htop" console command to check CPU cores load.

***** Example of test TCP server execution *****

nikolay@volga:~/repo/tcpserver$ ./tcpserver test_configuration.cfg

TCP server test application

Server's configuration:
----------------------------------------------
Server IP addr:                   127.0.0.1
Server IP port:                   2850
Max simultaneously clients:       10
Max client's requests per second: 10
----------------------------------------------

Start client's requests receiving
************ For stop processing please press Ctrl+C **************

Client's connection from 127.0.0.1:51556 is established
Client's connection from 127.0.0.1:51558 is established
Client's connection from 127.0.0.1:51560 is established
Client's connection from 127.0.0.1:51562 is established
Client's connection from 127.0.0.1:51566 is established
Client's connection from 127.0.0.1:51564 is established
Client's connection from 127.0.0.1:51568 is established
Client's connection from 127.0.0.1:51570 is established
Client's connection from 127.0.0.1:51572 is established
Client's connection from 127.0.0.1:51574 is established
Client's connection from 127.0.0.1:51566 is closed
Client's connection from 127.0.0.1:51556 is closed
Client's connection from 127.0.0.1:51562 is closed
Client's connection from 127.0.0.1:51560 is closed
Client's connection from 127.0.0.1:51574 is closed
Client's connection from 127.0.0.1:51568 is closed
Client's connection from 127.0.0.1:51572 is closed
Client's connection from 127.0.0.1:51564 is closed
Client's connection from 127.0.0.1:51558 is closed
Client's connection from 127.0.0.1:51570 is closed
Client's connection from 127.0.0.1:51980 is established
Client's connection from 127.0.0.1:51982 is established
Fail to read from socket error 104 (Connection reset by peer)
Client's connection from 127.0.0.1:51980 is closed
Fail to read from socket error 104 (Connection reset by peer)
Client's connection from 127.0.0.1:51982 is closed
^C
Server stop is initiated

Summary: (accept conn: 12, reset conn: 202, rx requests: 1252, completed: 1032, rejected: 220, error: 0)
Server stop is completed
nikolay@volga:~/repo/tcpserver$

***** Example of test TCP client execution *****

nikolay@volga:~/repo/tcpserver$ ./tcpclient 10 80 12
Client's configuration:
-------------------------------------------
Server IP address:      127.0.0.1
Server IP port:         2850
Test duration:          10 sec
Inter request delay:    80 ms
Clients running:        12
-------------------------------------------
Started client 1
Started client 4
Started client 3
Started client 2
Started client 5
Started client 7
Started client 6
Started client 8
Started client 9
Started client 11
Started client 12
Started client 10
Client task test is done (Task: 1, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 2, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 3, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 4, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 5, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 6, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 7, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 8, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 9, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 10, connects: 102, requests: 102, completed: 0, rejected: 0, error: 0)
Client task test is done (Task: 11, connects: 1, requests: 125, completed: 103, rejected: 22, error: 0)
Client task test is done (Task: 12, connects: 102, requests: 102, completed: 0, rejected: 0, error: 0)
Full test done!!!!

nikolay@volga:~/repo/tcpserver$
