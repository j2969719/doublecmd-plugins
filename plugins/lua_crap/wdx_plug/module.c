#define _DEFAULT_SOURCE
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <glib.h>
#include <dlfcn.h>
#include "wdxplugin.h"

#define DEFINE_INT(name, value) lua_pushinteger(L, value); lua_setfield(L, -2, name)
#define DEFINE_STR(name, value) lua_pushstring(L, value);  lua_setfield(L, -2, name)
#define WDX_MAX_LEN 2048

typedef int (*tContentGetDetectString)(char* DetectString, int maxlen);
typedef int (*tContentGetSupportedField)(int FieldIndex, char* FieldName, char* Units, int maxlen);
typedef int (*tContentGetValue)(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags);
typedef int (*tContentGetValueW)(WCHAR* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags);
typedef void (*tContentSetDefaultParams)(ContentDefaultParamStruct* dps);
typedef void (*tContentPluginUnloading)(void);

typedef struct
{
	char name[WDX_MAX_LEN];
	char units[WDX_MAX_LEN];
	int type;
} ContentField;

typedef struct
{
	void* handle;
	char detectstring[WDX_MAX_LEN];
	int fields_count;
	ContentField *fields;
	tContentGetDetectString ContentGetDetectString;
	tContentGetSupportedField ContentGetSupportedField;
	tContentGetValue ContentGetValue;
	tContentGetValueW ContentGetValueW;
	tContentSetDefaultParams ContentSetDefaultParams;
	tContentPluginUnloading ContentPluginUnloading;
} PlugData;

static WCHAR* utf8_to_wchar(char* string)
{
	if (!string)
		return NULL;

	gsize bytes_written = 0;
	return (WCHAR*)g_convert(string, -1, "UTF-16LE", "UTF-8", NULL, &bytes_written, NULL);
}

static gchar* wchar_to_utf8(WCHAR* string)
{
	if (!string)
		return NULL;

	gsize u16_len = 0;

	while (string[u16_len] != 0)
		u16_len++;

	return g_convert((const char*)string, u16_len * 2, "UTF-8", "UTF-16LE", NULL, NULL, NULL);
}

static int call_getvalue(int index, int unit, char *filename, PlugData *data, char *buf)
{
	int result = ft_fileerror;

	if (data->ContentGetValueW)
	{
		WCHAR *filenamew = utf8_to_wchar(filename);
		result = data->ContentGetValueW(filenamew, index, unit, buf, WDX_MAX_LEN, 0);
		g_free(filenamew);
	}
	else
		result = data->ContentGetValue(filename, index, unit, buf, WDX_MAX_LEN, 0);

	if (result == ft_stringw || result == ft_fulltextw)
	{
		gchar *string = wchar_to_utf8((WCHAR*)buf);

		if (string)
		{
			g_strlcpy(buf, string, WDX_MAX_LEN);
			g_free(string);
		}
		else
			result = ft_fieldempty;
	}

	return result;
}

static gboolean get_value(lua_State *L, int index, int units, char *filename, PlugData *data)
{
	gboolean result = FALSE;

	if (index < 0 || units < 0 || !filename)
		return FALSE;

	ContentField field = data->fields[index];
	char buf[WDX_MAX_LEN * 2];
	memset(buf, 0, sizeof(buf));

	if (field.type != ft_fulltext && field.type != ft_fulltextw)
	{
		int ret = call_getvalue(index, units, filename, data, buf);

		switch (ret)
		{
		case ft_stringw:
		case ft_string:
		case ft_multiplechoice:
			lua_pushstring(L, buf);
			result = TRUE;
			break;

		case ft_boolean:
		{
			int* value = (int*)buf;
			lua_pushboolean(L, *value != 0);
			result = TRUE;
			break;
		}

		case ft_numeric_32:
		{
			int* value = (int*)buf;
			lua_pushnumber(L, *value);
			result = TRUE;
			break;
		}

		case ft_numeric_64:
		{
			gint64* value = (gint64*)buf;
			lua_pushnumber(L, *value);
			result = TRUE;
			break;
		}

		case ft_numeric_floating:
		{
			double* value = (double*)buf;
			lua_pushnumber(L, *value);
			result = TRUE;
			break;
		}

		case ft_datetime:
		{
			PFILETIME value = (PFILETIME)buf;
			guint64 unixtime = ((guint64)value->dwHighDateTime << 32) | value->dwLowDateTime;

			if (unixtime >= 0x019DB1DED53E8000ULL)
			{
				unixtime -= 0x019DB1DED53E8000ULL;
				lua_pushnumber(L, unixtime / 10000000ULL);
				result = TRUE;
			}

			break;
		}

		case ft_time:
		{
			char string[19];
			ptimeformat value = (ptimeformat)buf;
			snprintf(string, sizeof(string), "%02d:%02d:%02d", value->wHour, value->wMinute, value->wSecond);
			lua_pushstring(L, string);
			result = TRUE;
			break;
		}

		case ft_date:
		{
			char string[19];
			pdateformat value = (pdateformat)buf;
			snprintf(string, sizeof(string), "%4d:%02d:%02d", value->wYear, value->wMonth, value->wDay);
			lua_pushstring(L, string);
			result = TRUE;
			break;
		}
		case ft_fulltext:
		case ft_fulltextw:
			g_printerr("dafaq its fulltext\n");
		}
	}
	else
	{
		int offset = 0;
		int ret = ft_fileerror;
		luaL_Buffer luabuf;
		luaL_buffinit(L, &luabuf);

		do
		{
			ret = call_getvalue(index, offset, filename, data, buf);

			if (ret == ft_fulltext)
			{
				size_t len = strlen(buf);
				luaL_addstring(&luabuf, buf);
				offset += len;
			}
			else if (ret == ft_fulltextw)
			{
				gchar *string = wchar_to_utf8((WCHAR*)buf);

				if (string)
				{
					size_t len = strlen(buf);
					luaL_addstring(&luabuf, buf);
					offset += len;
				}
			}
		}
		while (ret == ft_fulltext || ret == ft_fulltextw);

//		if (ret != ft_fieldempty)
//			call_getvalue(index, -1, filename, data, buf);

		if (ret == ft_fieldempty)
		{
			luaL_pushresult(&luabuf);
			result = TRUE;
		}
	}

	return result;
}

