# linux-opengl-render
This is a simple YUV video rendering module based on OpenGL, designed for Linux platform.

## compile

`cmake -B build -DCMAKE_BUILD_TYPE=Release`

`cmake --build build --config Release`


## run test
```
Usage: ./demo <mode:0-fit or 1-fullfit> <bg_color:0xRRGGBB>
```