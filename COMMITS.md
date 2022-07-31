# Conventional Commits

This project will use [Conventional Commits v1.0.0](https://www.conventionalcommits.org/en/v1.0.0/).

## Message subject

|Type|Usage|Version|Header|
|----|----|----|----|
|`feat`|new feature for the user, not a new feature for build script|MINOR|Features|
|`fix`|bug fix for the user, not a fix to a build script|PATCH|Bugfixes|
|`assets`|only artwork: icons, images, but not css (those belong to `style`)|||
|`build`|changes that affect the build system (conan, cmake, later npm) or external dependencies|||
|`chore`|updating grunt tasks etc; no production code change|||
|`ci`|changes to CI configuration files and scripts (GitHub actions)|||
|`docs`|changes to the documentation|||
|`perf`|performance improvement for the user|||
|`refactor`|refactoring production code, eg. renaming a variable|||
|`revert`|neither fixing a bug nor adding a feature|||
|`style`|formatting, missing semi colons, etc; no production code change|||
|`test`|adding missing tests, refactoring tests; no production code change|||

|Scope|Usage|
|-|-|
|`lang`|either .lang or .po files; until I have proper `lang` exec on CI, also files, which should be generated (.cc, .hh and .pot)|
|`api`|`git2-c++` or `cov-api` libraries|
|`lighter`|any of the `hilite` libraries|
|`app`|`app` library|
|`rt`|`cov-rt` library|
|`core`|`cov` and builtins|
|`cmake`, `conan`, `yarn`|for `build`|
|`server`|changes to beast code, but not routes|
|`ws` or `rest`|changes to C++ half of communication|
|`webapp`|changes to React half of communication|

Additionally, after v1.0.0 release, any breaking change will have a `!` appended to type/scope and `BREAKING CHANGE: <description>` in the message footer.

## Message body
- uses the imperative, present tense: “change” not “changed” nor “changes”
- includes motivation for the change and contrasts with previous behavior

## Message footer

### Referencing issues

Closed issues should be listed on a separate line in the footer prefixed with "Closes" header like this:

```
Closes: #234
```

or in the case of multiple issues:

```
Closes: #123, #245, #992
```

Referenced, but not closed issues should use "Refs" footer header:

```
Refs: #123, #245, #992
```

### Breaking changes

All breaking changes have to be mentioned in footer with the description of the change, justification and migration notes. This should be the last part of a commit message.

```
BREAKING CHANGE:

`port-runner` command line option has changed to `runner-port`, so that it is
consistent with the configuration file syntax.

To migrate your project, change all the commands, where you use `--port-runner`
to `--runner-port`.
```

## Further references
- https://gist.github.com/joshbuchea/6f47e86d2510bce28f8e7f42ae84c716
- https://github.com/angular/angular/blob/22b96b9/CONTRIBUTING.md#commit
- http://karma-runner.github.io/1.0/dev/git-commit-msg.html
- https://365git.tumblr.com/post/3308646748/writing-git-commit-messages
