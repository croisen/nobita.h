# No external Build system In This App

Well I just wanted to try if I can just not use gnu make, cmake, conan, bazel,
or other c build systems and just define the build rules in C.

[tsoding](https://github.com/tsoding) have done it, and well it doesn't hurt to
try

[Unfiltered Journey](https://www.youtube.com/watch?v=3WnfNWqSwLg&list=PLtBi6iGOJtqg4Aimt-C-vYChz6_boprmp&pp=gAQBiAQB)

## Inspirations
-   [tsodinng's nobuild](https://github.com/tsoding/nobuild)
-   [nothings' stb](https://github.com/nothings/stb)
-   [zig build system](https://ziglang.org/learn/build-system/)

## TODOs

-   [x] Be a header-only library
-   [x] Build an executable
-   [x] Build a library
-   [x] Build sources to object files first then link
-   [x] Rebuild based on the last modification time of the source and output
        files
-   [x] Rebuild the build executable if it's sources are newer
-   [x] Support the prefix directory
-   [ ] Allow multiple commands to occur at once
-   [x] Have a cache directory to dump the object files in, instead of being
        next to their respective source files
-   [ ] Be crossplatform
