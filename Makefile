CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -g
LDLIBS = -lpthread -lcurl -lcrypto

# -std=c99

server: queue.o server.o util.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

server.o: server.c util.h queue.h
	$(CC) $(CFLAGS) -c $< -o $@

util.o: util.c util.h
	$(CC) $(CFLAGS) -c $< -o $@

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server *.o

clearcache:
	rm -rf ./cache/*
	
run: server
	gnome-terminal -- bash -c "./server 8080 30; exec bash"

debug: server
	gnome-terminal -- bash -c "gdb -ex run --args ./server 8080 30; exec bash"

push:
ifndef COMMIT_MSG
	$(error COMMIT_MSG is not set)
endif
	git add .
	git commit -m "$(COMMIT_MSG)"
	git push
