# evaluate Lua code from GNU make


In an experiment that probably never should've seen the light of day, I created a small binding that exposes lua to GNU make via make's `load` feature.

It gives you:

## the lua side

    function eval(expr)

which does the obvious thing

    function expand(a,b,c)

which expands a, b and c as make expressions and pushes the results thereof

    function export(name, function)

to make lua functions available as make functions

    the global table make

whose __index metamethods looks up make variables, so `make.MAKECMDGOALS` does the obvious thing


## the make side

    $(lua expr,...)

function to evaluate lua expressions


## examples

The following

```lua

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
```

will make these makefile snippet

```make
$(lua $(lua_init))

$(say Hello! These are the makefiles: $(MAKEFILE_LIST))
$(show MAKE_VERSION,.FEATURES)
```

result in the output

```
[0.002632]      Hello! These are the makefiles:  Makefile load.mk
[0.002669]      MAKE_VERSION    4.2.1
[0.002697]      .FEATURES       target-specific order-only second-expansion else-if shortest-stem undefine oneshell archives jobserver output-sync check-symlink guile load
```



I have not yet done anything useful with this.

