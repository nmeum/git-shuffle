# git-shuffle

Randomize timestamps associated with Git commits to enhance privacy.

## Motivation

Git associates timestamps with commits. These timestamps expose coding
hours and thereby potentially violate ones privacy. This tool randomizes
the hour of the day, as contained in these timestamps, to enhance
privacy. The tool can be employed automatically for all Git repository
through global `githooks(5)` (see below).

## Status

More or less feature complete but not well tested yet.

## Installation

This software only requires [libgit2][libgit2 website]. If libgit2 was
installed successfully, compile this software as follows:

	make

Afterwards, the software can be installed system-wide using:

	make install

## Usage

This tool can be invoked manually from a Git repository. For example,
the following command would randomize timestamps of all unpushed
commits on the `master` branch:

	$ git shuffle origin/master

However, it is likely desirable to automate this process through global
`githooks(5)`. For this purpose `core.hooksPath` will need to be set
using `git-config(1)`. Additionally, a `post-commit` must be created
which amends the previously created commit. For example using:

	$ git config --global core.hooksPath ~/.config/git/hooks
	$ mkdir -p ~/.config/git/hooks
	$ printf "#!/bin/sh\ngit shuffle -a\n" > ~/.config/git/hooks/post-commit
	$ chmod +x ~/.config/git/hooks/post-commit

## Related Work

The [git-privacy][git-privacy repo] utility shares the same goals but
has way more configuration options and is thus more complicated.
Furthermore, it doesn't utilize [libgit2][libgit2 website].

## License

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.

[libgit2 website]: https://libgit2.org/
[git-privacy repo]: https://github.com/EMPRI-DEVOPS/git-privacy
