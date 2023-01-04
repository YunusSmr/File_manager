all:
	gcc -o fm file_manager.c -lpthread
	gcc -o fc file_client.c 
clean:
	rm fm
	rm fc
	