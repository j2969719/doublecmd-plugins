#define _DEFAULT_SOURCE
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <errno.h>
#include <utime.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "wfxplugin.h"

#include <libspeechd.h>

#define DEFINE_INT(name, value) lua_pushinteger(L, value); lua_setfield(L, -2, name)
#define DEFINE_STR(name, value) lua_pushstring(L, value);  lua_setfield(L, -2, name)
#define BUFSIZE 8192

static gboolean trash_file(const char *filename)
{
	gboolean result = FALSE;
	GFile *gfile = g_file_new_for_path(filename);

	if (gfile)
	{
		result = g_file_trash(gfile, NULL, NULL);
		g_object_unref(gfile);
	}

	return result;
}

static int copy_file(const char* src, const char* dst)
{
	int ifd, ofd;
	ssize_t len;
	char buff[BUFSIZE];
	struct stat buf;
	int result = FS_FILE_OK;

	if (strcmp(src, dst) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (stat(src, &buf) != 0)
		return FS_FILE_READERROR;

	ifd = open(src, O_RDONLY);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (ofd > -1)
	{
		while ((len = read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
			{
				result = FS_FILE_WRITEERROR;
				break;
			}
		}

		close(ofd);
		chmod(dst, buf.st_mode);
	}
	else
		result = FS_FILE_WRITEERROR;

	close(ifd);

	return result;
}


static int lua_witch(lua_State *L)
{
	const char *arg = luaL_checkstring(L, 1);

	if (arg)
	{
		gchar *string = g_find_program_in_path(arg);

		if (string)
		{
			lua_pushstring(L, string);
			g_free(string);
		}
		else
			lua_pushnil(L);
	}
	else
		lua_pushnil(L);

	return 1;
}

static int lua_trash_file(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	if (filename)
		lua_pushboolean(L, trash_file(filename));
	else
		lua_pushboolean(L, FALSE);

	return 1;
}

static int lua_copy_file(lua_State *L)
{
	const char *src = luaL_checkstring(L, 1);
	const char *dst = luaL_checkstring(L, 2);

	if (src && dst)
		lua_pushboolean(L, copy_file(src, dst) == FS_FILE_OK);
	else
		lua_pushboolean(L, FALSE);

	return 1;
}

static int lua_grab_props(lua_State *L)
{
	gboolean is_pushnoi = FALSE;
	const char *filename = luaL_checkstring(L, 1);

	if (filename)
	{
		GFile *gfile = g_file_new_for_path(filename);

		if (gfile)
		{
			GFileInfo *fileinfo = g_file_query_info(gfile, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

			if (fileinfo)
			{
				gchar **attrs = g_file_info_list_attributes(fileinfo, NULL);
				guint len = g_strv_length(attrs);
				lua_createtable(L, 0, len);
				is_pushnoi = TRUE;

				for (guint i = 0; i < len; i++)
				{
					GFileAttributeType type = g_file_info_get_attribute_type(fileinfo, attrs[i]);

					if (type == G_FILE_ATTRIBUTE_TYPE_BOOLEAN)
					{
						lua_pushboolean(L, g_file_info_get_attribute_boolean(fileinfo, attrs[i]));
						lua_setfield(L, -2, attrs[i]);
					}
					/*else if (type == G_FILE_ATTRIBUTE_TYPE_UINT32 && g_strcmp0(attrs[i], "unix::mode") == 0)
					{
						int octal = 0;
						int decimal = g_file_info_get_attribute_uint32(fileinfo, attrs[i]);
						int base = 1;

						while (decimal != 0)
						{
							octal += (decimal % 8) * base;
							decimal /= 8;
							base *= 10;
						}

						lua_pushnumber(L, octal);
						lua_setfield(L, -2, attrs[i]);
					}*/
					else if (type == G_FILE_ATTRIBUTE_TYPE_UINT32)
					{
						lua_pushnumber(L, g_file_info_get_attribute_uint32(fileinfo, attrs[i]));
						lua_setfield(L, -2, attrs[i]);
					}
					else if (type == G_FILE_ATTRIBUTE_TYPE_INT32)
					{
						lua_pushnumber(L, g_file_info_get_attribute_int32(fileinfo, attrs[i]));
						lua_setfield(L, -2, attrs[i]);
					}
					else if (type == G_FILE_ATTRIBUTE_TYPE_UINT64)
					{
						lua_pushnumber(L, g_file_info_get_attribute_uint64(fileinfo, attrs[i]));
						lua_setfield(L, -2, attrs[i]);
					}
					else if (type == G_FILE_ATTRIBUTE_TYPE_INT64)
					{
						lua_pushnumber(L, g_file_info_get_attribute_int64(fileinfo, attrs[i]));
						lua_setfield(L, -2, attrs[i]);
					}
					else
					{
						gchar *string = g_file_info_get_attribute_as_string(fileinfo, attrs[i]);

						if (string)
						{
							lua_pushstring(L, string);
							g_free(string);
							lua_setfield(L, -2, attrs[i]);
						}
					}
				}

				g_strfreev(attrs);
				g_object_unref(fileinfo);
			}

			g_object_unref(gfile);
		}
	}

	if (!is_pushnoi)
		lua_pushnil(L);

	return 1;
}

static int lua_human_size(lua_State *L)
{
	gint64 size = (gint64)lua_tonumber(L, 1);
	gchar *string = g_format_size_full(size, G_FORMAT_SIZE_IEC_UNITS);
	lua_pushstring(L, string);
	g_free(string);

	return 1;
}

static int lua_get_output(lua_State *L)
{
	int argc = lua_gettop(L);
	const char *command = luaL_checkstring(L, 1);

	if (!command)
	{
		lua_pushnil(L);
		return 1;
	}

	gchar **envp = NULL;
	char *argv[] = {"sh", "-c", (char*)command, NULL};

	if (argc > 1 && lua_istable(L, 2))
	{
		/*while (lua_next(L, 1) != 0)
		{
			const char *var = luaL_checkstring(L, 
			const char *value = luaL_checkstring(L, 
			envp = g_environ_setenv(envp ? envp : g_get_environ(), var, value, TRUE);
			
		}*/
	}

	gchar *string = NULL;
	g_spawn_sync(NULL, argv, envp, G_SPAWN_SEARCH_PATH_FROM_ENVP, NULL, NULL, &string, NULL, NULL, NULL);

	if (envp)
		g_strfreev(envp);

	if (string)
		lua_pushstring(L, string);
	else
		lua_pushnil(L);

	return 1;
}

static int dir_iterator(lua_State *L)
{
	struct dirent *entry;
	DIR **pointer = (DIR**)lua_touserdata(L, lua_upvalueindex(1));

	if (!pointer || !*pointer)
		return 0;

	while ((entry = readdir(*pointer)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		else
			break;
	}

	if (!entry)
	{
		closedir(*pointer);
		*pointer = NULL;
		return 0;
	}

	lua_pushstring(L, entry->d_name);

	switch (entry->d_type)
	{
	case DT_REG:
		lua_pushstring(L, "reg");
		break;

	case DT_DIR:
		lua_pushstring(L, "dir");
		break;

	case DT_FIFO:
		lua_pushstring(L, "fifo");
		break;

	case DT_CHR:
		lua_pushstring(L, "chr");
		break;

	case DT_BLK:
		lua_pushstring(L, "blk");
		break;

	default:
		lua_pushstring(L, "unknown");
		break;
	}

	return 2;
}

static int lua_list_dir(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	DIR *dir = opendir(path);

	if (!dir)
		return luaL_error(L, "opendir fail (%s)", path);

	DIR **pointer = (DIR**)lua_newuserdata(L, sizeof(DIR*));
	*pointer = dir;
	lua_pushcclosure(L, dir_iterator, 1);

	return 1;
}

static const struct luaL_Reg shitcode[] =
{
	{"which",	     lua_witch},
	{"trash_file",	lua_trash_file},
	{"copy_file",	 lua_copy_file},
	{"grab_props",	lua_grab_props},
	{"human_size",	lua_human_size},
//	{"get_output",	lua_get_output},
	{"list_dir",	  lua_list_dir},
	{NULL,			  NULL}
};

int LUAOPEN_NAME(lua_State *L)
{
#if LUA_VERSION_NUM == 501
	lua_newtable(L);
	luaL_register(L, NULL, shitcode);
#else
	luaL_newlib(L, shitcode);
#endif
	DEFINE_STR("user_cache_dir", g_get_user_cache_dir());
	DEFINE_STR("user_data_dir", g_get_user_data_dir());
	DEFINE_STR("user_config_dir", g_get_user_config_dir());
	DEFINE_STR("user_state_dir", g_get_user_state_dir());
	DEFINE_STR("user_runtime_dir", g_get_user_runtime_dir());

	return 1;
}
