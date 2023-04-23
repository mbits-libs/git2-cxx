# Cov

Coverage visualization system living in your git repository.

## Revision names

Names in Cov are largely simplified, compared to Git. They can be:

- `<sha1>`, long or short
- `<refname>`
- `<rev>^0`, `<rev>^1` or `<rev>^`, meaning either the report itself, or it's parent.
- `<rev>~[<n>]`, where `<rev>~3` means `<rev>^^^`.

If a command allows a range, either `..<rev>`, `<rev>..` or `<rev>..<rev>` (but not `..`) is allowed. In case of missing edge, `HEAD` is used.

## Commands

- **Setup and Config**: config, module, ~~help~~ \
  `cov config [-h] [<file-options>]`
  - `<name> [<value>]`
  - `--add <name> <value>`
  - `--get <name>`
  - `--get-all <name>`
  - `--unset <name>`
  - `--unset-all <name>`
  - `-l | --list`

  The **cov config** is basically **git config**, but working on cov-specific config files.

  `cov module [-h]`
  - `[<git-commit>]`
  - `--add <name> <dir>`
  - `--remove <name> <dir>`
  - `--remove-all <name>`

  Modules are used by **git show** to organize source files into groups. Calling **cov module** without any argument will show the modules configured in current Git work directory; calling it with name of a commit will show the modules associated with this reference. **Add** adds a new directory to a named module, **remove** removes a directory (and if this was the last one in a module, module is also removed); finally, **remove-all** removes all directories of a module, with the module itself.

- **Creating a Repository**: init \
  `cov init [-h] [--git-dir <dir>] [--force] [directory]`

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

- **Basic Snapshotting**: report, remove _(soon)_, ~~reset~~ \
  `cov report [-h] <report-file> [-f <filter>] [--amend]`

  This command adds a new report to the report list. It it like **git add** and **git commit** rolled into one and just like **commit**, it normally adds the report on top of exiting history, unless there is an **--amend** parameter. In this case, it tries to replace current tip of the history with updated report.

  The _report file_ format is a JSON described by the [report-schema.json](apps/report-schema.json), but it can be filtered from other formats by **-f \<filter\>** argument. Currently, the **cov report** has filters for Cobertura and Coveralls.

- **Branches and Tags**: branch, tag, checkout _(soon)_ \
  `cov branch [-h] [--color <when>]`
  - `[--force] <name>`
  - `--list [<pattern>...]`
  - `--delete <name>`
  - `--show-current`

  `cov tag [-h]`
  - `[--force] <name>`
  - `--list [<pattern>...]`
  - `--delete <name>`

  Again, **cov branch** and **cov tag** are simplified versions of **git branch** and **git tag**, respectively, working on Cov structure.

- **Inspection and Comparison**: log, show  _(soon)_, serve  _(soon)_\
  `cov log [-h] [<options>] [<revision-range>|<revision>]`

  Lists all reports reachable from **\<revision>**. Alternatively, lists all reports reachable from right side to **\<revision-range>**, but not reachable from left side.
