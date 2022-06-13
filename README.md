# fits-wasm
Webassembly version of the cfitsio library.

The fits format is a popular storage format for astronomy data. This is a project that demonstrates using the [cfitsio library](https://heasarc.gsfc.nasa.gov/fitsio/) compiled to webassembly. This allows javascript to read from fits files approaching native speeds. The demonstration focuses on reading from flux time series files produced by the TESS mission. This project was integrated into my personal [web page](https://eta-earth.org/tess_fits_play.html) so you can try it out there. 

### Webassembly with cfitsio compile notes
These are notes that describe the compile and build procedure. I developed this  on an M1 mac laptop. Prequisites are a working emscripten sdk.
1. One needs to compile cfitsio from source which requires the zlib library. I downloaded a tar.gz of [zlib 1.2.12](https://zlib.net/). Unzip/untar zlib, then the following commands built the zlib library.
```
emconfigure ./configure
emmake make
```
2. Download cfitsio library tar.gz. I used cfitsio-4.1.0. In the configure and build of cfitsio library, I struggled to get the paths for the emscripten built zlib, but these are the commands that worked for me.
```
export LDFLAGS="-L/Users/cjburke/Work/fits-wasm/zlib-1.2.12"
export CFLAGS="-I/Users/cjburke/Work/fits-wasm/zlib-1.2.12"
emconfigure ./configure --disable-curl --without-zlib-check CPPFLAGS=-D__x86_64__=1
emmake make libcfitsio.a
```
The LDFLAGS and CFLAGS are set to inform where the zlib libarary and headers were located. During compile curl was disabled, and despite my efforts I needed to add the without-zlib-check flag. Configure did not properly report the architecture to cfitsio and it was not working due to a endian check. However, by using the CPPFLAGS to define a __x86_64__ architecture, allowed it to work. My understanding is that the webassembly 'stack machine' uses little-endian and 64 bit, which matches with the x86_64 architecture?

3. For the TESS light curve reading, the c code that is compiled to webassembly is located at `src/tess_fitslc_export.c`. I am using CMake with a 'UNIX make' target in order to build project. If you examine CMakeLists.txt you will find that I struggled to have CMake automatically find the zlib and cfitsio headers and libraries. I pretty much hardcoded their locations. I also am very inexperienced with CMake. From the fits-wasm directory I run the following commands to compile.
```
emcmake cmake -S . -B build/
cd build
make
```
This puts the final javascript and wasm files in the build directory.

4. The html `tess_fits_play.html` demonstrates how to use the tess light curve reader, and uses plotly.js to make an interactive figure of the light curve. An additional javascript `tess_fitslc_export_pre.js` is needed for memory management on the javascript side. By default the html will have an input button where it will read a local fits file. There is also a way to pass a resource in the URL i n order to stream the fits file from MAST. However, this violates CORS restrictions, thus I use the [Allow CORS](https://chrome.google.com/webstore/detail/allow-cors-access-control/lhobafahddgcelffkeicbaginigeejlf?hl=en) Chrome extension to turn off CORS restrictions. You shouldn't do this all the time.

### TODOS:
1.  Support other HLSP light curve formats
2. Play around with increase optimization in the build `-O2` to see if that improves anything
3. Detrending and phase folding?
4. Multiple sectors of light curves?

