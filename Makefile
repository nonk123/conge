OBJS = conge.obj conge_input.obj conge_graphics.obj

conge.lib: $(OBJS)
	lib /OUT:$@ $**

conge.obj: conge.h
conge_input.obj: conge.h
conge_graphics.obj: conge.h

.PHONY: test clean

test: conge.lib
	cl conge_test.c /link conge.lib user32.lib

clean:
	-rm -f conge.lib conge_test.exe $(OBJS)
