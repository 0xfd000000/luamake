luabasic:
	@echo $(lua return 123)
	@echo $(lua `123)
	@echo $(lua return make.CC)
	@echo $(lua `make.CC)
	@echo $(lua `2+123)
	@echo $(lua `os.clock())
	@echo $(lua `print(44))

define lua_init

local function say(...)
	print(string.format("[%.6f]", os.clock()), ...)
end
local function show(...)
	local N = select('#', ...)
	for i=1,N do
		local name = select(i, ...)
		say(name, make[name])
	end
end

export("say", say)
export("show", show)

endef

$(lua $(lua_init))

$(lua print(...),1,2,3)

$(say Hello! These are the makefiles: $(MAKEFILE_LIST))
$(show MAKE_VERSION,.FEATURES)


$(info SOMEVAR: $(lua `make.SOMEVAR:gsub("\\","/")))
SOMEVAR = a:\windows\path
$(info SOMEVAR: $(lua `make.SOMEVAR:gsub("\\","/")))

#include fn.mk
#include list.mk
#.ONESHELL:
lu.txt: load.mk
	$(say Generating $@)
	$(show @,%,*,+,|,<,^,?)

define sho1
	@echo '$1 $($1)'

endef
define sho
$(foreach V,$1,$(call sho1,$V))
endef

$(show .LOADED)

mk.txt: lu.txt
	$(say Generating $@)
	$(call sho,@ % * + | < ^ ?)

wat: mk.txt
	$(say Done)
