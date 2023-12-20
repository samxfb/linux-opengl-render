#ifndef LINUX_OPENGL_RENDER_H
#define LINUX_OPENGL_RENDER_H

#define GL_GLEXT_PROTOTYPES // enable gl extention prototypes
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <vector>

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

enum ScalingMode {
    kScalingModeFit = 0,
    kScalingModeFullFill,
};

class VideoRendererOpenGL {
public:
    VideoRendererOpenGL(Window window);
    virtual ~VideoRendererOpenGL();

public:
    bool CheckRender(Window window);
    bool RenderYuvData(const char *data, unsigned int size, int video_width, int video_height, ScalingMode mode, uint32_t bg_color);
private:
    bool InitContext();
    void releaseContext();
    void initializeGL();
    void releaseGL();
    void Update(ScalingMode mode, uint32_t bg_color);

private:
    Window window_ = 0;
    Display *display_ = NULL;
    GLXContext glContext_ = NULL;
    GLuint id_y, id_u, id_v; // Texture id
    GLuint v = 0; // vertex shader
    GLuint f = 0; // fragment sharder
    GLuint p = 0; // program
    GLint textureUniformY = -1; // uniform tex_y location
    GLint textureUniformU = -1; // uniform tex_u location
    GLint textureUniformV = -1; // uniform tex_v location 
    std::vector<unsigned char> data_y;
    std::vector<unsigned char> data_u;
    std::vector<unsigned char> data_v;
    uint32_t last_video_w = 0;
    uint32_t last_video_h = 0;
    uint32_t last_window_width = 0;
    uint32_t last_window_height = 0;
    int last_render_mode = 0; // kScalingModeFit
    bool isInitialized_ = false;
    bool window_forever_invalid = false;
};

#endif // LINUX_OPENGL_RENDER_H