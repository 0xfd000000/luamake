/**
 * Minimal Lua embedding into GNU Makefiles via GNU make's
 * loadable objects feature (introduced in make 4.0)
 * 
 * Copyright (C) 2019 swym (0xfd000000@gmail.com)
 * 
 * 
 * This software is released under the terms of
 *     version 3 of the GNU General Public License.
 * 
 * see the file LICENSE for the full licene
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gnumake.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef _WIN32
#define LUA_MAKE_EXPORT __declspec(dllexport)
#else
#define LUA_MAKE_EXPORT
#endif //_WIN32

LUA_MAKE_EXPORT int plugin_is_GPL_compatible;

static lua_State* L = NULL;

/** hacky way to provoke make to error out: synthesize a call
 * to make's $(error) function
 */
static void
error(const char* msg)
{
  char buf[256];
  snprintf(buf, 256, "$(error %s)\n", msg);
  fprintf(stderr, "error '%s'\n\n", msg);
  gmk_eval(buf, NULL);
}

/** lookup a make variable by name
 * 
 * basically turns a string passed into the make expression
 *   $(string)
 * and returns its value after expansion
 * 
 * Used as __index metamethod for the `make` global
 */
static int
lua_lookup(lua_State* L)
{
  luaL_checkstring(L, 2);
  //ignore arguments 3..N
  lua_settop(L, 2);

  // arg -> $(arg)
  lua_pushliteral(L, "$(");
  lua_insert(L, 2);
  lua_pushliteral(L, ")");
  lua_concat(L, 3);

  //expand $(arg)
  char* res = gmk_expand(lua_tostring(L, -1));
  lua_settop(L, 0);
  lua_pushstring(L, res);
  gmk_free(res);
  return 1;
}

/** lua function that evaluates a given expression
 * as a make expression.
 */
static int
lua_eval(lua_State* L)
{
  const char* str = lua_tostring(L, -1);
  if (str) {
    lua_Debug info;
    gmk_floc loc = {
      .filenm = "lua",
      .lineno = 42
    };
    gmk_floc* locptr = NULL;
    if (lua_getstack(L, 1, &info)) {
        lua_getinfo(L, "nSl", &info);
        loc.filenm = info.short_src;
        loc.lineno = info.currentline;
        locptr = &loc;
    }
    gmk_eval(str, locptr);
  }
  lua_settop(L, 0);
  return 0;
}

/** expands each argument as a make expression and returns
 * the result for each of them.
 */
static int
lua_expand(lua_State* L)
{
  int N = lua_gettop(L);
  for (int i = 1; i <= N; ++i) {
    char* ex = gmk_expand(lua_tostring(L, i));
    lua_pushstring(L, ex);
    lua_replace(L, i);
    gmk_free(ex);
  }
  return N;
}

/** wrapper around lua_pcall that, if > 0 values are returned by
 * the function being called, converts the first value to a string
 * and assigns it to @param *res . This string is allocated via
 * gmk_alloc so it can be passed to GNU make safely.
 */
static int
pcall(lua_State* L, char** res, unsigned argc, char** argv)
{
  if (lua_isfunction(L, -1)) {
    int base = lua_gettop(L);
    for (int i = 0; i < argc; ++i) {
      char* exp = gmk_expand(argv[i]);
      lua_pushstring(L, exp);
      gmk_free(exp);
    }
    int Nargs = lua_gettop(L) - base;
    if (lua_pcall(L, Nargs, LUA_MULTRET, 0)) {
      error(lua_tostring(L, -1));
      return 0;
    }
    if (lua_gettop(L) >= base) {
      size_t sz;
      const char* str = lua_tolstring(L, base, &sz);
      if (str && sz) {
        *res = gmk_alloc(sz + 1);
        memcpy(*res, str, sz + 1);
      }
    }
    return 1;
  }
  return 0;
}

/** function for invoking `export`ed lua functions.
 * Dispatches on @param name by looking it up in lua's globals
 * table.
 */
static char *
dispatch(const char *name, unsigned argc, char **argv)
{
  int top = lua_gettop(L);
  char* ret = NULL;

  lua_getglobal(L, name);

  (void) pcall(L, &ret, argc, argv);

  lua_settop(L, top);
  return ret;
}

/** export a lua function as a make function
 * 
 *    export("foo", bar)
 * 
 * will effectively do `_G["foo"] = bar`, allowing it to be
 * found by `dispatch()`. Afterwards, you can use
 *    $(foo a,b,3)
 * from your Makefile. This will result in bar being called
 * like
 *    bar("a", "b", "3")
 * 
 */
static int
lua_export(lua_State* L)
{
  const char* name = lua_tostring(L, 1);
  lua_setglobal(L, name);
  gmk_add_function (name, dispatch, 1, 8, GMK_FUNC_NOEXPAND);
  lua_settop(L, 0);
  return 0;
}

/** evaluate a lua expression, passing arguments 2..N
 * 
 * In
 *   $(lua expr,a,b,3)
 * `expr` will be loaded as a chunk. This chunk will be executed, passing
 * the list
 *   "a", "b", "3"
 * as arguments. This means you can refer to those from `expr` via `...`
 * So
 *   $(lua print(...),a,b,3)
 * will print
 *   a       b       3
 * 
 * If `expr` is prefixed with a ` (backtick), `expr` will be preceded with `return`
 * So 
 *   $(lua `1 + 2)
 * is the same as
 *   $(lua return 1 + 2)
 * 
 */
static char *
lua(const char *nm, unsigned argc, char **argv)
{
  char* ret = NULL;
  int top = lua_gettop(L);
  char* expanded = NULL;
  const char* script = argv[0];

  if (script[0] == '`') {
    lua_pushliteral(L, "return ");
    lua_pushstring(L, script + 1);
    lua_concat(L, 2);
    script = lua_tostring(L, -1);
  }
  expanded = gmk_expand(script);

  if (!luaL_loadstring(L, expanded)) {
    (void) pcall(L, &ret, argc - 1, argv + 1);
  } else {
    error(lua_tostring(L, -1));
  }

  gmk_free(expanded);
  lua_settop(L, top);
  return ret;
}

LUA_MAKE_EXPORT int
lua_make_gmk_setup ()
{
  L = luaL_newstate();
  luaL_openlibs(L);

  lua_newtable(L);
  {
    lua_pushcfunction(L, lua_eval);
    lua_setglobal(L, "eval");
  }
  {
    lua_pushcfunction(L, lua_export);
    lua_setglobal(L, "export");
  }
  {
    lua_pushcfunction(L, lua_expand);
    lua_setglobal(L, "expand");
  }
  int env = lua_gettop(L);
  lua_newtable(L);
  int mt = lua_gettop(L);
  {
    lua_pushcfunction(L, lua_lookup);
    lua_setfield(L, mt, "__index");
  }
  lua_setmetatable(L, env);
  lua_setglobal(L, "make");

  gmk_add_function ("lua", lua, 1, 8, GMK_FUNC_NOEXPAND);
  return 1;
}
