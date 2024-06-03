# Changelog

All notable changes to this project will be documented in this file. See [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) and [COMMITS.md](COMMITS.md) for commit guidelines.

## [0.25.0](https://github.com/mzdun/cov/compare/v0.24.1...v0.25.0) (2024-06-03)

### New Features

- use libarch to unpack the iana db ([35533fc](https://github.com/mzdun/cov/commit/35533fcee7da7cef03b05562d6d202f887bf3680))

### Bug Fixes

- files need directories to exist ([90b7038](https://github.com/mzdun/cov/commit/90b7038c127ae29959657a2b4a7e87e88f721dc0))
- download data-tz to TMP ([b8347c6](https://github.com/mzdun/cov/commit/b8347c6e42c9c3afaf9bd3916520f1cecdbf6ba5))
- **docs**: update roadmap ([8cc4b2d](https://github.com/mzdun/cov/commit/8cc4b2de5a1f40b5b9de64272de91f80feb25fd5))

## [0.24.1](https://github.com/mzdun/cov/compare/v0.24.0...v0.24.1) (2024-04-17)

### Bug Fixes

- do not upload on RELEASE ([6ad91a2](https://github.com/mzdun/cov/commit/6ad91a29a976662689bba988c5997e73fe70f97d))

## [0.24.0](https://github.com/mzdun/cov/compare/v0.23.0...v0.24.0) (2024-04-17)

### New Features

- add error descriptions to `cov-report` ([c49c608](https://github.com/mzdun/cov/commit/c49c608e70338dd41509b2229f74d9ab00f54b6b))
- clean error output ([bb1af96](https://github.com/mzdun/cov/commit/bb1af96e5892059f18225492c0c9cae85e521710)), closes [#92](https://github.com/mzdun/cov/issues/92)
- prepare cov-collect configs ([d2a5119](https://github.com/mzdun/cov/commit/d2a5119913e89602660446aab01e4ab9ddb29b2a))

## [0.23.0](https://github.com/mzdun/cov/compare/v0.22.0...v0.23.0) (2024-04-16)

### New Features

- add strip-excludes filter ([fc17d19](https://github.com/mzdun/cov/commit/fc17d19cab5a1a2359ce75c2d3f14b9ac6e244f1)), closes [#73](https://github.com/mzdun/cov/issues/73)
- support filter arguments in cov-report ([764b231](https://github.com/mzdun/cov/commit/764b23186670d7ceeae54e3093c327ca6a50c314))

### Bug Fixes

- do not patch schema versions in tests ([e8d0b6e](https://github.com/mzdun/cov/commit/e8d0b6e4e8d6aaa98cb02ea5bb632def2326bd96))
- iron out kinks in Schemas used in the project ([a682db4](https://github.com/mzdun/cov/commit/a682db4d9a1e5c6e2e6543464086a99265922fb0))
- bring clang back ([d3c9255](https://github.com/mzdun/cov/commit/d3c9255b0741a30836dcd6511de8d178ab70ebf0)), closes [#76](https://github.com/mzdun/cov/issues/76)

## [0.22.0](https://github.com/mzdun/cov/compare/v0.21.3...v0.22.0) (2024-04-11)

### New Features

- allow opening Cov repos from worktrees ([ae04133](https://github.com/mzdun/cov/commit/ae041335b951880443a01d350673ec670774ea1e)), closes [#81](https://github.com/mzdun/cov/issues/81)

### Bug Fixes

- compilation warning on GCC ([aa695da](https://github.com/mzdun/cov/commit/aa695da8558bf6039832e6e426e9199f8645d717))
- read_line ([4969867](https://github.com/mzdun/cov/commit/4969867553321c9125bcb5676c8d19c98751fd2a))

## [0.21.3](https://github.com/mzdun/cov/compare/v0.21.2...v0.21.3) (2024-04-09)

### Bug Fixes

- discover cov from inside worktree ([2279586](https://github.com/mzdun/cov/commit/2279586910385ef9c3d7a2703ab54446cf83159c)), closes [#78](https://github.com/mzdun/cov/issues/78)

## [0.21.2](https://github.com/mzdun/cov/compare/v0.21.1...v0.21.2) (2024-04-07)

### patch

- code signing on Windows (#77) ([50e5c49](https://github.com/mzdun/cov/commit/50e5c49260968b753a9d87ca686808170cecfca2)), references [#71](https://github.com/mzdun/cov/issues/71)

## [0.21.1](https://github.com/mzdun/cov/compare/v0.21.0...v0.21.1) (2023-07-24)

### Bug Fixes

- **api**: use COV_PATH for tools ([649c449](https://github.com/mzdun/cov/commit/649c44963c45693f4c9e6b5c19062a0506fe8661)), closes [#47](https://github.com/mzdun/cov/issues/47)
- **docs**: update roadmap, add next steps list ([04a6d23](https://github.com/mzdun/cov/commit/04a6d23bada0d2a4363615bd34f2ec5964f7158a))

## [0.21.0](https://github.com/mzdun/cov/compare/v0.20.0...v0.21.0) (2023-07-23)

### New Features

- fully alias co-exiting functions ([8e27957](https://github.com/mzdun/cov/commit/8e279572342e160fddd51df303c50a4776c78222)), closes [#55](https://github.com/mzdun/cov/issues/55)
- **api**: rename `.covmodule` to `.covmodules` ([55d3a3c](https://github.com/mzdun/cov/commit/55d3a3cd7fb76a2259fd2548ffa52bc6cf82a095)), closes [#54](https://github.com/mzdun/cov/issues/54)
- **api**: prepare log formatter for various objects ([57ba02b](https://github.com/mzdun/cov/commit/57ba02bb3c164c7e25d110d1a5ee26086f83cecb))
- **api**: take or borrow naked pointers ([e6ea747](https://github.com/mzdun/cov/commit/e6ea7471c35e3a24aa89e5a876bc8961a437b95b))
- **api**: use git::oid/oid_view ([4e9cb92](https://github.com/mzdun/cov/commit/4e9cb92b4b4aedda839b2e12db4b12f8fa6127a0))
- **api**: loosen revparse restrictions ([85ec4f2](https://github.com/mzdun/cov/commit/85ec4f21bd25f377d90bd8d62da13354d74c4726)), references [#51](https://github.com/mzdun/cov/issues/51)
- **core**: allow only reports in `cov log` ([7957fc8](https://github.com/mzdun/cov/commit/7957fc841cf43f5bb32222872e2dbc95bcb90195)), closes [#51](https://github.com/mzdun/cov/issues/51), references [#51](https://github.com/mzdun/cov/issues/51)
- **rt**: `show` as many kinds of info as possible ([53f5cfe](https://github.com/mzdun/cov/commit/53f5cfe73841d8ccac2c86ab5be4c251abb0d02d)), references [#51](https://github.com/mzdun/cov/issues/51)

### Bug Fixes

- apply changes required/uncovered by tests ([6e00711](https://github.com/mzdun/cov/commit/6e007115239e0d91737bff290339d49bfd30e6b6))
- **api**: allow zero id to be valid in revparse ([24391e9](https://github.com/mzdun/cov/commit/24391e9fb64e5be136d5929e250ca511d52981b1)), references [#51](https://github.com/mzdun/cov/issues/51)
- **api**: properly mark uncalled functions ([585c783](https://github.com/mzdun/cov/commit/585c783535f2a2f5f23dfb7eec657192152fbf87))
- **rt**: adapt tests to new logger ([95c0126](https://github.com/mzdun/cov/commit/95c0126c86876dee8808e8f4d340c8ad4ab5ad95))

## [0.20.0](https://github.com/mzdun/cov/compare/v0.19.1...v0.20.0) (2023-07-21)

### New Features

- **api**: split rating colors into kinds ([75efad8](https://github.com/mzdun/cov/commit/75efad895446a8b2d8687e37458de77a5e810f30)), references [#48](https://github.com/mzdun/cov/issues/48)
- **rt**: add new columns to `cov show` table ([61d5a39](https://github.com/mzdun/cov/commit/61d5a3996fbc432fc3949b20e9bbf14968df288b)), references [#48](https://github.com/mzdun/cov/issues/48)
- **rt**: show more ratings in `cov report`/`reset` ([7c7af1a](https://github.com/mzdun/cov/commit/7c7af1aa361ea24b47bd93d344afee847864acf0)), references [#48](https://github.com/mzdun/cov/issues/48)
- **runtime, core**: show functions in file view ([31851f2](https://github.com/mzdun/cov/commit/31851f27d807b1f1bc5d2f8a6f34a8b757d3ecf3)), references [#48](https://github.com/mzdun/cov/issues/48)

### Bug Fixes

- apply linter comments ([112409b](https://github.com/mzdun/cov/commit/112409b249ddbce31624e1aac7a433d36ccf947c))
- **api, rt**: cleanup table code ([b553362](https://github.com/mzdun/cov/commit/b553362572d0f027c0631c23ac1ccb32a555c1fe))
- **rt**: recalculate coverage for function aliases ([e484dad](https://github.com/mzdun/cov/commit/e484dadc62ee4da7c5a1669a343ef42b6d9cb9f0)), closes [#48](https://github.com/mzdun/cov/issues/48)
- **rt**: calc chunks using function counts ([5ee14de](https://github.com/mzdun/cov/commit/5ee14de23b0f74de48a1c9f1c1e02f7a5a2702d6)), references [#48](https://github.com/mzdun/cov/issues/48)
- **runtime**: expect lines to be 1-based, but keep them 0-based ([6126026](https://github.com/mzdun/cov/commit/61260265c473a7da0a0a6859fe522067ab927c91))

## [0.19.1](https://github.com/mzdun/cov/compare/v0.19.0...v0.19.1) (2023-07-19)

### Bug Fixes

- **ci**: copy test results on non-release builds ([c66e1a4](https://github.com/mzdun/cov/commit/c66e1a45ddf44cdb409593ae7698b127d3e11a68))
- **ci**: mark the release as latest ([8db978b](https://github.com/mzdun/cov/commit/8db978bcbab5661ccbeb2bae8cccf90a6c28bc7e))

## [0.19.0](https://github.com/mzdun/cov/compare/v0.18.0...v0.19.0) (2023-07-19)

### New Features

- **api**: remodel the log formatter ([615b812](https://github.com/mzdun/cov/commit/615b8128d333c62625c230a8f003f173ee30e7e0)), references [#46](https://github.com/mzdun/cov/issues/46)
- **core**: update apps and tests ([2448ed4](https://github.com/mzdun/cov/commit/2448ed45ef1c6d1baced2e3d0d186ef1974a57d0)), closes [#46](https://github.com/mzdun/cov/issues/46)
- **rt**: adapt to new logging ([918792e](https://github.com/mzdun/cov/commit/918792e5b7340b70eec45c6eec0d17f175ba5f75)), references [#46](https://github.com/mzdun/cov/issues/46)

### Bug Fixes

- reorganize the documentation files ([fb8795d](https://github.com/mzdun/cov/commit/fb8795d42b17ac29fdd60a60f65e03d4da36df73))
- **api**: remove build issue hidden on vs 19.37 ([5a5582d](https://github.com/mzdun/cov/commit/5a5582d006c5a3927937a825609817d0fd3b1692))
- **api**: changes resulting from new tests ([fd64b2b](https://github.com/mzdun/cov/commit/fd64b2bee2815331c781101e9893efae1a5b7fbc))

## [0.18.0](https://github.com/mzdun/cov/compare/v0.17.3...v0.18.0) (2023-07-16)

### New Features

- show properties in non-root views ([5ced119](https://github.com/mzdun/cov/commit/5ced1196252ce3f92a5bfbfed71c522647899398))
- prepare properties answer file with cmake ([67699bb](https://github.com/mzdun/cov/commit/67699bb55518511ec5afc3e6d646208ec83fbd8c))
- **api**: merge two lowest-level libraries ([17cb7fe](https://github.com/mzdun/cov/commit/17cb7fe953360c1499d050ba15180c8e46ad9484))
- **api**: shake up the data structures ([0d51e15](https://github.com/mzdun/cov/commit/0d51e1513bfc010c5e4ae0b7c8c3676c17ca32c1))
- **api**: adding functions and branches to stats ([3540b3e](https://github.com/mzdun/cov/commit/3540b3eed53cf61be50c1d98c50d0931467a1ab1))
- **api**: prep for multiple total/visited pairs ([4014755](https://github.com/mzdun/cov/commit/40147559bb2927e6d75f7838089ce735faeccefc))
- **api**: store additional OIDs in report files ([94edd09](https://github.com/mzdun/cov/commit/94edd099de2ffe89505a8f97ee506c6f9a6b842b))
- **api**: expose additional OIDs from report_entry ([a9aed8f](https://github.com/mzdun/cov/commit/a9aed8f17b62d4e78d2fd40844e5ed2dbf3e1c97))
- **api**: select preferred version for io handler ([dc6e9c4](https://github.com/mzdun/cov/commit/dc6e9c40a772f8b2b94f825ff2a429a194c840a8))
- **api, core**: normalize build properties ([61a97d3](https://github.com/mzdun/cov/commit/61a97d3672f0c1db0871575f51b660b2d9d96ca8))
- **core**: show builds as part of `report` ([2f0fff7](https://github.com/mzdun/cov/commit/2f0fff7cc7b198c28773a28dadc593b33bc9b90b))

### Bug Fixes

- update build scripts ([c26fb5e](https://github.com/mzdun/cov/commit/c26fb5e8312bcdcaf01200b1b42b7296a239847c))
- apply further linter comments ([861091c](https://github.com/mzdun/cov/commit/861091c2ef5ef607416cd52dacd829a456af5580))
- remove UB issue from files loader ([481c8ce](https://github.com/mzdun/cov/commit/481c8cef99114a57706e0dfbdf56531bc1d90af9)), closes [#43](https://github.com/mzdun/cov/issues/43)
- apply further linter comments ([7624e9c](https://github.com/mzdun/cov/commit/7624e9c7f65619a17f21630bc9fb45ca9b48f8db))
- apply linter comments ([50d9528](https://github.com/mzdun/cov/commit/50d95288057b9b502ec0670abc06a006992b3a9b))
- **api**: add missing files stats ([0deb2de](https://github.com/mzdun/cov/commit/0deb2dece2caec7e8c4baf5f08230cef2bd35f24))

### BREAKING CHANGES

Data structures between 0.17.3 and 0.18.0 change to the point of being unusable. Store your local aliases from `.git/.covdata/setup`, remove the `.git/.covdata` and re-init the cov repo with newest version of tools (via `cov init`).

## [0.17.3](https://github.com/mzdun/cov/compare/v0.17.2...v0.17.3) (2023-07-10)

### Continuos Integration

- **github**: clean workflow ([3448083](https://github.com/mzdun/cov/commit/344808315693efd15482ba111f85d7ac961dfbb1))
- **github**: fix asset upload ([94e631f](https://github.com/mzdun/cov/commit/94e631fd106f0355a5538d193f3e15036207697e))
- **github**: switch from `curl` to `gh api` ([2fabad3](https://github.com/mzdun/cov/commit/2fabad3d1bb036054eb31be7c80250918b29b0a4))
- **github**: hook upload to github actions ([3880a42](https://github.com/mzdun/cov/commit/3880a42ee454069083c3f6bc8772490ee163e2c0))
- **github**: do not force -test on main branch ([b7c7015](https://github.com/mzdun/cov/commit/b7c7015eb2fa72c165249f11ece27bba1c20b4e6))
- **github**: upload artifacts to release ([9ec6d60](https://github.com/mzdun/cov/commit/9ec6d604ed21725b10acaaa5bdd0e46b7a0b1e46))
- **github**: clean up the release script ([eefc2ff](https://github.com/mzdun/cov/commit/eefc2ff4981681ebfa4d27e60fc53bee7cd65c8f))

## [0.17.2](https://github.com/mzdun/cov/compare/v0.17.1...v0.17.2) (2023-07-08)

### Bug Fixes

- create WiX installer ([6bfe24d](https://github.com/mzdun/cov/commit/6bfe24d4a5f9e46832e47e9346cdf0db77231684))

## [0.17.1](https://github.com/mzdun/cov/compare/v0.17.0...v0.17.1) (2023-07-06)

### Bug Fixes

- show summary for HEAD..HEAD ([fa981b2](https://github.com/mzdun/cov/commit/fa981b2a8b0dcd0a3f0f2fba6fe1275fad5dba08))

## [0.17.0](https://github.com/mzdun/cov/compare/v0.16.0...v0.17.0) (2023-07-06)

### New Features

- **api**: locate file contents by path ([5850d90](https://github.com/mzdun/cov/commit/5850d901866ab7afae5b0c007df0c3f4d4093df7))
- **api**: calculate module and/or directory projection ([2e12983](https://github.com/mzdun/cov/commit/2e129833e3403452c49247fd46327a9b7dfec25c))
- **api**: take workdir modules if commit is also HEAD ([c837521](https://github.com/mzdun/cov/commit/c837521013ec6e68b27ff99e6bd4cd2ee28f15ae))
- **api**: expose apply_mark to wider audience ([10729b6](https://github.com/mzdun/cov/commit/10729b6d8d2d4d00d0a25619152cba617d79194f))
- **api**: report error code on bad cast ([aea68b6](https://github.com/mzdun/cov/commit/aea68b6138e589a70c7b215acd71435ac4544c48))
- **api**: format tabular data ([fc3a87d](https://github.com/mzdun/cov/commit/fc3a87d8eb9fb98e3011ab265e7fa2f816a7c75a))
- **core**: added translations to cov show ([ffc9e18](https://github.com/mzdun/cov/commit/ffc9e181d801de137caeaf72e1de89d63f937acd))
- **core**: show file coverage ([edd4e95](https://github.com/mzdun/cov/commit/edd4e954b4e1c623bdd8ffc5965ff3b0284b7f69))
- **core, rt**: add cov show command ([2475041](https://github.com/mzdun/cov/commit/24750415fc6b6d072a4c0702745a24936e8f76be))
- **rt**: format file contents for xterm ([b8471f4](https://github.com/mzdun/cov/commit/b8471f4da8f5bec8c320c4e7b241538a1077495d))

### Bug Fixes

- apply linter' note ([1ec1b27](https://github.com/mzdun/cov/commit/1ec1b2791580d09c83c71acc7073fcb8a0eaad99))
- remove compilation issues ([6d823e9](https://github.com/mzdun/cov/commit/6d823e987bb90a01c637615db5bc91cfe3ead1ef))
- stabilize show test, apply linter findings ([3ff576f](https://github.com/mzdun/cov/commit/3ff576fa6253072b8cd710ac1654316e63e1f2f0))
- **api**: removed another instance of an UB ([ba7fd88](https://github.com/mzdun/cov/commit/ba7fd88921067f9565253a7aa0f52fe3d05bf369))

## [0.16.0](https://github.com/mzdun/cov/compare/v0.15.2...v0.16.0) (2023-06-30)

### New Features

- add cov reset ([5a0e415](https://github.com/mzdun/cov/commit/5a0e415bb1ac35a2d5574841e5593156b0a8e939))
- recognize (nearly) repeated reports ([3e314b6](https://github.com/mzdun/cov/commit/3e314b6d5bfb4bd06cbc66c6d568dbd73976fef4))

### Bug Fixes

- patched an undefined behavior ([08fdaa1](https://github.com/mzdun/cov/commit/08fdaa1f9fdc80f98fd51677eddf0300d2197622))
- no `uz` on MSVC yet ([6c6d9df](https://github.com/mzdun/cov/commit/6c6d9df929f9ecef3274246c810d6aa6a0ce6749))

## [0.15.2](https://github.com/mzdun/cov/compare/v0.15.1...v0.15.2) (2023-05-28)

### Bug Fixes

- sort gcc/clang issues on GitHub dockers ([0d037a7](https://github.com/mzdun/cov/commit/0d037a721fd1c2d5d32b0bab75f61a80ba12566b))

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
