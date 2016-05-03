#pragma once

#include <stdint.h>
#include <cstring>
#include <string>
#include <sstream>
#include <lua.hpp>
#include <GarrysMod/Lua/Types.h>
#include <GarrysMod/Lua/LuaInterface.h>

namespace helpers
{

inline std::string GetBinaryFileName(
	const std::string &name,
	bool libprefix = true,
	bool srvsuffix = true,
	const std::string &extraprefix = ""
)
{

#if defined _WIN32

	(void)libprefix;
	(void)srvsuffix;
	return extraprefix + name + ".dll";

#elif defined __linux

	return extraprefix + ( libprefix ? "lib" : "" ) + name + ( srvsuffix ? "_srv.so" : ".so" );

#elif defined __APPLE__

	(void)libprefix;
	(void)srvsuffix;
	return extraprefix + name + ".dylib";

#endif

}

static int32_t LuaErrorTraceback( lua_State *state )
{
	std::string spaces( "\n  " );
	std::ostringstream stream;

	if( lua_type( state, 1 ) == LUA_TSTRING )
		stream << lua_tostring( state, 1 );

	lua_Debug dbg = { 0 };
	for(
		int32_t lvl = 1;
		lua_getstack( state, lvl, &dbg ) == 1;
		++lvl, std::memset( &dbg, 0, sizeof( dbg ) )
	)
	{
		if( lua_getinfo( state, "Sln", &dbg ) == 0 )
			break;

		stream
			<< spaces
			<< lvl
			<< ". "
			<< ( dbg.name == nullptr ? "unknown" : dbg.name )
			<< " - "
			<< dbg.short_src
			<< ':'
			<< dbg.currentline;
		spaces += ' ';
	}

	lua_pushstring( state, stream.str( ).c_str( ) );
	return 1;
}

static bool PushHookRun( GarrysMod::Lua::ILuaBase *lua, const char *hook_name )
{
	lua->PushCFunction( LuaErrorTraceback );

	lua->GetField( GarrysMod::Lua::INDEX_GLOBAL, "hook" );
	if( !lua->IsType( -1, GarrysMod::Lua::Type::TABLE ) )
	{
		lua->Pop( 2 );
		return false;
	}

	lua->GetField( -1, "Run" );
	if( !lua->IsType( -1, GarrysMod::Lua::Type::FUNCTION ) )
	{
		lua->Pop( 3 );
		return false;
	}

	lua->Remove( -2 );

	lua->PushString( hook_name );

	return true;
}

static bool CallHookRun(
	GarrysMod::Lua::ILuaBase *lua,
	int32_t args = 0,
	int32_t rets = 0,
	bool print_error = true,
	bool pop_error = true
)
{
	if( lua->PCall( 1 + args, rets, -args - 3 ) != 0 )	/* -args for the last arg, -1 for hook name,
														-1 for hook.Run and -1 for debug.traceback */
	{
		if( print_error )
			static_cast<GarrysMod::Lua::ILuaInterface *>( lua )->ErrorNoHalt(
				"\n%s\n\n",
				lua->GetString( -1 )
			);
		if( pop_error )
			lua->Pop( 2 ); /* pop error string and debug.traceback */
		else
			lua->Remove( -2 ); /* remove debug.traceback */
		return false;
	}

	lua->Remove( -rets - 1 ); /* remove debug.traceback and leave the returns in the stack */
	return true;
}

}