static int get_field_index(PlugData *data, const char *name)
{
	if (!name || !data)
		return -1;

	for (int i = 0; i < data->fields_count; i++)
	{
		if (g_strcmp0(name, data->fields[i].name) == 0)
			return i;
	}

	return -1;
}

static int get_units_index(PlugData *data, int i, const char *unit)
{
	if (!data)
		return -1;

	if (!unit || data->fields[i].units[0] == '\0' || data->fields[i].type == ft_multiplechoice || \
	                data->fields[i].type == ft_fulltext || data->fields[i].type == ft_fulltextw)
		return 0;

	gchar **split = g_strsplit(data->fields[i].units, "|", -1);
	guint len = g_strv_length(split);

	for (guint n = 0; n < len; n++)
	{
		if (g_strcmp0(unit, split[n]) == 0)
			return n;
	}

	g_strfreev(split);

	return -1;
}

static PlugData* load_plugin(const char *filename)
{
	void* handle = dlopen(filename, RTLD_LAZY);

	if (handle)
	{
		PlugData *data = (PlugData*)g_new0(PlugData, 1);

		data->handle = handle;
		data->ContentGetDetectString = (tContentGetDetectString)dlsym(handle, "ContentGetDetectString");
		data->ContentGetSupportedField = (tContentGetSupportedField)dlsym(handle, "ContentGetSupportedField");
		data->ContentGetValue = (tContentGetValue)dlsym(handle, "ContentGetValue");
		data->ContentGetValueW = (tContentGetValueW)dlsym(handle, "ContentGetValueW");
		data->ContentSetDefaultParams = (tContentSetDefaultParams)dlsym(handle, "ContentSetDefaultParams");
		data->ContentPluginUnloading = (tContentPluginUnloading)dlsym(handle, "ContentPluginUnloading");

		if (!data->ContentGetSupportedField || (!data->ContentGetValue && !data->ContentGetValueW))
		{
			dlclose(handle);
			g_free(data);
			g_printerr("mandatory api fail (%s)", filename);
		}
		else
		{
			if (data->ContentSetDefaultParams)
			{
				ContentDefaultParamStruct dps;
				memset(&dps, 0, sizeof(ContentDefaultParamStruct));
				snprintf(dps.DefaultIniName, MAX_PATH, "%s/doublecmd/wdx.ini", g_get_user_config_dir());
				dps.PluginInterfaceVersionHi = 1;
				dps.PluginInterfaceVersionLow = 50;
				dps.size = sizeof(ContentDefaultParamStruct);
				data->ContentSetDefaultParams(&dps);
			}

			if (data->ContentGetDetectString)
				data->ContentGetDetectString(data->detectstring, WDX_MAX_LEN);

			char tmp[WDX_MAX_LEN];

			while (data->ContentGetSupportedField(data->fields_count, tmp, tmp, WDX_MAX_LEN) > ft_nomorefields)
				data->fields_count++;

			data->fields = (ContentField*)g_new0(ContentField, data->fields_count);

			for (int i = 0; i < data->fields_count; i++)
				data->fields[i].type = data->ContentGetSupportedField(i, data->fields[i].name, data->fields[i].units, WDX_MAX_LEN);

			return data;
		}
	}

	return NULL;
}

static int lua_load_plug(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	if (filename)
	{
		PlugData *data = load_plugin(filename);

		if (data)
		{
			lua_pushlightuserdata(L, data);
			return 1;
		}
	}

	lua_pushnil(L);

	return 1;
}

