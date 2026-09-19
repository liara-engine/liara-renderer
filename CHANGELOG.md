# Changelog

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
