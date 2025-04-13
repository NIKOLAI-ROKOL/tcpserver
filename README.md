# tcpserver
The client and server test applications for client's requests limited processig demonstration

1. How to open tcpserver repository from GitHub.
1.1. Create root repo directory via "mkdir repo" console command,
1.2. Go to root direrctory via "cd repo" console command.
1.3. execute "git clone https://github.com/NIKOLAI-ROKOL/tcpserver.git tcpserver" console command to clone repository from GitHub.
1.4. Go to tcpserver work directory via "cd tcpserver" console command.

2. How to compile TCP server and client applications
2.1. Go to tcpserver reposidory root directory
2.2. Run "make all" console command to compile TCP server and TCP client applications
2.3. Run "make tcpclient" console command to compile TCP client only application
2.4. Run "make tcpserver" console command to compile TCP server only application
2.5. For cleanup please run "make clean" console command.

3. Functional test example
3.1. Raun at console 1 TCP server test application via "./tcpserver test_configuration.cfg" console command.
3.2. Run at console 2 TCP client test application via "./tcpclient 10 80 12" console command.

4, Load test example
4.1. Run at console 1 TCP server test application at single CPU core 0 via "taskset --cpu-list 0-0 ./tcpserver test_config_load.cfg" console command.
4.2. Run at console 2 TCP client test application at CPU cores from 1 till 3 via "taskset --cpu-list 1-3 ./tcpclient 10 1 50" console command.
4.3. Opend third console and run 'htop" console command to check CPU cores load.
