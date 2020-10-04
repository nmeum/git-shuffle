#include <err.h>
#include <fcntl.h>
#include <libgen.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <git2.h>

static bool amend_single = false;
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
	const char *usage = "[-a] [-v] revspec";

	fprintf(stderr, "USAGE: %s %s\n", basename(prog), usage);
	exit(EXIT_FAILURE);
}

static void
initrand(void)
{
	int fd;
	unsigned int seed;

	if ((fd = open("/dev/random", O_RDONLY)) == -1)
		err(EXIT_FAILURE, "open failed");
	if (read(fd, &seed, sizeof(seed)) != sizeof(seed))
		errx(EXIT_FAILURE, "read from /dev/random failed");

	close(fd);
	srand(seed);
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
rebase(const char *revspec)
{
	git_oid oid;
	git_rebase *rebase;
	git_annotated_commit *upstream;
	git_rebase_operation *op;

	if (git_annotated_commit_from_revspec(&upstream, repo, revspec))
		giterr("git_annotated_commit_from_revspec");
	if (git_rebase_init(&rebase, repo, NULL, upstream, NULL, NULL))
		giterr("git_rebase_init");

	while (!git_rebase_next(&op, rebase)) {
		git_commit *commit;
		git_signature *author;

		if (git_commit_lookup(&commit, repo, &op->id))
			goto err;
		author = redate(commit);
		if (git_rebase_commit(&oid, rebase, author, author, NULL, NULL))
			goto err;

		git_commit_free(commit);
		git_signature_free(author);
	}

	if (git_rebase_finish(rebase, NULL))
		giterr("git_rebase_finish");
	return;

err:
	warnx("rebase failed: %s", git_error_last());
	if (git_rebase_abort(rebase))
		giterr("git_rebase_abort");
}

static void
amend(void)
{
	git_oid oidcpy;
	git_commit *commit;
	const git_oid *oid;
	git_reference *head;
	const char *refname;
	git_signature *author;
	git_annotated_commit *annotated;

	if (git_repository_head(&head, repo))
		giterr("git_repository_head");
	if (git_annotated_commit_from_ref(&annotated, repo, head))
		giterr("git_annotated_commit_from_ref");

	oid = git_annotated_commit_id(annotated);
	if (git_oid_cpy(&oidcpy, oid))
		giterr("git_oid_cpy");

	if (git_commit_lookup(&commit, repo, oid))
		giterr("git_commit_lookup");
	refname = git_reference_name(head);

	author = redate(commit);
	if (git_commit_amend(&oidcpy, commit, refname, author, author, NULL, NULL, NULL))
		giterr("git_commit_amend");
}

int
main(int argc, char **argv)
{
	int opt;
	static char cwd[PATH_MAX + 1];
	static git_buf rfp;

	initrand();
	if (!getcwd(cwd, sizeof(cwd)))
		err(EXIT_FAILURE, "getcwd failed");

	while ((opt = getopt(argc, argv, "av")) != -1) {
		switch (opt) {
		case 'a':
			amend_single = true;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	git_libgit2_init();
	if (git_repository_discover(&rfp, cwd, 0, NULL))
		giterr("git_repository_discover");
	if (git_repository_open(&repo, rfp.ptr))
		giterr("git_repository_open");

	if (amend_single) {
		amend();
	} else {
		if (argc <= 1 || optind >= argc)
			usage(argv[0]);
		rebase(argv[optind]);
	}
}
