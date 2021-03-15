OBJS = conge.obj conge_input.obj conge_graphics.obj

conge.dll: $(OBJS)
	cl /LD /Fe:$@  $** /link user32.lib

conge.obj: conge.h
conge_input.obj: conge.h
conge_graphics.obj: conge.h

.PHONY: test clean

test: conge.dll
	cl conge_test.c /link conge.lib

clean:
	-rm -f conge.dll conge.lib conge_test.exe $(OBJS)
