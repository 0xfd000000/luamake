default: all

all: lua_make.so usage
all: ;

lua_make.so: lua_make.c
	$(CC) -shared -fPIC -o $@ $< -llua

.PHONY: usage
usage:
	@echo specify test.genrule or test.simple for some demonstrations

-load lua_make.so

ifneq (,$(filter lua_make.so,$(.LOADED)))

tests = $(filter test.%,$(MAKECMDGOALS))
include $(tests:test.%=%.mk)

.PHONY: test.%

endif

