# GLU tessellator (libtess)

The polygon tessellator from [Mesa GLU](https://gitlab.freedesktop.org/mesa/glu) 9.0.3
(`src/libtess/`, `include/GL/glu.h`, `src/include/gluos.h`), unmodified, under the
SGI Free Software License B 2.0 stated at the top of each file.

libcitygml tessellates CityGML polygons with GLU (`gluNewTess` ...). macOS and Linux link the
system or Homebrew GLU; iOS has none, so the iOS build compiles these files into the extension.
`include/GL/gl.h` holds the few OpenGL types and enums they need, `include/OpenGL/glu.h`
forwards libcitygml's Apple include, and `src/glu_error.c` provides `gluErrorString()`
for the tessellator errors.
