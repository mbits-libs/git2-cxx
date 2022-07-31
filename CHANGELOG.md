# Changelog

All notable changes to this project will be documented in this file. See [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) and [COMMITS.md](COMMITS.md) for commit guidelines.

## [0.12.1-beta](https://github.com/mzdun/cov/compare/v0.12.0-beta...v0.12.1-beta) (2022-07-30)

### Bug Fixes

#### Language Strings

- add win32 RC description string ([85f938a](https://github.com/mzdun/cov/commit/85f938ac03064d04cef7cbcb54599a1f54894843))

### Other Changes

- install polib ([c8e40ea](https://github.com/mzdun/cov/commit/c8e40eabdb537d3396cb922ca8a882adfc394c00))
- prepare for releases ([e594c57](https://github.com/mzdun/cov/commit/e594c570e45b0a4325d34c9ab8311d0fe8ca5665))
- update roadmap, add conventional commits ([dbfe43b](https://github.com/mzdun/cov/commit/dbfe43b528069d7059d5173e17d8da73f1c89bb2))

#### Application

- add shield icons ([cbb8334](https://github.com/mzdun/cov/commit/cbb833490991e4a748fa8492810eca0c4ba474f7))

#### Language Strings

- no more version updates in .PO[T] files ([5f9e77e](https://github.com/mzdun/cov/commit/5f9e77e5bff2362df68bb399c5ae9be0d215a198))

#### CMake

- add win32 resources to binary ([da0db4a](https://github.com/mzdun/cov/commit/da0db4a214b830ecf2b3290cd2712a60a663ac3b))

## [0.12.0-beta](https://github.com/mzdun/cov/compare/v0.11.1-beta...v0.12.0-beta) (2022-07-29)

### New Features

#### Application Library

- add cov module code ([872fdcb](https://github.com/mzdun/cov/commit/872fdcbe0451d3e671cc64fba765395bde496164))

#### API

- add modules to cov-api ([a80d769](https://github.com/mzdun/cov/commit/a80d769b8e1ca6b5684827e0581a6141d7190cd8))

#### Language Strings

- add strings for cov module ([c47e875](https://github.com/mzdun/cov/commit/c47e875fc6714ef76c0fa5c7f991534a6b407778))

### Other Changes

- apply fixes for clang-fromat findings ([40be4a8](https://github.com/mzdun/cov/commit/40be4a81f1f6212980b99d159d2bb15ab65a9f03))
- set private $TMP for each test run ([c3b0f76](https://github.com/mzdun/cov/commit/c3b0f7668635858ef81c0ed0724349f759fe1f72))
- make sure OOM tests will succeed in short TMP directories ([203acd4](https://github.com/mzdun/cov/commit/203acd4aef994af93f132a21ec7269cb3465f42e))

#### Application Library

- move args composition from cov RT to app lib ([787dc0c](https://github.com/mzdun/cov/commit/787dc0cb0dbe0ff16e4a3403d3b240006f8790e5))

#### Conan

- update libgit2 ref, add config creation from memory ([1f16ac0](https://github.com/mzdun/cov/commit/1f16ac047937d7bbb7a32a840484a9f476508809))

#### github

- fix linter issues, try adding clang-format to the mix ([73175a6](https://github.com/mzdun/cov/commit/73175a61b93f8f23b16795540b40ec3df08e706f))
- allow local runs with LOG_LEVEL=VERBOSE ([8f7de37](https://github.com/mzdun/cov/commit/8f7de37e57d1c159e45f3cb3dcafceefb2f096e8))
- add python linting ([89e0620](https://github.com/mzdun/cov/commit/89e06205f1a26df04f0a9120ea3205b7ce07ad43))

## [0.11.1-beta](https://github.com/mzdun/cov/compare/v0.11.0-beta...v0.11.1-beta) (2022-07-23)

### Bug Fixes

#### Language Strings

- fix string issues in cov config ([de84f34](https://github.com/mzdun/cov/commit/de84f3461368f6f45e1d172d089c0383bfb409eb))

### Other Changes

#### CMake

- flatten the data directory structure ([0fec3e8](https://github.com/mzdun/cov/commit/0fec3e88aa876b01bec408334a15089c059bb53f))

#### Linter

- specify, why NOLINT is used ([a98388a](https://github.com/mzdun/cov/commit/a98388a242f7a013a1316a665db87018686c5497))
- move run_linter to tools ([83ea900](https://github.com/mzdun/cov/commit/83ea900c6cef5f0533efcad8abc8e714af3dc8df))
- add Super Linter (slim) (#15) ([7903516](https://github.com/mzdun/cov/commit/79035166211b1dd9b1321cbb3d1e47b04372d7c9))

## [0.11.0-beta](https://github.com/mzdun/cov/compare/v0.10.0-beta...v0.11.0-beta) (2022-07-19)

### New Features

#### Application Library

- add cov config code ([b7cbe23](https://github.com/mzdun/cov/commit/b7cbe23e7be46bb313565d54044335a684fa7499))

#### Language Strings

- add strings for cov config ([bce1915](https://github.com/mzdun/cov/commit/bce1915d163b05f9120bf9e59e337eae2bcf214f))

## [0.10.0-beta](https://github.com/mzdun/cov/compare/v0.9.0-beta...v0.10.0-beta) (2022-07-19)

### New Features

#### Application

- add code for cov init ([a445cae](https://github.com/mzdun/cov/commit/a445caefd24e5c26bc4a773d5469cbb9ca549303))

#### Language Strings

- adding strings for cov init ([e0981e2](https://github.com/mzdun/cov/commit/e0981e2e93356f1c88de075baf8ec6a6601c2137))

### Bug Fixes

#### Build System

- fix runner output patching on paths w/short names ([d80695a](https://github.com/mzdun/cov/commit/d80695a79b3f59cb3c353021e01a45877f8b7c71))
- fix install/pack issue with moved dirs.hh.in ([97aedc1](https://github.com/mzdun/cov/commit/97aedc1bd3089ee92ed7e8a5b8d498ed9af5c394))

### Other Changes

#### Application Library

- move parts of rt to app for externals to access ([4afefa2](https://github.com/mzdun/cov/commit/4afefa2c76ca91f778ec8e7606d30b27c51c8cf7))

#### Testing

- update test driver for cov init tests ([4d73c4d](https://github.com/mzdun/cov/commit/4d73c4d72a975bfb921d6ab1cf80271f670ba67e))

## [0.9.0-beta](https://github.com/mzdun/cov/compare/v0.8.0-beta...v0.9.0-beta) (2022-07-17)

### New Features

#### Language Strings

- add strings for cov itself ([b88a155](https://github.com/mzdun/cov/commit/b88a155e3c5c2f21156d18de7e10d8d94df1e1f5))
- add strings for args::base_translator ([d5b8335](https://github.com/mzdun/cov/commit/d5b8335f40d8fb3ceac09c09834ecfb756f6801e))

#### Application

- support UTF-8/UCS-2 conversion ([f12443c](https://github.com/mzdun/cov/commit/f12443c2861170ca70d48424edf05b627ec4c8ed))

### Bug Fixes

#### App Runtime

- fix error in parser::noent ([c32cd90](https://github.com/mzdun/cov/commit/c32cd90ad1929c1c627a28b4a1e0b86f10cad215))

### Other Changes

#### CMake

- add library-only lngs dependency ([b4a053b](https://github.com/mzdun/cov/commit/b4a053b9300ae921a955890606c03b8d2efbe126))

## [0.8.0-beta](https://github.com/mzdun/cov/compare/v0.7.2-alpha...v0.8.0-beta) (2022-07-30)

### New Features

- add tool runner (alias, builtin, external) ([b87984f](https://github.com/mzdun/cov/commit/b87984faa4a7fd15bcc57e167091f6180b6515d8))

## [0.7.2-alpha](https://github.com/mzdun/cov/compare/v0.6.0-alpha...v0.7.2-alpha) (2022-07-30)

### New Features

#### API

- add author/committer/message access to git::commit ([177b110](https://github.com/mzdun/cov/commit/177b1107a09a3b3f506d01882ee2152435e8d0c2))
- add file stats diffs ([c2bbafc](https://github.com/mzdun/cov/commit/c2bbafca28d21f508d3781ba300083cc455bc542))
- add reports, objects and blobs to repository ([7d0df3d](https://github.com/mzdun/cov/commit/7d0df3dcc2a296ef6a877bd734857e8bb0f0636a))
- embrace std::error_code more ([4323855](https://github.com/mzdun/cov/commit/43238556367a6366ec39c326d8a683915e092672))
- add DWIM and refs to repository ([5f3c9a3](https://github.com/mzdun/cov/commit/5f3c9a3997d005e8023dd3e2f15516d4cfe0c872))
- enhance config file loading ([e2b64d0](https://github.com/mzdun/cov/commit/e2b64d0e1182a5da462476ed032ffe4f21d846bd))

### Other Changes

#### API

- fix tests on Windows ([b5f71fa](https://github.com/mzdun/cov/commit/b5f71fa4ae51d2a355bf383dc30e8efbc1ff2cb0))
- add test coverage on Windows ([a939a23](https://github.com/mzdun/cov/commit/a939a232af910d384e02650e156ff93c7caa3a4e))

#### Lighter

- add missing c++ lighter tests ([9a3546d](https://github.com/mzdun/cov/commit/9a3546d7ed104cc889f9d9065e3b521863176e89))

#### CMake

- reorganize to grant `libcov` access to `lighter` ([c557fdd](https://github.com/mzdun/cov/commit/c557fdd6d3ce5b84468bd22d239d007a8bd6d9c9))

## [0.6.0-alpha](https://github.com/mzdun/cov/compare/v0.5.0-alpha...v0.6.0-alpha) (2022-07-08)

### New Features

#### Lighter

- add newer features to C++ ([79ec6bc](https://github.com/mzdun/cov/commit/79ec6bcc603ab36e2d2dedb69bffa665e17a9c53))
- add hilite library (with C++ highlighter) ([9237b59](https://github.com/mzdun/cov/commit/9237b5944664b7041c58b9ee38302f3775048874))

### Other Changes

#### CMake

- trying to get pack to hilite-cxx ([2f8a010](https://github.com/mzdun/cov/commit/2f8a01093adafabb089089a39bc7fe7c5f61c37a))
- making hilites' target name more CPack-friendly ([d55efbe](https://github.com/mzdun/cov/commit/d55efbe8fd1bdfd01224b678dcd1f3b24b70958b))

## [0.5.0-alpha](https://github.com/mzdun/cov/compare/v0.4.1-alpha...v0.5.0-alpha) (2022-07-05)

### New Features

#### API

- find a way to get list of renamed files ([61152fc](https://github.com/mzdun/cov/commit/61152fc6ec5db10fbd19daf839c8171c8f58bb15))

### Other Changes

#### API

- make diff tests independent from this repo ([ab11bd1](https://github.com/mzdun/cov/commit/ab11bd11c7b45d97d2fe70c19e5fa760b765a843))
- no need for duplicating <new>'s job ([e138877](https://github.com/mzdun/cov/commit/e1388773c31ac8fdbc0d81b1a9213c9b6afa7505))
- stress-testing allocator uses ([58937b8](https://github.com/mzdun/cov/commit/58937b8700db95b0f358e4db2179f1e2f23ee2b7))
- stress-testing allocator uses ([e790323](https://github.com/mzdun/cov/commit/e790323a82cbe1c3b285a6ec38c3c1f5e3fe0cf2))

## [0.4.1-alpha](https://github.com/mzdun/cov/compare/v0.4.0-alpha...v0.4.1-alpha) (2022-07-04)

### Other Changes

- adjust conan cache recovery ([0b267d2](https://github.com/mzdun/cov/commit/0b267d20bc4895d1edcd477f00cb2ef1f5f46259))
- print current conanfile hash ([4720c8f](https://github.com/mzdun/cov/commit/4720c8f85abe899590148a365ba4ea704c870076))

## [0.4.0-alpha](https://github.com/mzdun/cov/compare/v0.3.0-alpha...v0.4.0-alpha) (2022-07-04)

### New Features

#### API

- support %w(...) ([1748098](https://github.com/mzdun/cov/commit/174809847abd48fe762fb2a0ca7de96539461822))
- support %C(...) ([113a4e0](https://github.com/mzdun/cov/commit/113a4e01b75a39e7d8204234b41d922426f64a17))
- add subset of pretty-formats ([22d85ed](https://github.com/mzdun/cov/commit/22d85edbdaa428581c6d90a29870604d1d614671))

### Other Changes

- remove some tests when on GitHub ([cd69c8f](https://github.com/mzdun/cov/commit/cd69c8f0fbb8736f0179535f45dc1cd464b4303a))

#### API

- switch from git_time_t to sys_seconds ([419f9ca](https://github.com/mzdun/cov/commit/419f9caeaad7c7c192bed0d93877dd6fdd051d23))

## [0.3.0-alpha](https://github.com/mzdun/cov/compare/v0.2.0..v0.3.0-alpha) (2022-07-01)

### New Features

#### API

- add tags and "branches" ([36109a1](https://github.com/mzdun/cov/commit/36109a1c35e0d5cf3e5e68d896c8b1b4be565525))
- add reference API ([43d8fc6](https://github.com/mzdun/cov/commit/43d8fc666fc587f9c80c33b249f6f69e6143f1d7))

### Other Changes

#### API

- free ref for references, add ref_ptr ([be526d9](https://github.com/mzdun/cov/commit/be526d9d2f89083025b05aa431b9f837867d95e7))
