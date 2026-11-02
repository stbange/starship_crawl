Starship Crawl uses SDL 3.2.30, pinned to commit
`f5e5f6588921eed3d7d048ce43d9eb1ff0da0ffc`, from
https://github.com/libsdl-org/SDL. SDL source is downloaded during configuration;
it is not vendored into this repository. The WebAssembly build also uses the
Emscripten toolchain and its runtime libraries under their respective upstream
licenses: https://github.com/emscripten-core/emscripten/blob/main/LICENSE.

SDL license:

Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
