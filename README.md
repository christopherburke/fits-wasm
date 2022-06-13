# fits-wasm
Webassembly version of the cfitsio library.

The fits format is a popular storage format for astronomy data. This is a project that demonstrates using the [cfitsio library](https://heasarc.gsfc.nasa.gov/fitsio/) compiled to webassembly. This allows javascript to read from fits files approaching native speeds. The demonstration focuses on reading from flux time series files produced by the TESS mission.

### Webassembly with cfitsio compile notes
These are notes that describe the compile and build procedure. I developed this  on an M1 mac laptop. Prequisites are a working emscripten sdk.
1. One needs to compile cfitsio from source which requires the zlib library. I downloaded a tar.gz of [zlib 1.2.12](https://zlib.net/). Unzip/untar zlib, then the following commands built the library.
```
emconfigure ./configure
emmake make
```

