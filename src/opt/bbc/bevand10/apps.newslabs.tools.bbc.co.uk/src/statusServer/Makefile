.PHONY: compile

bin=statusServer

compile: 
	gcc -o $(bin) -static $(bin).c
	strip $(bin)
	ls -lh $(bin)

run:
	./$(bin) 8080 .
