#include <git2.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include "wdxplugin.h"

#define detect_string "EXT=\"*\""

typedef struct s_field
{
	char *name;
	int type;
	char *unit;
} t_field;

typedef struct s_cache
{
	unsigned int status_flags;
	char lastfile[PATH_MAX];
	char dirname[PATH_MAX];
	git_repository *repo;
	const char *workdir;
} t_cache;

#define fieldcount (sizeof(fields)/sizeof(t_field))

t_field fields[] =
{
	{"bare repository",			ft_boolean,	""},
	{"empty repository",			ft_boolean,	""},
	{"linked work tree",			ft_boolean,	""},
	{"shallow clone",			ft_boolean,	""},
	{"current branch is unborn",		ft_boolean,	""},
	{"repository's HEAD is detached",	ft_boolean,	""},
	{"branch",				ft_string,	""},
	{"remote",				ft_string,	""},
	{"commits ahead",			ft_numeric_32,	""},
	{"commits behind",			ft_numeric_32,	""},
	{"status: new",				ft_boolean,	""},
	{"status: modified",			ft_boolean,	""},
	{"status: deleted",			ft_boolean,	""},
	{"status: typechange",			ft_boolean,	""},
	{"status: renamed",			ft_boolean,	""},
	{"status: unreadable",			ft_boolean,	""},
	{"status: ignored",			ft_boolean,	""},
	{"status: conflicted",			ft_boolean,	""},
	{"workdir",				ft_string,	""},
	{"filepath",				ft_string,	""},
};

t_cache *cachedata;

