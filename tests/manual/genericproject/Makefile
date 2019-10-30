CC      := g++
FLAGS   :=
FILES   := $(shell find . -name '*.cpp' -o -name '*.h')
DESTNAME:= generic

.PHONY: all clean

TARGETS := Generic

all: $(TARGETS)

clean:
	find . -type f -name '*.o' -delete
	rm -rf .obj
	rm -f generic

$(TARGETS):
	$(CC) $(FLAGS) $(FILES) -o $(DESTNAME)
