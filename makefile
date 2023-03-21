CC = gcc
CFLAGS = -Wall -Wextra -g
THREADS = -pthread

SRCS = HomeIOT.C
SRCS_SENSOR = Sensor.C
SRCS_CONSOLE = UserConsole.C

OBJS = $(SRCS:.C=)
OBJS_SENSOR = $(SRCS_SENSOR:.C=)
OBJS_CONSOLE = $(SRCS_CONSOLE:.C=)

all: $(OBJS) $(OBJS_SENSOR) $(OBJS_CONSOLE)

$(OBJS): $(SRCS)
	$(CC) $(THREADS) $(CFLAGS) -o $@ $^

$(OBJS_SENSOR): $(SRCS_SENSOR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJS_CONSOLE): $(SRCS_CONSOLE)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(OBJS_SENSOR) $(OBJS_CONSOLE)

