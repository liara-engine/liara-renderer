---
title: About Liara Renderer
description: The reference Vulkan renderer for the Liara Engine.
sidebar:
    order: 0
---

# liara-renderer

> The reference Vulkan renderer for the Liara Engine.

It takes render packets and produces pixels. It knows nothing about the ECS, about gameplay or about the editor, and only ever sees the data the host hands it each frame.

It is the most complex module in the project and the one whose replaceability matters most, since a renderer built on another graphics API is the substitution somebody would plausibly actually make. Its only dependency is [liara-interfaces](https://github.com/liara-engine/liara-interfaces).

## Status

**Phase 0.** What exists is a placeholder that can be created, handed a packet and destroyed, which is enough to prove the composition works. Real Vulkan work starts with v0.1: instance, device, swapchain, and one triangle.

## Documentation

- **[Module guide](https://liara-engine.liara-engine-documentation.workers.dev/liara-renderer/latest/guides/)**: what this module is for and what it will not contain.
- **[API reference](https://liara-engine.liara-engine-documentation.workers.dev/liara-renderer/latest/api/)**: generated from the headers.
- **Architecture**: the meta repository's [architecture](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/architecture/) and [modules](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/modules/renderer/) pages, plus [ADR 0010](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/adr/0010-vulkan-without-abstraction-layer/) for why there is no abstraction layer over Vulkan.
- **Workflow**: [contributing](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/contributing/).

## Building it on its own

Usually you do not. This repository is consumed through the workspace superbuild, or through `find_package(LiaraRenderer)` from an install.

Two things make a bare clone awkward, and both are deliberate. It has no `CMakePresets.json`, because presets are generated into the workspace from a template in the meta repository. And it fails to configure without `Liara::Interfaces`, with a message saying so.

To build it alone against an installed contract:

```bash
git clone https://github.com/liara-engine/liara-renderer.git
cd liara-renderer

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_PREFIX_PATH=/path/to/liara-interfaces/install
cmake --build build
ctest --test-dir build --output-on-failure
```

From v0.1 this also needs the Vulkan SDK, since the module links against the loader and compiles its shaders with `glslc`. [Bootstrap](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/bootstrap/) covers installing it.

## Consuming it

```cmake
find_package(LiaraRenderer REQUIRED)
target_link_libraries(my_module PRIVATE Liara::Renderer)
```

Always through the `Liara::Renderer` alias rather than the bare target name, so that a build compiles against exactly what an external consumer gets.

## License

[MIT](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/about/license/).
