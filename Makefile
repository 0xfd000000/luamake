default: all

all: lua_make.so
all: ;

lua_make.so: lua_make.c
	$(CC) -shared -fPIC -o $@ $< -llua


-load lua_make.so

ifneq (,$(filter lua_make.so,$(.LOADED)))

tests = $(filter test.%,$(MAKECMDGOALS))
include $(tests:test.%=%.mk)

.PHONY: test.%

endif

