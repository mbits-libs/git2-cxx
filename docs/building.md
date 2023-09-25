# Working with code

## Prerequisites

To configure the project:
- [Python 3.8](https://www.python.org/) to run all the tools and scripts, including `./flow`.
- [Conan 1.60](https://conan.io/) to get all the dependencies.
- [CMake 3.25](https://cmake.org/) to configure the build and to pack the binaries.

To build the project:
- On Windows, Visual Studio 2022.
- On Linux, GCC 12 and [Ninja 1.10](https://ninja-build.org/).

## Flow

Easiest way to get the binaries is to use flows. They are used as part of GitHub workflow (and they even inform the GitHub, what actions are there to take).

```sh
./flow <command> <config-to-build>
```

### Commands
Currently, there are few specialized flows: **config**, **build**, **report** and **verify**, plus one general one, named simply **run**. Each performs a predefined set of steps, with `./flow run` being able to specify, which one to actually run.

|Step|Meaning|`run`|`config`|`build`|`report`|`verify`|
|-|-|:-:|:-:|:-:|:-:|:-:|
|**Conan**|Recreates `./build/conan` and puts Conan package references there.|✓|✓||||
|**CMake**|Recreates `./build/${build_type}` and runs the `cmake` there.|✓|✓||||
|**Build**|Builds the project.|✓||✓|✓|✓|
|**Test**|Either directly calls `ctest`, or builds the self-coverage gathering target.|✓|||✓|✓|
|**Report**|Puts the latest coverage into local instance of **cov**.||||✓|✓|
|**Sign**|On Windows, signs installable binaries|✓||||✓|
|**Pack**|Creates archives and installers|✓||||✓|
|**SignPackages**|On Windows, signs the MSI package|✓||||✓|
|**Store**|Copies packages from **Pack** to `./build/artifacts`.|✓||||✓|
|**BinInst**|Extracts the ZIP/TAR.GZ to `./build/.local/`. Adding `./build/.local/bin` to $PATH should help with running latest build from the get-go.|||||✓|
|**DevInst**|Extracts the ZIP/TAR.GZ to `./build/.user/`.|||||✓|

### Config

Config is taken from `"matrix"` dictionary in the `.github/workflows/flow.json`. Currently, it is:

```json
{
  "compiler": ["gcc", "clang", "msvc"],
  "build_type": ["Release", "Debug"],
  "sanitizer": [true, false],
  "os": ["ubuntu", "windows"]
}
```

and calling any flow command without specified config will try to perform the widest possible set of actions. The only exception is, that in a normal `./flow` run, **not** setting `"os"` will assume the "right" value, adding either `-c os=ubuntu`, or `-c os=windows`.

Since this means _not_ setting a config key would possibly double the number of resulting configurations, there are couple shortcuts provided. Each one introduces a list of configs, with `<comp>` being `gcc` on Linux, `msvc` on Windows, or whatever `DEV_CXX` environment variable contains:

|Shortcut|Expansion|
|--------|---------|
|`--dev`|`-c` `compiler=<comp>` `build_type=Debug` `sanitizer=OFF`|
|`--rel`|`-c` `compiler=<comp>` `build_type=Release` `sanitizer=OFF`|
|`--both`|`-c` `compiler=<comp>` `sanitizer=OFF`|
|`--sane`|`-c` `compiler=<comp>` `build_type=Debug` `sanitizer=ON`|

To run `verify` over `./build/release` directory:
```sh
./flow verify --rel
```

To use Clang in Debug mode:
```sh
DEV_CXX=clang ./flow verify --dev
```

## Building the code

### Configuration

```sh
./flow config --rel
```

### Compilation

```sh
./flow build --rel
```

### Packing archives/installers

Running either of below commands should leave the artifacts in `./build/artifacts` directory.
```sh
./flow verify --rel
./flow run --rel -s pack,store
```

### Installing

There is no `./flow` step, which would install the code, but CMake's `--install` should also do the trick:

```sh
sudo cmake --install ./build/release [--prefix <install-dir>]
```
