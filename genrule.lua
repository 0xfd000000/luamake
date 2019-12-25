--[[
DSL for creating make rules

```
r = Rule {
    "@touch $$@"
}
```

will create a (uniquely named) rule. `tostring(r)` will yield
an `$(eval)`uatable make expression that defines the rule

```
_anon_lua_rule_1:
    @touch $@
```

key-value pairs are turned into target-specific variables:
```
r = Rule {
    FOO="bar",
    {
        "@echo hello from $$@'s rule",
        "@echo the value of FOO is $$(FOO)",
        "@touch $$@",
    }
}
```

tostring(r):
```
_anon_lua_rule_1:FOO=bar
_anon_lua_rule_1:
    @echo hello from $@'s rule
    @echo the value of FOO is $(FOO)
    @touch $@

```
]]

local M = {}

local i = 1
function M.beep()
    print(string.format("beep %d", i))
    i = i + 1
end

local MTi_rule = {}
local MT_rule = {__index = MTi_rule}

function MT_rule:__tostring()
    return table.concat({
        self:head(),
        self:recipe(),
    }, "\n")
end

-- the "head" is a bunch of rule:VAR=value lines
function MTi_rule:head()
    local head = {}
    for k,v in pairs(self.locals) do
        table.insert(head, string.format("%s:%s=%s", self.name, k, tostring(v)))
    end
    return table.concat(head, "\n")
end
-- generates the actual recipe
function MTi_rule:recipe()
    local ret = {
        self.name..":"
    }
    for _, cmd in ipairs(self.commands) do
        table.insert(ret, cmd)
    end
    return table.concat(ret, "\n\t")
end

local anonRuleCount = 0
local function mkname()
    anonRuleCount = anonRuleCount + 1
    return "_anon_lua_rule_" .. tostring(anonRuleCount)
end

local function mkrule(name, cfg)
    local locals = {}
    for k,v in pairs(cfg) do
        if type(k) == "string" then
            locals[k] = v
        end
    end
    local commands = type(cfg[1]) == "table" and cfg[1] or {cfg[1]}
    local ret = {
        name = name,
        locals = locals,
        commands = commands,
    }
    return setmetatable(ret, MT_rule)
end

function M.rule(name)
    if type(name) == "table" then
        return mkrule(mkname(), name)
    end
    return function(cfg)
        return mkrule(name, cfg)
    end
end

local function iter(name)
    local ex = expand(string.format("$(patsubst %%,{%%},$(%s))", name))
    return ex:gmatch "{([^}]*)}"
end

function M.list(name)
    local ret = {}
    for el in iter(name) do
        table.insert(ret, el)
    end
    return ret
end

M.iter = iter

return M