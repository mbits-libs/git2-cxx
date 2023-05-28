# Changelog

All notable changes to this project will be documented in this file. See [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) and [COMMITS.md](COMMITS.md) for commit guidelines.

## [0.15.1](https://github.com/mzdun/cov/compare/v0.15.0...v0.15.1) (2023-05-28)

### Bug Fixes

- build with g++-12 and clang-15 ([437081c](https://github.com/mzdun/cov/commit/437081c454e6c6b4de652189371d9534fb114e6a))
- clang-format arguing ([fcc30ec](https://github.com/mzdun/cov/commit/fcc30ecb3afe0ae46d6f968a3abc363f974dd75c))
- remove percived ub (gcc) ([6ec4f44](https://github.com/mzdun/cov/commit/6ec4f443043451f671a73bdf65471078fe046231))
- remove leaks and an ub (clang) ([e78fa5f](https://github.com/mzdun/cov/commit/e78fa5fdd08d40c76fc3be1c3d2c740783f445b0))
- compile with clang ([c831cd4](https://github.com/mzdun/cov/commit/c831cd4f81df4b418136aad80a67a45272bea50c))

## [0.15.0](https://github.com/mzdun/cov/compare/v0.14.0...v0.15.0) (2023-04-30)

### New Features

- allow TTY-check from outside the formatting ([f40f536](https://github.com/mzdun/cov/commit/f40f536e588e903f10031279a0a7e16a544b38b0))
- **api**: expose common code for branching ([2039d38](https://github.com/mzdun/cov/commit/2039d38db123953a1db3d17e2ea875828b7da10b))
- **api**: allow removing references ([55083b4](https://github.com/mzdun/cov/commit/55083b442f5b2235ed1e2a7b9bd7682b212d853a))
- **api**: add error codes specific to cov-api ([9082d91](https://github.com/mzdun/cov/commit/9082d918838f5abdc152346ca086f92b3394c066))
- **app**: allow help args be both opt an multi ([b53da41](https://github.com/mzdun/cov/commit/b53da411c79d8428cb4869f6fe548aa84b0ab614))
- **core**: provide checkout builtin commands ([863ddaa](https://github.com/mzdun/cov/commit/863ddaa4b80b06314cb6a9890210c636fe3bd3a1))
- **core**: provide branch/tag builtin commands ([f55ba7a](https://github.com/mzdun/cov/commit/f55ba7a73434be76ec78dde10749bc3dd08dc3c0))
- **lang**: add strings for checkout ([b352c4d](https://github.com/mzdun/cov/commit/b352c4d5b661c9aabd5fb00329603a0f659cb734))
- **lang**: add strings for branch and tag ([b2f54db](https://github.com/mzdun/cov/commit/b2f54db8cc50b3bc24b8b3d0e8a9612ea5a6d153))
- **rt**: support branch start point ([d192fa5](https://github.com/mzdun/cov/commit/d192fa577377b916d1986456212f916674634b04))
- **rt**: add code behind `cov checkout` ([ce07c33](https://github.com/mzdun/cov/commit/ce07c338162f395e1a9815a990d412bcb701c368))
- **rt**: adapt to API changes ([6bfc2d8](https://github.com/mzdun/cov/commit/6bfc2d828cd4f5d793fba058a3bd15d62ac90efe))
- **rt**: backend for tag and branch commands ([407481a](https://github.com/mzdun/cov/commit/407481a6ed260fca7b709194715730ec40c6a8b0))

### Bug Fixes

- patch CVE-2007-4559 (#25) ([da48ace](https://github.com/mzdun/cov/commit/da48ace0744040b8479a42f910e2a80b8dd1bdba))
- **api**: appease linter (cont'd) ([47316f2](https://github.com/mzdun/cov/commit/47316f2fe577746aad57c9cc18315db5390677c8))
- **api**: appease linter ([5bc6421](https://github.com/mzdun/cov/commit/5bc6421bfd2a4fd74ddce397b2678240b1251adb))
- **api**: react to safe stream failure on rename ([7cd8ddb](https://github.com/mzdun/cov/commit/7cd8ddbfb81a9954ba13948e25f6b061dcbc68a3))
- **api**: remove crash in remove_ref ([a7f65bf](https://github.com/mzdun/cov/commit/a7f65bf1fd9fee945fb1460c8a90fd369a12fa0c))
- **core**: adapt to start point API ([c768457](https://github.com/mzdun/cov/commit/c7684579ff6ca73482bb1d8ccb54d750921f9af8))

## [0.14.0](https://github.com/mzdun/cov/compare/v0.13.1...v0.14.0) (2022-08-15)

### New Features

- **core**: add `log` to builtin commands ([cfe1fb1](https://github.com/mzdun/cov/commit/cfe1fb115b8ec950caaf08342fa38332adfe5da4))
- **rt**: show specified range, predefine formats ([a701db7](https://github.com/mzdun/cov/commit/a701db7f5a9f5973638a777c13144b91f6d67759))
- **lang**: add strings for cov-log ([6854d39](https://github.com/mzdun/cov/commit/6854d3955ceb412b39866b5434d4e9ede370f05c))
- **lang**: use lngs strings for formatter ([cf34c58](https://github.com/mzdun/cov/commit/cf34c5835758dfa23c551f692792615f424fc3f7))
- **api**: support generalized tristate feature ([d77dd1e](https://github.com/mzdun/cov/commit/d77dd1e5a0d0e498e436bf24967a9fe0d824cfac))
- **api**: parse rev pairs ([87ca7c8](https://github.com/mzdun/cov/commit/87ca7c848dc537a295c67aab6b5764cdb3b15aed))
- **api**: create format context in unified way ([1706e5a](https://github.com/mzdun/cov/commit/1706e5a21fc2582757610fdd336ef5f66f1f6084))
- **api**: add partial lookup to loose backend ([362c5e7](https://github.com/mzdun/cov/commit/362c5e739c9b7cfa99bd2e101f12d6c6b9dcf5af))
- **api**: clean up colors, add file list hashes ([1a6874c](https://github.com/mzdun/cov/commit/1a6874ce8957636c223077bc313c42d813ec6093))

### Bug Fixes

- clean up linter findings ([1191da3](https://github.com/mzdun/cov/commit/1191da384a50cecfffa6a19187d7fe47e57a62a1))
- enhance error reporting for cov-log and cov-show ([3ca1584](https://github.com/mzdun/cov/commit/3ca1584b46de4b70c9025f11fbfdc6eec3d51356))
- **api**: don't load tzdb if not needed ([24e5b1e](https://github.com/mzdun/cov/commit/24e5b1eec7290a732c013dcfc199fcc4a87b703b))
- **rt**: extract opening cov repo to function ([9570b7a](https://github.com/mzdun/cov/commit/9570b7a3d265a88a0fc8066b900b369363779465))
- **rt, core**: detach rt paths from tools ([7f2eb76](https://github.com/mzdun/cov/commit/7f2eb76c4114b40a4a5ee34cf1b0435e5d1f58ab))

## [0.13.1](https://github.com/mzdun/cov/compare/v0.13.0-beta...v0.13.1) (2022-08-08)

### Bug Fixes

- describe `module` and `report` ([90e29d8](https://github.com/mzdun/cov/commit/90e29d8ae59692247550cfa35a8eb90016199122))
- **api**: switch from `sth_create` to `sth::create` ([2900978](https://github.com/mzdun/cov/commit/29009784df44bee982e9dd6020e67be121813d42))

## [0.13.0-beta](https://github.com/mzdun/cov/compare/v0.12.4-beta...v0.13.0-beta) (2022-08-08)

### New Features

- **core**: add `report` to builtin commands ([49d4391](https://github.com/mzdun/cov/commit/49d4391b306bb8ca5959663157bf81c2ee60bfdd))
- **api**: read and/or update current HEAD ([8e109a3](https://github.com/mzdun/cov/commit/8e109a3bf71fcbc39b53fc377ea3daae572a24f2))
- **lang**: add strings for cov-report ([f429b99](https://github.com/mzdun/cov/commit/f429b9923cba374067bd9a427e43d5fa52e2b7e9))
- **app**: remove copies of make_path from apps ([576f4a5](https://github.com/mzdun/cov/commit/576f4a51ec5580578526348a87af0f8dc177634d))
- **api**: create null ref after dangling one ([1c3faf4](https://github.com/mzdun/cov/commit/1c3faf4ebaa0dcde57e9c988d2ea9aeefc656ac4))
- **posix**: add filter runner ([482d505](https://github.com/mzdun/cov/commit/482d50595caf2977edfa2a6011a131a019beb006))
- **win32**: add filter runner ([d924ae0](https://github.com/mzdun/cov/commit/d924ae01c39e431082f2373a093735956c46d694))
- **rt**: expand line-to-hits map to v1 coverage ([eacf940](https://github.com/mzdun/cov/commit/eacf940495eda2827d24f29cc31a2993b67a0e92))
- **core**: add filters for cobertura and coveralls ([da12beb](https://github.com/mzdun/cov/commit/da12beb49c5edbbf2b8c05787fc45b972c418041))
- **rt**: load additional info from Git repo ([f4df01c](https://github.com/mzdun/cov/commit/f4df01cdac97cac9c2fd672d92c58272d79e6baf))
- **rt**: read reports from JSON ([bfbb92d](https://github.com/mzdun/cov/commit/bfbb92dc5e9d681e6a06584e845cbc8a19cd1d88))
- **rt**: setup code for reports ([5b8060e](https://github.com/mzdun/cov/commit/5b8060e3b023c431f4d63c6b0866c645e1d06d3d))
- **api**: switch to `report_files_builder` ([c6f6d3e](https://github.com/mzdun/cov/commit/c6f6d3e9b72f7b04bc3c0beeb486c3c425531cfc))

### Bug Fixes

- **core**: simplify text output from filter ([0ddd14a](https://github.com/mzdun/cov/commit/0ddd14aed7778d12510af32c2d749f007dc1f665))
- **rt**: report total lines in blobs/files ([5b2d0a1](https://github.com/mzdun/cov/commit/5b2d0a1ea2185273a538d624f1b7e5b3da467435))
- **api**: simplify report entry ([864c596](https://github.com/mzdun/cov/commit/864c5967c1665c91c9d858fd0b5b17fa16e28c19))
- **cmake**: switch winrc to new PO layout ([b6f1d01](https://github.com/mzdun/cov/commit/b6f1d01c96b58a94a9dfa8687a45dd5ac24bc56e))

## [0.12.4-beta](https://github.com/mzdun/cov/compare/v0.12.3-beta...v0.12.4-beta) (2022-08-04)

### Bug Fixes

- change schema id to expected tag name ([57a6069](https://github.com/mzdun/cov/commit/57a6069a9a1252beb76d9f9c2dd82c576d3fd630))
- create JSON schema for a report ([a145021](https://github.com/mzdun/cov/commit/a1450216ed5fbd69b8a1c3ba70bbcae98d68d835))

## [0.12.3-beta](https://github.com/mzdun/cov/compare/v0.12.2-beta...v0.12.3-beta) (2022-07-31)

### Build System

- automatically remove docker containers ([47bdd3a](https://github.com/mzdun/cov/commit/47bdd3af3c796a7a1698e091656207dd95e32e8a))
- force unix line endings in split files ([72fe514](https://github.com/mzdun/cov/commit/72fe5143a52c7694d869e89cfb118e9ca2e32210))
- remove first version of helpers ([595de5c](https://github.com/mzdun/cov/commit/595de5ca6474d569fc88dde7ee62a3b2611514e3))
- home-brewed make scripts for new language structure ([b75104b](https://github.com/mzdun/cov/commit/b75104bc662a110a8ad6639a75ea7f53e4bc1a6f))
- **lang**: switch to combined POT/PO files ([7ae0d19](https://github.com/mzdun/cov/commit/7ae0d1987f60eb85c455a44edcad7a38ae7e537a))
- **lang**: remove per-lngs gettext files ([4f9be6e](https://github.com/mzdun/cov/commit/4f9be6e1486768d113af826b54156a0799dd6a18))

### Code Style

- **spell-check**: add polib names to dictionary ([603555b](https://github.com/mzdun/cov/commit/603555bf5c06919a95e47345bea4c2dd861f2b10))

## [0.12.2-beta](https://github.com/mzdun/cov/compare/v0.12.1-beta...v0.12.2-beta) (2022-07-31)

### Bug Fixes

- update the 0.12.1 to flat layout ([79883ac](https://github.com/mzdun/cov/commit/79883aceb4e86905b66a832d7e457df2c2727d1b))
- `docs` should be fully treated as a `fix` ([14ace6a](https://github.com/mzdun/cov/commit/14ace6ac439ce90c60a516198fa60eb804902ec2))
- update commit footer description ([803400d](https://github.com/mzdun/cov/commit/803400d4a15b6dd9a4f3ca0e847ac7dfb82069d2))
- hide non-fix/non-feat from changelog ([4134039](https://github.com/mzdun/cov/commit/413403906c1e1118c8f1be5c960f43b10ea8baac))

## [0.12.1-beta](https://github.com/mzdun/cov/compare/v0.12.0-beta...v0.12.1-beta) (2022-07-30)

### Bug Fixes

- **lang**: add win32 RC description string ([85f938a](https://github.com/mzdun/cov/commit/85f938ac03064d04cef7cbcb54599a1f54894843))
- update roadmap, add conventional commits ([dbfe43b](https://github.com/mzdun/cov/commit/dbfe43b528069d7059d5173e17d8da73f1c89bb2))

## [0.12.0-beta](https://github.com/mzdun/cov/compare/v0.11.1-beta...v0.12.0-beta) (2022-07-29)

### New Features

#### Application Library

- add cov module code ([872fdcb](https://github.com/mzdun/cov/commit/872fdcbe0451d3e671cc64fba765395bde496164))

#### API

- add modules to cov-api ([a80d769](https://github.com/mzdun/cov/commit/a80d769b8e1ca6b5684827e0581a6141d7190cd8))

#### Language Strings

- add strings for cov module ([c47e875](https://github.com/mzdun/cov/commit/c47e875fc6714ef76c0fa5c7f991534a6b407778))

## [0.11.1-beta](https://github.com/mzdun/cov/compare/v0.11.0-beta...v0.11.1-beta) (2022-07-23)

### Bug Fixes

#### Language Strings

- fix string issues in cov config ([de84f34](https://github.com/mzdun/cov/commit/de84f3461368f6f45e1d172d089c0383bfb409eb))

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

## [0.6.0-alpha](https://github.com/mzdun/cov/compare/v0.5.0-alpha...v0.6.0-alpha) (2022-07-08)

### New Features

#### Lighter

- add newer features to C++ ([79ec6bc](https://github.com/mzdun/cov/commit/79ec6bcc603ab36e2d2dedb69bffa665e17a9c53))
- add hilite library (with C++ highlighter) ([9237b59](https://github.com/mzdun/cov/commit/9237b5944664b7041c58b9ee38302f3775048874))

## [0.5.0-alpha](https://github.com/mzdun/cov/compare/v0.4.1-alpha...v0.5.0-alpha) (2022-07-05)

### New Features

#### API

- find a way to get list of renamed files ([61152fc](https://github.com/mzdun/cov/commit/61152fc6ec5db10fbd19daf839c8171c8f58bb15))

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

## [0.3.0-alpha](https://github.com/mzdun/cov/compare/v0.2.0..v0.3.0-alpha) (2022-07-01)

### New Features

#### API

- add tags and "branches" ([36109a1](https://github.com/mzdun/cov/commit/36109a1c35e0d5cf3e5e68d896c8b1b4be565525))
- add reference API ([43d8fc6](https://github.com/mzdun/cov/commit/43d8fc666fc587f9c80c33b249f6f69e6143f1d7))
