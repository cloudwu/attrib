#include <lua.h>
#include <lauxlib.h>

//#include "luacompat52.h"
#include "attrib.h"

static int
_attrib(lua_State *L) {
	struct attrib ** box = lua_newuserdata(L, sizeof(struct attrib *));
	*box = attrib_new();
	struct attrib_e ** e = lua_touserdata(L,1);
	if (e == NULL) {
		luaL_error(L, "Need expression");
	}
	if (*e == NULL) {
		luaL_error(L, "Empty expression");
	}

	lua_pushvalue(L, lua_upvalueindex(1));
	lua_setmetatable(L,-2);

	attrib_attach(*box, *e);

	return 1;
}

static int
_attrib_delete(lua_State *L) {
	struct attrib **box = lua_touserdata(L,1);
	if (*box) {
		attrib_delete(*box);
		*box = NULL;
	}

	return 0;
}

static int
_attrib_read(lua_State *L) {
	struct attrib **box = lua_touserdata(L,1);
	int r = luaL_checkinteger(L,2);
	float v = attrib_read(*box, r);
	lua_pushnumber(L,v);

	return 1;
}

static int
_attrib_write(lua_State *L) {
	struct attrib **box = lua_touserdata(L,1);
	int r = luaL_checkinteger(L,2);
	float v = luaL_checknumber(L,3);
	attrib_write(*box, r,v);

	return 0;
}

static int
_attrib_inc(lua_State *L) {
	struct attrib **box = lua_touserdata(L,1);
	int r = luaL_checkinteger(L,2);
	float v = luaL_checknumber(L,3);
	float old = attrib_read(*box, r);
	attrib_write(*box, r, v+old);

	lua_pushnumber(L, v+old);

	return 1;
}

static struct attrib_e *
_new_expression(lua_State *L, int idx) {
	luaL_checktype(L,idx, LUA_TTABLE);
	struct attrib_e * e = attrib_enew();
	int n = lua_rawlen(L, idx);
	int i;
	for (i=0;i<n;i+=2) {
		lua_rawgeti(L, idx, i + 1);
		int r = luaL_checkinteger(L, -1);
		lua_pop(L,1);
		lua_rawgeti(L, idx, i + 2);
		const char * exp = luaL_checkstring(L, -1);
		lua_pop(L,1);
		const char * err = attrib_epush(e, r, exp);
		if (err) {
			attrib_edelete(e);
			luaL_error(L, "R%d = %s %s",r, exp, err);
		}
	}
	const char * err = attrib_einit(e);
	if (err) {
		attrib_edelete(e);
		luaL_error(L, "%s", err);
	}
	return e;
}

static int
_expression(lua_State *L) {
	struct attrib_e ** box = lua_newuserdata(L, sizeof(struct attrib_e *));
	luaL_checktype(L,1,LUA_TTABLE);
	*box = _new_expression(L, 1);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_setmetatable(L,-2);

	return 1;
}

static int
_expression_delete(lua_State *L) {
	struct attrib_e ** box = lua_touserdata(L,1);
	if (*box) {
		attrib_edelete(*box);
		*box = NULL;
	}
	return 0;
}

static int
_expression_update(lua_State *L) {
	struct attrib_e ** box = lua_touserdata(L,1);
	if (*box) {
		if (lua_toboolean(L,3)==0) {
			attrib_edelete(*box);
			*box = NULL;
		}
	}

	*box = _new_expression(L,2);

	return 0;
}

static void
_pushattrib(lua_State *L) {
	lua_createtable(L,0,3);
	lua_pushcfunction(L,_attrib_read);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,_attrib_write);
	lua_setfield(L,-2,"__newindex");
	lua_pushcfunction(L,_attrib_delete);
	lua_setfield(L,-2,"__gc");
	lua_pushcfunction(L,_attrib_inc);
	lua_setfield(L,-2,"__call");
	lua_pushcclosure(L, _attrib, 1);
}

static void
_pushexpression(lua_State *L) {
	lua_createtable(L,0,2);
	lua_pushcfunction(L,_expression_update);
	lua_setfield(L,-2,"__call");
	lua_pushcfunction(L,_expression_delete);
	lua_setfield(L,-2,"__gc");
	lua_pushcclosure(L, _expression, 1);
}

int
luaopen_attrib_c(lua_State *L) {
	luaL_checkversion(L);

	lua_createtable(L,0,2);
	_pushattrib(L);
	lua_setfield(L,-2,"attrib");
	_pushexpression(L);
	lua_setfield(L,-2,"expression");

	return 1;
}

