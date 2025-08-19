main: main.c
	gcc -Wall -Wextra -std=c11 -pedantic -o main client.c main.c `pkg-config --cflags --libs dbus-1`
