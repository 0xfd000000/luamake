define lua_init

local genrule = require "genrule"
Rule = genrule.rule

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

# demonstrate unique names & tostring
$(info $(lua `tostring(Rule {"@echo ola"}) ))
$(info $(lua `tostring(Rule {"@echo ola"}) ))


# simple rule
define simple_rule

local r = Rule {
    "@touch $$@"
}
eval(tostring(r))
return r.name

endef

simple = $(lua $(simple_rule))
$(info name of simple rule: $(simple))
test.genrule: $(simple)

# multi-line recipes and target-specific variables
define genrule


local r = Rule {
	CC="clang",
	CXX="clang++",
	{
		"$$(say hello, this is a synthesized rule)",
		"$$(show CC,CXX)",
		"$$(say my target is: $$@)",
	}
}

eval(tostring(r))
say("Generated rule", r.name)
return r.name
endef

# generate a few rules
# note the use of simple assignment - the whole kaboodle, including
# side-effects, would be re-evaluated of recursive assign (=) was used
gens := $(lua $(genrule)) $(lua $(genrule)) $(lua $(genrule)) $(lua $(genrule))
$(info generated: $(gens))
generated: $(gens)
	$(say $@: $^)