char* strlcpy(char* p, const char* p2, int maxlen)
{
	if ((int)strlen(p2) >= maxlen)
	{
		strncpy(p, p2, maxlen);
		p[maxlen] = 0;
	}
	else
		strcpy(p, p2);

	return p;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	strlcpy(Units, fields[FieldIndex].unit, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	strlcpy(DetectString, detect_string, maxlen - 1);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int ret;
	size_t ahead, behind;
	git_reference *head = NULL;
	git_strarray remote;
	git_oid upstream;
	const git_oid *local;
	char path_temp[PATH_MAX];
	const char *string = NULL;

	if (strcmp(FileName, cachedata->lastfile) != 0)
	{
		strlcpy(path_temp, FileName, PATH_MAX);
		char *current_dir = dirname(path_temp);

		if (strcmp(current_dir, cachedata->dirname) != 0)
		{
			if (cachedata->repo != NULL)
			{
				git_repository_free(cachedata->repo);
				cachedata->repo = NULL;
			}

			git_buf repo_buf = {};

			if (git_repository_discover(&repo_buf, current_dir, 1, NULL) == 0)
			{
				git_repository_open(&cachedata->repo, repo_buf.ptr);
				git_buf_free(&repo_buf);
			}

			strlcpy(cachedata->dirname, current_dir, PATH_MAX);
		}

		if (cachedata->repo != NULL)
		{
			cachedata->workdir = git_repository_workdir(cachedata->repo);
			memset(path_temp, 0, PATH_MAX);
			realpath(FileName, path_temp);

			if (cachedata->workdir)
				git_status_file(&cachedata->status_flags, cachedata->repo, path_temp + strlen(cachedata->workdir));
		}

		strlcpy(cachedata->lastfile, FileName, PATH_MAX);
	}

	if (cachedata->repo == NULL)
		return ft_fileerror;

	switch (FieldIndex)
	{
	case 0:
		*(int*)FieldValue = git_repository_is_bare(cachedata->repo);
		break;

	case 1:
		*(int*)FieldValue = git_repository_is_empty(cachedata->repo);
		break;

	case 2:
		*(int*)FieldValue = git_repository_is_worktree(cachedata->repo);
		break;

	case 3:
		*(int*)FieldValue = git_repository_is_shallow(cachedata->repo);
		break;

	case 4:
		*(int*)FieldValue = git_repository_head_unborn(cachedata->repo);
		break;

	case 5:
		*(int*)FieldValue = git_repository_head_detached(cachedata->repo);
		break;

	case 6:
		if (git_repository_head(&head, cachedata->repo) == 0)
		{
			string = git_reference_shorthand(head);
			git_reference_free(head);
		}

		if (!string)
			return ft_fieldempty;
		else
			strlcpy((char*)FieldValue, string, maxlen - 1);

		break;

	case 7:
		if (git_repository_head(&head, cachedata->repo) == 0)
		{
			git_remote_list(&remote, cachedata->repo);
			git_reference_free(head);
		}

		if (!remote.strings[0])
			return ft_fieldempty;
		else
			strlcpy((char*)FieldValue, remote.strings[0], maxlen - 1);

		break;

	case 8:
		if (git_repository_head(&head, cachedata->repo) == 0)
		{
			string = git_reference_shorthand(head);
			git_remote_list(&remote, cachedata->repo);

			if (remote.count)
			{
				local = git_reference_target(head);
				strcpy(path_temp, "refs/remotes/");

				if (remote.strings[0])
					strcat(path_temp, remote.strings[0]);

				strcat(path_temp, "/");
				strcat(path_temp, string);
				ret = git_reference_name_to_id(&upstream, cachedata->repo, path_temp);

				if (local && !ret)
					ret = git_graph_ahead_behind(&ahead, &behind, cachedata->repo, local, &upstream);
			}

			git_reference_free(head);
		}

		if (ret != 0)
			return ft_fieldempty;
		else
			*(int*)FieldValue = ahead;


		break;

	case 9:
		if (git_repository_head(&head, cachedata->repo) == 0)
		{
			string = git_reference_shorthand(head);
			git_remote_list(&remote, cachedata->repo);

			if (remote.count)
			{
				local = git_reference_target(head);
				strcpy(path_temp, "refs/remotes/");

				if (remote.strings[0])
					strcat(path_temp, remote.strings[0]);

				strcat(path_temp, "/");
				strcat(path_temp, string);
				ret = git_reference_name_to_id(&upstream, cachedata->repo, path_temp);

				if (local && !ret)
					ret = git_graph_ahead_behind(&ahead, &behind, cachedata->repo, local, &upstream);
			}

			git_reference_free(head);
		}

		if (ret != 0)
			return ft_fieldempty;
		else
			*(int*)FieldValue = behind;

		break;

	case 10:
		if (cachedata->status_flags & GIT_STATUS_WT_NEW)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 11:
		if (cachedata->status_flags & GIT_STATUS_WT_MODIFIED)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 12:
		if (cachedata->status_flags & GIT_STATUS_WT_DELETED)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 13:
		if (cachedata->status_flags & GIT_STATUS_WT_TYPECHANGE)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 14:
		if (cachedata->status_flags & GIT_STATUS_WT_RENAMED)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 15:
		if (cachedata->status_flags & GIT_STATUS_WT_UNREADABLE)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 16:
		if (cachedata->status_flags & GIT_STATUS_IGNORED)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 17:
		if (cachedata->status_flags & GIT_STATUS_CONFLICTED)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 18:
		if (cachedata->workdir != NULL)
			strlcpy((char*)FieldValue, cachedata->workdir, maxlen - 1);
		else
			return ft_fieldempty;

		break;

	case 19:
		if (cachedata->workdir != NULL)
		{
			memset(path_temp, 0, PATH_MAX);
			realpath(FileName, path_temp);
			strlcpy((char*)FieldValue, path_temp + (strlen(cachedata->workdir) - 1), maxlen - 1);
		}
		else
			return ft_fieldempty;

		break;

	default:
		return ft_nosuchfield;
	}

	return fields[FieldIndex].type;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	cachedata = malloc(sizeof(t_cache));
	memset(cachedata, 0, sizeof(t_cache));
	git_libgit2_init();
}

void DCPCALL ContentPluginUnloading()
{
	if (cachedata->repo != NULL)
		git_repository_free(cachedata->repo);

	free(cachedata);
	git_libgit2_shutdown();
}
