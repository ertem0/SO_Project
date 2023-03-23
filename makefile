CC = gcc
CFLAGS = -Wall -Wextra -Wwrite-strings -g
THREADS = -pthread

SRCS = HomeIOT.c
SRCS_SENSOR = Sensor.c
SRCS_CONSOLE = UserConsole.c

OBJS = $(SRCS:.c=)
OBJS_SENSOR = $(SRCS_SENSOR:.c=)
OBJS_CONSOLE = $(SRCS_CONSOLE:.c=)

all: $(OBJS) $(OBJS_SENSOR) $(OBJS_CONSOLE)

$(OBJS): $(SRCS)
#	$@ (target) is the OBJS and $^ (dependecies) is the SRCS
	$(CC) $(THREADS) $(CFLAGS) -o $@ $^ 

$(OBJS_SENSOR): $(SRCS_SENSOR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJS_CONSOLE): $(SRCS_CONSOLE)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(OBJS_SENSOR) $(OBJS_CONSOLE)

