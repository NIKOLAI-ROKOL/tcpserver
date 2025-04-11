#######################################################################################
#Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia
#Revision history:
#10.04.2025 - Created initial version
#######################################################################################
WARN	= #-Wall -Wstrict-prototypes -Wmissing-prototypes
CC	= gcc $(WARN)
DEBUG   = -O2 -fno-strength-reduce
CFLAGS	= 
LDFLAGS	=
LD	= $(CC)

RM	= rm -f
DEBUG	= -g

SERVER_TRG	= tcpserver

SERVER_LIBS	= -lrt -lnsl

SERVER_INCS	=	-I ./server \
				-I ./common

SERVER_SRCS = \
./common/common_lib.c \
./server/tcp_server.c \

SERVER_OBJS	=  $(SERVER_SRCS:.c=.o)

CLIENT_TRG	= tcpclient

CLIENT_LIBS	= -lrt -lpthread -lnsl

CLIENT_INCS	=	-I ./client \
				-I ./common

CLIENT_SRCS = \
./common/common_lib.c \
./client/tcp_client.c \

CLIENT_OBJS	=  $(CLIENT_SRCS:.c=.o)

.c.o: 
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) -c $(SERVER_INCS) $< -o $@
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) -c $(CLIENT_INCS) $< -o $@

server: $(SERVER_TRG)
client: $(CLIENT_TRG)
all: $(SERVER_TRG) $(CLIENT_TRG)

$(SERVER_TRG) : $(SERVER_OBJS)
	$(LD) -o $@ $(SERVER_OBJS) $(DEBUG) $(LDFLAGS) $(SERVER_LIBS)

$(CLIENT_TRG) : $(CLIENT_OBJS)
	$(LD) -o $@ $(CLIENT_OBJS) $(DEBUG) $(LDFLAGS) $(CLIENT_LIBS)

clean:
	$(RM) $(SERVER_OBJS) $(SERVER_TRG) $(CLIENT_OBJS) $(CLIENT_TRG)
