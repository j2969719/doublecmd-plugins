#include <git2.h>
#include <unistd.h>
#include <string.h>
#include "wdxplugin.h"

#define _detectstring "EXT=\"*\""
#define PATH_LENGTH 4096

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
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
	strlcpy(DetectString, _detectstring, maxlen);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int ret;
	unsigned int status_flags;
	size_t ahead, behind;
	git_buf repo_buf = {};
	git_repository *repo = NULL;
	git_reference *head = NULL;
	git_strarray remote;
	git_oid upstream;
	const git_oid *local;
	char remoteName[PATH_LENGTH];
	const char *strval = NULL;

	git_libgit2_init();

	if (git_repository_discover(&repo_buf, FileName, 1, NULL) != 0)
	{
		git_libgit2_shutdown();
		return ft_fieldempty;
	}

	if (git_repository_open(&repo, repo_buf.ptr) != 0)
	{
		git_buf_free(&repo_buf);
		git_libgit2_shutdown();
		return ft_fieldempty;
	}

	git_buf_free(&repo_buf);

	switch (FieldIndex)
	{
	case 0:
		if (git_repository_is_bare(repo))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 1:
		if (git_repository_is_empty(repo))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 2:
		if (git_repository_is_worktree(repo))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 3:
		if (git_repository_is_shallow(repo))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 4:
		if (git_repository_head_unborn(repo))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 5:
		if (git_repository_head_detached(repo))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 6:
		if (git_repository_head(&head, repo) == 0)
		{
			strval = git_reference_shorthand(head);
			git_reference_free(head);
		}

		if (!strval)
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}
		else
			strlcpy((char*)FieldValue, strval, maxlen - 1);

		break;

	case 7:
		if (git_repository_head(&head, repo) == 0)
		{
			git_remote_list(&remote, repo);
			git_reference_free(head);
		}

		if (!remote.strings[0])
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}
		else
			strlcpy((char*)FieldValue, remote.strings[0], maxlen - 1);

		break;

	case 8:
		if (git_repository_head(&head, repo) == 0)
		{
			strval = git_reference_shorthand(head);
			git_remote_list(&remote, repo);

			if (remote.count)
			{
				local = git_reference_target(head);
				strcpy(remoteName, "refs/remotes/");

				if (remote.strings[0])
					strcat(remoteName, remote.strings[0]);

				strcat(remoteName, "/");
				strcat(remoteName, strval);
				ret = git_reference_name_to_id(&upstream, repo, remoteName);

				if (local && !ret)
					ret = git_graph_ahead_behind(&ahead, &behind, repo, local, &upstream);
			}

			git_reference_free(head);
		}

		if (ret != 0)
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}
		else
			*(int*)FieldValue = ahead;


		break;

	case 9:
		if (git_repository_head(&head, repo) == 0)
		{
			strval = git_reference_shorthand(head);
			git_remote_list(&remote, repo);

			if (remote.count)
			{
				local = git_reference_target(head);
				strcpy(remoteName, "refs/remotes/");

				if (remote.strings[0])
					strcat(remoteName, remote.strings[0]);

				strcat(remoteName, "/");
				strcat(remoteName, strval);
				ret = git_reference_name_to_id(&upstream, repo, remoteName);

				if (local && !ret)
					ret = git_graph_ahead_behind(&ahead, &behind, repo, local, &upstream);
			}

			git_reference_free(head);
		}

		if (ret != 0)
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}
		else
			*(int*)FieldValue = behind;

		break;

	case 10:
		strval = git_repository_workdir(repo);

		if (git_status_file(&status_flags, repo, FileName + strlen(strval)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_NEW)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}

		break;

	case 11:
		strval = git_repository_workdir(repo);

		if (git_status_file(&status_flags, repo, FileName + strlen(strval)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_MODIFIED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}

		break;

	case 12:
		strval = git_repository_workdir(repo);

		if (git_status_file(&status_flags, repo, FileName + strlen(strval)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_DELETED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}

		break;

	case 13:
		strval = git_repository_workdir(repo);

		if (git_status_file(&status_flags, repo, FileName + strlen(strval)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_TYPECHANGE)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}

		break;

	case 14:
		strval = git_repository_workdir(repo);

		if (git_status_file(&status_flags, repo, FileName + strlen(strval)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_RENAMED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}

		break;

	case 15:
		strval = git_repository_workdir(repo);

		if (git_status_file(&status_flags, repo, FileName + strlen(strval)) == 0)
		{
			if (status_flags & GIT_STATUS_WT_UNREADABLE)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}

		break;

	case 16:
		strval = git_repository_workdir(repo);

		if (git_status_file(&status_flags, repo, FileName + strlen(strval)) == 0)
		{
			if (status_flags & GIT_STATUS_IGNORED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}

		break;

	case 17:
		strval = git_repository_workdir(repo);

		if (git_status_file(&status_flags, repo, FileName + strlen(strval)) == 0)
		{
			if (status_flags & GIT_STATUS_CONFLICTED)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
		}
		else
		{
			git_repository_free(repo);
			git_libgit2_shutdown();
			return ft_fieldempty;
		}

		break;

	default:
		git_repository_free(repo);
		git_libgit2_shutdown();
		return ft_nosuchfield;
	}

	git_repository_free(repo);
	git_libgit2_shutdown();
	return fields[FieldIndex].type;
}
