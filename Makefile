test:
	$(CC) conge_test.c conge_complete.c /link user32.lib

clean:
	-rm -f conge_test.exe
