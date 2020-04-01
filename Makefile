target  ?= ipsort
objects := $(patsubst %.c,%.o,$(wildcard *.c))

libs:=sqlite3

#EXTRAS += -fsanitize=undefined -fsanitize=null -fcf-protection=full -fstack-protector-all -fstack-check -Wimplicit-fallthrough

ifdef libs
LDLIBS  += $(shell pkg-config --libs   ${libs})
CFLAGS  += $(shell pkg-config --cflags ${libs})
endif

LDFLAGS += -Os -flto ${EXTRAS}
CFLAGS  += -std=gnu11 -Os -flto -ggdb ${EXTRAS}

.PHONY: all
all:	$(target)

.PHONY: clean
clean:
	rm -f $(target) $(objects)

$(target): $(objects)
