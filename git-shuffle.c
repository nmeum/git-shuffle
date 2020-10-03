#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "git2.h"

static git_repository *repo;

/* TODO: Make this more advanced */
static void
randtime(git_time *dest, git_commit *commit)
{
	dest->time = 0;

	dest->offset = git_commit_time_offset(commit);
	dest->sign = (dest->offset < 0) ? '-' : '+';
}

static void
shuftimes(git_rebase *rebase)
{
	git_oid oid;
	git_rebase_operation *op;

	while (!git_rebase_next(&op, rebase)) {
		git_commit *commit;
		git_signature *newauth;
		const git_signature *origauth;

		if (git_commit_lookup(&commit, repo, &op->id))
			err(EXIT_FAILURE, "git_commit_lookup failed");

		/* TODO: Optional verbose output */
		/* printf("commit: %s\n", git_commit_message(commit)); */

		origauth = git_commit_author(commit);
		if (git_signature_dup(&newauth, origauth))
			err(EXIT_FAILURE, "git_signature_dup failed");
		randtime(&newauth->when, commit);

		if (git_rebase_commit(&oid, rebase, newauth, newauth, NULL, NULL))
			errx(EXIT_FAILURE, "git_rebase_commit failed");

		git_commit_free(commit);
		git_signature_free(newauth);
	}
}

static void
rebase(void)
{
	git_rebase *rebase;
	git_annotated_commit *upstream;

	/* TODO: abort rebase on error */

	/* TODO: Don't hardcode origin/HEAD */
	if (git_annotated_commit_from_revspec(&upstream, repo, "origin/HEAD"))
		errx(EXIT_FAILURE, "git_annotated_commit_from_revspec failed");
	if (git_rebase_init(&rebase, repo, NULL, upstream, NULL, NULL))
		errx(EXIT_FAILURE, "git_rebase_init failed");

	shuftimes(rebase);
	if (git_rebase_finish(rebase, NULL))
		errx(EXIT_FAILURE, "git_revwalk_finish");
}

int
main(void)
{
	static char cwd[PATH_MAX + 1];
	static git_buf rfp;

	if (!getcwd(cwd, sizeof(cwd)))
		err(EXIT_FAILURE, "getcwd failed");

	/* TODO: Parse command-line arguments */

	git_libgit2_init();
	if (git_repository_discover(&rfp, cwd, 0, NULL))
		errx(EXIT_FAILURE, "git_repository_discover failed");
	if (git_repository_open(&repo, rfp.ptr))
		errx(EXIT_FAILURE, "git_repository_open failed");

	rebase();
}
