---
title: Changelog
description: The changelog for the Liara engine, with links to the GitHub compare view for each version.
sidebar:
    order: 1
---

# Changelog

## [0.2.0](https://github.com/liara-engine/liara-renderer/compare/v0.1.2...v0.2.0) (2026-09-24)


### ⚠ BREAKING CHANGES

* liara_renderer_destroy returns void instead of liara_result_t and takes a non-const liara_renderer_handle_t*. A caller that checked the return value no longer compiles.

### Bug Fixes

* **ci:** update pull request triggers for CI workflow ([5b2733b](https://github.com/liara-engine/liara-renderer/commit/5b2733b66e156e0a94d1e5ef7dba3edf837c793c))


### Code Refactoring

* destroy returns void ([#17](https://github.com/liara-engine/liara-renderer/issues/17)) ([bc41f16](https://github.com/liara-engine/liara-renderer/commit/bc41f166d5ef6c3dd41d41e762bd72c25be812e0))

## [0.1.2](https://github.com/liara-engine/liara-renderer/compare/v0.1.1...v0.1.2) (2026-09-20)


### Bug Fixes

* **ci:** remove main branch trigger since push to main is nom refused ([e560e18](https://github.com/liara-engine/liara-renderer/commit/e560e18802ffdd357c3b8e3e1540fa054902a29a))
* **clang-tidy:** adjust header filter regex and update checks ([71cc578](https://github.com/liara-engine/liara-renderer/commit/71cc578aaf3b890ee939cee91747949bf89f902b))

## [0.1.1](https://github.com/liara-engine/liara-renderer/compare/v0.1.0...v0.1.1) (2026-09-19)


### Features

* enhance build configuration, documentation, and CI workflows ([#11](https://github.com/liara-engine/liara-renderer/issues/11)) ([86e5751](https://github.com/liara-engine/liara-renderer/commit/86e575158ffccc6cdb9ddd19dd56917a834b726c))

## [0.1.0](https://github.com/liara-engine/liara-renderer/compare/v0.0.2...v0.1.0) (2026-08-01)


### ⚠ BREAKING CHANGES

* **renderer:** drop support of ABI v0.1.x

### Features

* **cmake:** enhance installation process with package configuration and versioning ([4e7b9a9](https://github.com/liara-engine/liara-renderer/commit/4e7b9a957801df277ec714bc26073491752de77e))
* **renderer:** add initial configuration for LiaraRenderer ([049a213](https://github.com/liara-engine/liara-renderer/commit/049a2139d6a7b6f232172420e10ad256c43247fb))


### Code Refactoring

* **renderer:** drop support of ABI v0.1.x ([5fe7eca](https://github.com/liara-engine/liara-renderer/commit/5fe7eca8f0b2ce017d0620c4f07e1dbf9f1211bd))

## [0.0.2](https://github.com/liara-engine/liara-renderer/compare/v0.0.1...v0.0.2) (2026-07-29)


### Features

* **renderer:** add compatibility with ABI v0.1.0 ([a8bc018](https://github.com/liara-engine/liara-renderer/commit/a8bc018d1b943d9e09800cc97b5a84aa19316336))
* **renderer:** add liara_renderer_println and liara_renderer_version ([7e43ab1](https://github.com/liara-engine/liara-renderer/commit/7e43ab18631dae93a9c9e607b700807b83551cd4))
* **renderer:** add LiaraRenderer class for frame rendering with console output ([403c3f0](https://github.com/liara-engine/liara-renderer/commit/403c3f06031161d3df695baa194deeca7f0db786))


### Bug Fixes

* add support for abi v0.1.1 ([#3](https://github.com/liara-engine/liara-renderer/issues/3)) ([f23cfab](https://github.com/liara-engine/liara-renderer/commit/f23cfab813e56aa86324e22f423b8c683ad3dcab))
* **cmake:** update error message for missing Liara::Interfaces to include documentation link ([be68042](https://github.com/liara-engine/liara-renderer/commit/be6804293ea492cbfae2e049aca3636a3e297a73))
* **renderer:** update return type for renderer functions to liara_result_t due to depreciation of liara_result ([b8d12bf](https://github.com/liara-engine/liara-renderer/commit/b8d12bf6d8c5cf2f18488aa8c0b7dfd0ce0f6479))

## 0.0.1 (2026-07-18)


### Features

* add initial documentation files and configuration ([1c1d130](https://github.com/liara-engine/liara-renderer/commit/1c1d130e157bc480cf968e6b7684a99897c753ce))
* **ci:** add workflows for commitlint, documentation generation, and release-please ([5fdee43](https://github.com/liara-engine/liara-renderer/commit/5fdee4308fb5fb0b89575b48972a5e298faaa23d))
* **renderer:** implement a very basic console renderer ([43c4799](https://github.com/liara-engine/liara-renderer/commit/43c4799ff97adbe82f5c2d926c6db71fe8f0336d))
