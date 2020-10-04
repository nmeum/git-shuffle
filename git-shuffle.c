#include <err.h>
#include <libgen.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "git2.h"

static bool verbose = false;
static git_repository *repo;

static void
usage(char *prog)
{
	char *usage = "[-v] REVSPEC";

	fprintf(stderr, "USAGE: %s %s\n", basename(prog), usage);
	exit(EXIT_FAILURE);
}

static void
randtime(git_time *dest, git_commit *commit)
{
	int offset;
	struct tm *tm;
	git_time_t ctime;
	time_t t;

	ctime = git_commit_time(commit);
	offset = git_commit_time_offset(commit);

	/* https://github.com/libgit2/libgit2/blob/79d0e0c10ffec81152b5b1eaeb47b59adf1d4bcc/examples/log.c#L321 */
	t = (time_t)ctime + (offset * 60);

	if (!(tm = gmtime(&t)))
		errx(EXIT_FAILURE, "gmtime failed");
	tm->tm_hour = rand() % 24; /* random hour on current date */

	if ((dest->time = mktime(tm)) == (time_t)-1)
		err(EXIT_FAILURE, "mktime failed");
	dest->offset = 0; /* XXX: How do timezones work?! */
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
rebase(const char *revspec)
{
	git_rebase *rebase;
	git_annotated_commit *upstream;

	if (git_annotated_commit_from_revspec(&upstream, repo, revspec))
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
	const char *revspec;
	static char cwd[PATH_MAX + 1];
	static git_buf rfp;

	/* Seed PRNG with current time for now */
	srand(time(NULL));

	if (!getcwd(cwd, sizeof(cwd)))
		err(EXIT_FAILURE, "getcwd failed");

	while ((opt = getopt(argc, argv, "v")) != -1) {
		switch (opt) {
		case 'v':
			verbose = true;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if (argc <= 1 || optind >= argc)
		usage(argv[0]);
	revspec = argv[optind];

	git_libgit2_init();
	if (git_repository_discover(&rfp, cwd, 0, NULL))
		errx(EXIT_FAILURE, "git_repository_discover failed");
	if (git_repository_open(&repo, rfp.ptr))
		errx(EXIT_FAILURE, "git_repository_open failed");

	rebase(revspec);
}
