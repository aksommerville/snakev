all:
.SILENT:
PRECMD=echo "  $(@F)" ; mkdir -p $(@D) ;

CCWARN:=-Werror -Wimplicit
CC:=gcc -c -MMD -O2 -Isrc $(CCWARN)
LD:=gcc
LDPOST:=

CFILES:=$(shell find src -name '*.c')
OFILES:=$(patsubst src/%.c,mid/%.o,$(CFILES))
-include $(OFILES:.o=.d)

mid/%.o:src/%.c;$(PRECMD) $(CC) -o $@ $<

EXE:=out/snake-libc
all:$(EXE)
$(EXE):$(OFILES);$(PRECMD) $(LD) -o $@ $^ $(LDPOST)

run:$(EXE);$(EXE)

clean:;rm -rf mid out
