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

static bool amendsingle = true;
static bool verbose = false;
static git_repository *repo;

static void
giterr(const char *fn)
{
	const git_error *err;

	err = git_error_last();
	errx(EXIT_FAILURE, "%s failed: %s", fn, err->message);
}

static void
usage(char *prog)
{
	char *usage = "[-a] [-v] REVSPEC";

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

static git_signature *
redate(git_commit *commit)
{
	char buf[GIT_OID_HEXSZ + 1];
	git_signature *newauth;
	const git_signature *origauth;

	if (verbose) {
		git_oid_tostr(buf, sizeof(buf), git_commit_id(commit));
		printf("Updating time of commit %s\n", buf);
	}

	origauth = git_commit_author(commit);
	if (git_signature_dup(&newauth, origauth))
		err(EXIT_FAILURE, "git_signature_dup failed");
	randtime(&newauth->when, commit);

	return newauth;
}

static void
shuftimes(git_rebase *rebase)
{
	git_oid oid;
	git_rebase_operation *op;

	while (!git_rebase_next(&op, rebase)) {
		git_commit *commit;
		git_signature *author;

		if (git_commit_lookup(&commit, repo, &op->id))
			err(EXIT_FAILURE, "git_commit_lookup failed");
		author = redate(commit);
		if (git_rebase_commit(&oid, rebase, author, author, NULL, NULL))
			giterr("git_rebase_commit");

		git_commit_free(commit);
		git_signature_free(author);
	}
}

static void
rebase(const char *revspec)
{
	git_rebase *rebase;
	git_annotated_commit *upstream;

	if (git_annotated_commit_from_revspec(&upstream, repo, revspec))
		giterr("git_annotated_commit_from_revspec");

	/* TODO: abort rebase on error */
	if (git_rebase_init(&rebase, repo, NULL, upstream, NULL, NULL))
		giterr("git_rebase_init");

	shuftimes(rebase);
	if (git_rebase_finish(rebase, NULL))
		giterr("git_rebase_finish");
}

static void
amend(const char *revspec)
{
	git_object *obj;
	const git_oid *oid;
	git_commit *commit;
	git_signature *author;

	if (git_revparse_single(&obj, repo, revspec))
		giterr("git_revparse_single");
	oid = git_object_id(obj);

	if (git_commit_lookup(&commit, repo, oid))
		giterr("git_commit_lookup");
	printf("commid: %s\n", git_commit_message(commit));

	author = redate(commit);
	if (git_commit_amend(oid, commit, "HEAD", author, author, NULL, NULL, NULL))
		giterr("git_commit_amend");
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

	while ((opt = getopt(argc, argv, "av")) != -1) {
		switch (opt) {
		case 'a':
			amendsingle = true;
			break;
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
		giterr("git_repository_discover");
	if (git_repository_open(&repo, rfp.ptr))
		giterr("git_repository_open");

	if (amendsingle)
		amend(revspec);
	else
		rebase(revspec);
}
