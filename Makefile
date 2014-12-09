all:
	gcc client.? server.? util.? game.? pong.c -o pong

run: all
	./pong

clean:
	rm pong
