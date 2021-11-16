#include <git2.h>
#include <unistd.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <string.h>
#include "wdxplugin.h"

#define detectstring "EXT=\"*\""

typedef struct sfield
{
	char *name;
	int type;
	char *unit;
} tfield;

#define fieldcount (sizeof(fields)/sizeof(tfield))

tfield fields[] =
{
	{"bare repository",			ft_boolean,			""},
	{"empty repository",			ft_boolean,			""},
	{"linked work tree",			ft_boolean,			""},
	{"shallow clone",			ft_boolean,			""},
	{"current branch is unborn",		ft_boolean,			""},
	{"repository's HEAD is detached",	ft_boolean,			""},
	{"branch",				ft_string,			""},
	{"remote",				ft_string,			""},
	{"commits ahead",			ft_numeric_32,			""},
	{"commits behind",			ft_numeric_32,			""},
	{"status: new",				ft_boolean,			""},
	{"status: modified",			ft_boolean,			""},
	{"status: deleted",			ft_boolean,			""},
	{"status: typechange",			ft_boolean,			""},
	{"status: renamed",			ft_boolean,			""},
	{"status: unreadable",			ft_boolean,			""},
	{"status: ignored",			ft_boolean,			""},
	{"status: conflicted",			ft_boolean,			""},
	{"root workdir",			ft_boolean,			""},
};

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
	strlcpy(DetectString, detectstring, maxlen);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int ret;
	bool fieldempty = false;
	unsigned int status_flags;
	size_t ahead, behind;
	git_buf repo_buf = {};
	git_repository *repo = NULL;
	git_reference *head = NULL;
	git_strarray remote;
	git_oid upstream;
	const git_oid *local;
	char path_temp[PATH_MAX];
	const char *string = NULL;

	if (git_repository_discover(&repo_buf, FileName, 1, NULL) != 0)
		return ft_fieldempty;

	if (git_repository_open(&repo, repo_buf.ptr) != 0)
	{
		git_buf_free(&repo_buf);
		return ft_fieldempty;
	}

	git_buf_free(&repo_buf);

	switch (FieldIndex)
	{
	case 0:
		*(int*)FieldValue = git_repository_is_bare(repo);
		break;

	case 1:
		*(int*)FieldValue = git_repository_is_empty(repo);
		break;

	case 2:
		*(int*)FieldValue = git_repository_is_worktree(repo);
		break;

	case 3:
		*(int*)FieldValue = git_repository_is_shallow(repo);
		break;

	case 4:
		*(int*)FieldValue = git_repository_head_unborn(repo);
		break;

	case 5:
		*(int*)FieldValue = git_repository_head_detached(repo);
		break;

	case 6:
		if (git_repository_head(&head, repo) == 0)
		{
			string = git_reference_shorthand(head);
			git_reference_free(head);
		}

		if (!string)
			fieldempty = true;
		else
			strlcpy((char*)FieldValue, string, maxlen - 1);

		break;

	case 7:
		if (git_repository_head(&head, repo) == 0)
		{
			git_remote_list(&remote, repo);
			git_reference_free(head);
		}

		if (!remote.strings[0])
			fieldempty = true;
		else
			strlcpy((char*)FieldValue, remote.strings[0], maxlen - 1);

		break;

	case 8:
		if (git_repository_head(&head, repo) == 0)
		{
			string = git_reference_shorthand(head);
			git_remote_list(&remote, repo);

			if (remote.count)
			{
				local = git_reference_target(head);
				strcpy(path_temp, "refs/remotes/");

				if (remote.strings[0])
					strcat(path_temp, remote.strings[0]);

				strcat(path_temp, "/");
				strcat(path_temp, string);
				ret = git_reference_name_to_id(&upstream, repo, path_temp);

				if (local && !ret)
					ret = git_graph_ahead_behind(&ahead, &behind, repo, local, &upstream);
			}

			git_reference_free(head);
		}

		if (ret != 0)
			fieldempty = true;
		else
			*(int*)FieldValue = ahead;


		break;

	case 9:
		if (git_repository_head(&head, repo) == 0)
		{
			string = git_reference_shorthand(head);
			git_remote_list(&remote, repo);

			if (remote.count)
			{
				local = git_reference_target(head);
				strcpy(path_temp, "refs/remotes/");

				if (remote.strings[0])
					strcat(path_temp, remote.strings[0]);

				strcat(path_temp, "/");
				strcat(path_temp, string);
				ret = git_reference_name_to_id(&upstream, repo, path_temp);

				if (local && !ret)
					ret = git_graph_ahead_behind(&ahead, &behind, repo, local, &upstream);
			}

			git_reference_free(head);
		}

		if (ret != 0)
			fieldempty = true;
		else
			*(int*)FieldValue = behind;

		break;

	case 10:
		string = git_repository_workdir(repo);

		if (string && git_status_file(&status_flags, repo, FileName + strlen(string)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_NEW)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;

		break;

	case 11:
		string = git_repository_workdir(repo);

		if (string && git_status_file(&status_flags, repo, FileName + strlen(string)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_MODIFIED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;

		break;

	case 12:
		string = git_repository_workdir(repo);

		if (string && git_status_file(&status_flags, repo, FileName + strlen(string)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_DELETED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;

		break;

	case 13:
		string = git_repository_workdir(repo);

		if (string && git_status_file(&status_flags, repo, FileName + strlen(string)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_TYPECHANGE)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;

		break;

	case 14:
		string = git_repository_workdir(repo);

		if (string && git_status_file(&status_flags, repo, FileName + strlen(string)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_RENAMED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;

		break;

	case 15:
		string = git_repository_workdir(repo);

		if (string && git_status_file(&status_flags, repo, FileName + strlen(string)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_UNREADABLE)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;

		break;

	case 16:
		string = git_repository_workdir(repo);

		if (string && git_status_file(&status_flags, repo, FileName + strlen(string)) == 0)
		{
			if (status_flags & GIT_STATUS_IGNORED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;
		break;

	case 17:
		string = git_repository_workdir(repo);

		if (string && git_status_file(&status_flags, repo, FileName + strlen(string)) == 0)
		{
			if (status_flags & GIT_STATUS_CONFLICTED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;

		break;

	case 18:
		string = git_repository_workdir(repo);
		strcpy(path_temp, FileName);
		strcat(path_temp, "/");

		if (string)
		{
			if (strcmp(path_temp, string) == 0)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
			fieldempty = true;

		break;

	default:
		git_repository_free(repo);
		return ft_nosuchfield;
	}

	git_repository_free(repo);

	if (fieldempty)
		return ft_fieldempty;

	return fields[FieldIndex].type;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	git_libgit2_init();
}

void DCPCALL ContentPluginUnloading()
{
	git_libgit2_shutdown();
}