static int lua_unload_plug(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
		return 0;

	PlugData *data = (PlugData*)lua_touserdata(L, 1);

	g_free(data->fields);

	if (data->ContentPluginUnloading)
		data->ContentPluginUnloading();

	dlclose(data->handle);
	g_free(data);

	return 0;
}

static int lua_get_fields(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
	{
		lua_pushnil(L);
		return 1;
	}

	PlugData *data = (PlugData*)lua_touserdata(L, 1);

	lua_createtable(L, data->fields_count, 0);

	for (int i = 0; i < data->fields_count; i++)
	{
		lua_createtable(L, 0, 3);
		lua_pushstring(L, data->fields[i].name);
		lua_setfield(L, -2, "name");
		lua_pushnumber(L, data->fields[i].type);
		lua_setfield(L, -2, "type");

		if (data->fields[i].units[0] != '\0')
		{
			gchar **split = g_strsplit(data->fields[i].units, "|", -1);
			guint len = g_strv_length(split);
			lua_createtable(L, len, 0);

			for (guint n = 0; n < len; n++)
			{
				lua_pushstring(L, split[n]);
				lua_rawseti(L, -2, n + 1);
			}

			g_strfreev(split);
			lua_setfield(L, -2, "units");
		}

		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

static int lua_get_value(lua_State *L)
{
	int result = 0;

	if (!lua_islightuserdata(L, 1))
		return result;


	PlugData *data = (PlugData*)lua_touserdata(L, 1);
	const char *filename = luaL_checkstring(L, 2);
	const char *field = luaL_checkstring(L, 3);
	const char *units = lua_tostring(L, 4);
	int index = get_field_index(data, field);
	gchar *fullpath = g_canonicalize_filename(filename, NULL);

	if (get_value(L, index, get_units_index(data, index, units), fullpath, data))
		result++;

	g_free(fullpath);

	return result;
}

static int lua_get_values(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
	{
		lua_pushnil(L);
		return 1;
	}

	PlugData *data = (PlugData*)lua_touserdata(L, 1);
	const char *filename = luaL_checkstring(L, 2);
	gchar *fullpath = g_canonicalize_filename(filename, NULL);
	lua_createtable(L, 0, data->fields_count);

	for (int i = 0; i < data->fields_count; i++)
	{
		if (get_value(L, i, 0, fullpath, data))
			lua_setfield(L, -2, data->fields[i].name);
	}

	g_free(fullpath);

	return 1;
}

static int lua_get_detectstring(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
	{
		lua_pushnil(L);
		return 1;
	}

	PlugData *data = (PlugData*)lua_touserdata(L, 1);
	lua_pushstring(L, data->detectstring);
	return 1;
}

static int lua_get_field_index(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
	{
		lua_pushnil(L);
		return 1;
	}

	int argc = lua_gettop(L);
	PlugData *data = (PlugData*)lua_touserdata(L, 1);
	const char *field = luaL_checkstring(L, 2);
	int index = get_field_index(data, field);
	lua_pushnumber(L, index);

	if (argc > 2)
	{
		const char *units = lua_tostring(L, 3);
		lua_pushnumber(L, get_units_index(data, index, units));
		return 2;
	}

	return 1;
}

static const struct luaL_Reg shitcode[] =
{
	{"load_plug",		lua_load_plug},
	{"unload_plug",	      lua_unload_plug},
	{"detectstring", lua_get_detectstring},
	{"get_fields",	       lua_get_fields},
	{"get_value",		lua_get_value},
	{"get_values",	       lua_get_values},
	{"get_indexes",	  lua_get_field_index},
	{NULL,				 NULL}
};

int LUAOPEN_NAME(lua_State *L)
{
#if LUA_VERSION_NUM == 501
	lua_newtable(L);
	luaL_register(L, NULL, shitcode);
#else
	luaL_newlib(L, shitcode);
#endif
	DEFINE_INT("ft_nomorefields", ft_nomorefields);
	DEFINE_INT("ft_numeric_32", ft_numeric_32);
	DEFINE_INT("ft_numeric_64", ft_numeric_64);
	DEFINE_INT("ft_numeric_floating", ft_numeric_floating);
	DEFINE_INT("ft_date", ft_date);
	DEFINE_INT("ft_time", ft_time);
	DEFINE_INT("ft_boolean", ft_boolean);
	DEFINE_INT("ft_multiplechoice", ft_multiplechoice);
	DEFINE_INT("ft_string", ft_string);
	DEFINE_INT("ft_fulltext", ft_fulltext);
	DEFINE_INT("ft_datetime", ft_datetime);
	DEFINE_INT("ft_stringw", ft_stringw);
	DEFINE_INT("ft_fulltextw", ft_fulltextw);
	DEFINE_INT("CONTENT_DELAYIFSLOW", CONTENT_DELAYIFSLOW);
	return 1;
}
