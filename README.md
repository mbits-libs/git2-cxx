# Cov

Coverage visualization system living in your git repository.

## Commands

- **Setup and Config**: config _(soon)_, ~~help~~
  - `cov config`\
    `[--global|--local|-f <file>]`\
    `(--list|name) [value]`
- **Creating a Repository**: init
  - `cov init [-h] [--git-dir <dir>] [--force] [directory]`

    Calling **cov init** without any arguments will lookup current Git common directory and attempt to initialize Cov repo inside the `.git/.covdata`. Adding either **--git-dir \<directory>** or **directory** will help locating the Git common directory, unless they are _both_ added and the **directory** is not child of Git's working directory. In that case, the Cov repo is still pointing to that Git, but it is created outside of it. E.g. each of:
    ```sh
    cov -C git/project/subdir init
    cov init --git-dir git/project/subdir
    cov init git/project/subdir
    cov init --git-dir git/project/subdir git/project
    ```
    will create Cov repository inside `git/project/.git/.covdata`, while
    ```sh
    cov init --git-dir git/project/subdir coverage/project
    ```
    will create Cov repository inside `coverage/project`, pointing back to `git/project/.git`.
- **Basic Snapshotting** _(soon)_: report, remove, ~~reset~~
- **Branches and Tags** _(soon)_: branch, checkout, log, tag
- **Inspection and Comparison** _(soon)_: show, log, serve
