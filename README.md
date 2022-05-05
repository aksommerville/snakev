# Variations in the Key of Snake

"What does it take to develop a game on _Platform X_?"

We're going to write one of the simplest games, Snake, for as wide a variety of platforms as possible.
The hope is that this will yield some insight to the relative complexity, performance, and whatnot of those platforms.

Each platform implementation is its own thing entirely, there is no "make all" at this level.
If there's no platform-specific reason to do otherwise, these will build with make, as I usually do.

## libc

Works pretty good for Snake but it's hard to picture non-trivial games this way.

We could add color without much effort.

Input is fatally crippled, though it doesn't matter for Snake: There is only key down, no such thing as key up.

Audio and joysticks will not be available without bringing in platform-specific libraries (and then why not bring in video too?).

Assessment: Poor choice all around.

## Desired Platforms

- tiny
- linux-x11
- linux-glx
- linux-drm-fb
- linux-drm-gles
- macos: Surely more than one set of APIs to choose from here.
- mswin: ''
- java: ''
- js-canvas2d
- js-webgl
- js-node: What options do we have here?
- wasm-browser
- sdl
- unity
- mac68k
- macppc
- pippin
- picosystem
- nes
- snes
- gameboy
- ios
- android
