---
title: liara-renderer
description: What the reference renderer owns, why the render target abstraction exists before anything needs it, and what exists today.
sidebar:
  label: Overview
  order: 0
---

The reference implementation of the renderer interface. It takes render packets and produces pixels, and that is the whole of its job.

It is the most complex module in the project and the one whose replaceability matters most, since a renderer built on another graphics API is the substitution somebody would plausibly actually make. Confining every Vulkan decision to this repository is what keeps that possible.

## What it owns

**The Vulkan device layer.** Instance, physical device selection, logical device, queues, command pools. Built on Vulkan-Hpp and VMA, for the reasons in [ADR 0010](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/adr/0010-vulkan-without-abstraction-layer/).

**The swapchain.** Surface creation from the native window handle the host passes in, then creation, recreation on resize, image acquisition and presentation.

**Render targets.** Swapchain-backed and offscreen-texture-backed, behind one opaque handle, with image transitions, format negotiation and allocation strategy encapsulated here.

**The pipeline cache.** Shader module loading, pipeline state description and creation, and caching keyed by that state. The renderer consumes SPIR-V and does not compile GLSL, which CMake does at build time through `glslc`.

**The render passes.** Receiving a packet, sorting by material and pipeline, issuing draw calls, handling transparency, presenting.

**Debug rendering.** Lines, wireframes, bounding boxes and frustums, submitted through the debug primitives interface, in a pass of its own with its own pipeline.

**The ImGui integration.** Drawing ImGui is a rendering operation, so it happens here. In v0.x that serves the developer console and the stats overlay.

## What it does not

No ECS, no gameplay, no input handling.

No window creation. The window belongs to `liara-platform`, and this module receives a native handle from the host and nothing else.

No asset loading. The host hands it prepared CPU-side data that came from `liara-assets`, and it uploads that to the GPU. It never reads a file and never calls the assets module.

## The generalisation that costs nothing now

Render targets are abstract and a frame may carry several views from v0.1, before anything needs either. In standalone mode that reduces to rendering one view into the swapchain, which is no more complicated than the naive version would have been.

What it buys is that an editor becomes possible later without an interface change, since a viewport panel is a target that is not the swapchain. Retrofitting it would be a breaking change to the contract, so it is done first. The same applies to the debug pass, which the editor reuses for its gizmos.

## What exists today

The Vulkan device layer, and a window cleared to one colour. Creating a renderer opens the Vulkan loader at run time rather than linking it, for the reasons in [ADR 0016](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/adr/0016-vulkan-loaded-at-run-time/), creates a Vulkan 1.3 instance, selects a GPU, and creates a logical device with one queue, a command pool and a VMA allocator. A debug build refuses to start without the Khronos validation layer, so that a clean validation log means the layer ran, and turns on synchronization validation.

Given a window, the renderer creates a surface on it and a FIFO swapchain, and each frame clears the next image to the packet's background colour and presents it. Given no window (`LIARA_NATIVE_WINDOW_KIND_NONE`), it creates the device alone and presents nothing. Either way the console renderer still draws the packet as coloured cells in the terminal. Nothing recreates the swapchain yet. When the window system reports it out of date, as X11 typically does after a resize, the renderer stops presenting; under Wayland, which reports nothing on a resize, it keeps presenting images of the window's first size. Resize handling, later in v0.1, removes both limits. The [roadmap](https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/roadmap/v0-1/) says what the rest of v0.1 adds.

The packet it receives today is also a placeholder: a grid width and height, a background colour, and drawables that are an x, a y and a packed colour. The views, lights, meshes and materials arrive with v0.2.

## Where to go next

The [API reference](https://liara-engine.liara-engine-documentation.workers.dev/liara-renderer/latest/api/) is generated from the headers in `liara-interfaces`, which is where this module's contract lives. The internal C++ behind it is private and deliberately undocumented here.
