# Using cov in a project

There are three phases to 

```plain
cov collect
cov report
cov show / export / serve
```

## Steps

1. Prepare `collect` config
   1. Decide, which parts of project needs reporting
   1. Inform `cov collect`, which compiler is used and where to find performance counters
   1. Automate with CMake or another build system
1. Prepare binaries to be observed
1. Clear any old performance counters
1. Observe inspected behavior
1. Collect data from performance counters
1. Report collected data
1. Present results

## Prepare collect config

### Decide, which parts of project needs reporting

`collect.include`, `collect.exclude`

`clang.exec`

### Inform `cov collect`, which compiler is used and where to find performance counters

`collect.compiler`

`collect.src-dir`, `collect.bin-dir`

### Automate with CMake

## Prepare binaries to be observed

### GCC

### Clang

### MSVC

## Clear any old performance counters
```plain
cov collect --clear
```

## Observe inspected behavior

```plain
cov collect --observe <tested-program>
```

```plain
cov collect --observe ctest --preset debug
```

## Collect data from performance counters

```plain
cov collect
```

## Report collected data

```plain
cov report -f strip-excludes -p...
```

## Present results

```plain
cov show
```

```plain
cov show HEAD:src/package
```

```plain
cov show HEAD:src/package/entry.cc
```

```plain
cov export --html
```
