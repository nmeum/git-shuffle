#include <err.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "git2.h"

static bool verbose = false;
static git_repository *repo;

static void
usage(char *prog)
{
	fprintf(stderr, "USAGE: %s [-v]\n", basename(prog));
	exit(EXIT_FAILURE);
}

/* TODO: Make this more advanced */
static void
randtime(git_time *dest, git_commit *commit)
{
	dest->time = 0;

	dest->offset = git_commit_time_offset(commit);
	dest->sign = (dest->offset < 0) ? '-' : '+';
}

static void
pcommit(git_commit *commit)
{
	char buf[GIT_OID_HEXSZ + 1];

	git_oid_tostr(buf, sizeof(buf), git_commit_id(commit));
	printf("Updating time of commit %s\n", buf);
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
		if (verbose)
			pcommit(commit);

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
	git_reference *lref, *uref;
	const char *branch;
	git_annotated_commit *upstream;

	/* TODO: Add more sanity checks here */
	if (git_repository_head(&lref, repo))
		errx(EXIT_FAILURE, "git_repository_head failed");
	branch = git_reference_shorthand(lref);
	if (git_branch_lookup(&lref, repo, branch, GIT_BRANCH_LOCAL))
		errx(EXIT_FAILURE, "git_branch_lookup failed");
	if (git_branch_upstream(&uref, lref))
		errx(EXIT_FAILURE, "git_branch_upstream failed");

	if (git_annotated_commit_from_ref(&upstream, repo, uref))
		errx(EXIT_FAILURE, "git_annotated_commit_from_revspec failed");

	/* TODO: abort rebase on error */
	if (git_rebase_init(&rebase, repo, NULL, upstream, NULL, NULL))
		errx(EXIT_FAILURE, "git_rebase_init failed");

	shuftimes(rebase);
	if (git_rebase_finish(rebase, NULL))
		errx(EXIT_FAILURE, "git_revwalk_finish");
}

int
main(int argc, char **argv)
{
	int opt;
	static char cwd[PATH_MAX + 1];
	static git_buf rfp;

	if (!getcwd(cwd, sizeof(cwd)))
		err(EXIT_FAILURE, "getcwd failed");

	while ((opt = getopt(argc, argv, "v")) != -1) {
		switch (opt) {
		case 'v':
			verbose = true;
			break;
		default:
			usage(argv[0]);
		}
	}

	git_libgit2_init();
	if (git_repository_discover(&rfp, cwd, 0, NULL))
		errx(EXIT_FAILURE, "git_repository_discover failed");
	if (git_repository_open(&repo, rfp.ptr))
		errx(EXIT_FAILURE, "git_repository_open failed");

	rebase();
}
