OBJS = conge_test.obj conge_complete.obj
EXES = conge_test_c.exe conge_test_cpp.exe

test:
	$(CC) /Fe:conge_test_c.exe conge_test.c conge_complete.c /link user32.lib
	$(CPP) /Fe:conge_test_cpp.exe conge_test.cpp conge_complete.c /link user32.lib

clean:
	-rm -f $(EXES) $(OBJS)
