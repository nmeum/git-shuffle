# git shuffle

Randomize dates of commits before pushing.

## Installation

This software only requires [libgit2][libgit2 website]. If libgit2 was
installed successfully, compile this software as follows:

	make

## Usage

On invocation this software performs a rebase operation beginning from
the latest commit on the upstream branch for the current branch. The
time information associated with the committer and author of affected
commits is modified. The hour encoded in the time information is
randomized. Future versions of this software may allow configuring this
behaviour further, e.g. to also randomize commit dates.  Randomizing
time information allows keeping coding hours private. 

The tool can be employed system-wide through global `githooks(5)`. In
order to do so, `core.hooksPath` will need to be set in `git-config(1)`.
Additionally, a `pre-push` hook which invokes `git-shuffle` must be
created. For example using:

	$ git config --local core.hooksPath ~/.config/git/hooks
	$ mkdir -p ~/.config/git/hooks
	$ printf "#!/bin/sh\ngit shuffle\n" > ~/.config/git/hooks/pre-push
	$ chmod +x ~/.config/git/hooks/pre-push

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

[libgit2]: https://libgit2.org/
