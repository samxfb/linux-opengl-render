#include "linux_opengl_render.h"
#include <iostream>
#include <stdio.h>

int XServerErrorHandler(Display* display, XErrorEvent* error_event) {
    (void) display;
    printf("XError occured, type = %d, request_code = %d, error_code = %d\n",  
                error_event->type, error_event->request_code , error_event->error_code);
    return 0;
}

// Helper class that registers X Window error handler.
class XErrorTrap {
public:
    explicit XErrorTrap(Display* display)
    {
        (void) display;
        XSetErrorHandler(&XServerErrorHandler);
    }
    ~XErrorTrap()
    {

    }

private:
    XErrorTrap(const XErrorTrap &) = delete;
    XErrorTrap & operator=(const XErrorTrap &) = delete;
};

VideoRendererOpenGL::VideoRendererOpenGL(Window window)
: window_(window)
{
    if (InitContext()) {
        initializeGL();
        isInitialized_ = true;
    }
   std::cout << "VideoRendererOpenGL ctor end, window:" << (long) window_ << std::endl;
}

VideoRendererOpenGL::~VideoRendererOpenGL()
{
    // releaseGL(); // fixme
    releaseContext();

    std::cout << "~VideoRendererOpenGL, window:" << (long) window_ << std::endl;
}

bool VideoRendererOpenGL::CheckRender(Window window)
{
    if (!isInitialized_ || window != window_) {
        return false;
    }
    // change to current window
    return glXMakeCurrent(display_, window, glContext_); // return bool
}

void VideoRendererOpenGL::releaseContext()
{
    if (display_ && glContext_) {
        glXMakeCurrent(display_, None, NULL);
    }
    
    if (glContext_) {
        glXDestroyContext(display_, glContext_);
        glContext_ = NULL;
    }

    if (display_) {
        XCloseDisplay(display_);
        display_ = NULL;
    }
}

bool VideoRendererOpenGL::InitContext()
{
    // 连接X服务器
    display_ = XOpenDisplay(NULL);
    if (display_ == NULL) {
        std::cout << "VideoRendererOpenGL::InitContext XOpenDisplay failed" << std::endl;
        return false;
    }
   
    // 创建OpenGL上下文
    static int visualAttributes[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        // GLX_DOUBLEBUFFER,
        None
    };

    XVisualInfo *visualInfo = glXChooseVisual(display_, 0, visualAttributes);
    if (visualInfo == NULL) {
        std::cout << "VideoRendererOpenGL::InitContext glXChooseVisual failed" << std::endl;
        XCloseDisplay(display_);
        display_ = NULL;
        return false;
    }

    glContext_ = glXCreateContext(display_, visualInfo, NULL, GL_TRUE);
    if (glContext_ == NULL) {
        std::cout << "VideoRendererOpenGL::InitContext glXCreateContext failed" << std::endl;
        XCloseDisplay(display_);
        display_ = NULL;
        XFree(visualInfo);
        return false;
    }
    XFree(visualInfo);

    // 将OpenGL上下文与窗口关联
    bool ret = glXMakeCurrent(display_, window_, glContext_); // return bool
    if (!ret) {
        std::cout << "VideoRendererOpenGL::glXMakeCurrent failed" << std::endl;
        glXDestroyContext(display_, glContext_);
        glContext_ = NULL;
        XCloseDisplay(display_);
        display_ = NULL;
        return false;
    }

    return true;
}

void VideoRendererOpenGL::releaseGL()
{
    glDeleteTextures(1, &id_y);
    glDeleteTextures(1, &id_u);
    glDeleteTextures(1, &id_v);

    glDisableVertexAttribArray(ATTRIB_VERTEX);
    glDisableVertexAttribArray(ATTRIB_TEXTURE);

    glUseProgram(0);
    glDetachShader(p, v);
    glDetachShader(p, f);
    glDeleteProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
}

void VideoRendererOpenGL::initializeGL()
{
    // Shader: step1
    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vs =
    "attribute vec4 vertexIn; \
    attribute vec2 textureIn; \
    varying vec2 textureOut;  \
    void main(void)           \
    {                         \
        gl_Position = vertexIn; \
        textureOut = textureIn; \
    }";
    const char *fs =
    "varying vec2 textureOut;\
    uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";

	//Shader: step2
    glShaderSource(v, 1, &vs, NULL);
    glShaderSource(f, 1, &fs, NULL);
	//Shader: step3
    glCompileShader(v);
    glCompileShader(f);

	//Program: Step1
    p = glCreateProgram(); 
	//Program: Step2
    glAttachShader(p, v);
    glAttachShader(p, f); 
    glBindAttribLocation(p, ATTRIB_VERTEX, "vertexIn");
    glBindAttribLocation(p, ATTRIB_TEXTURE, "textureIn");
	//Program: Step3
    glLinkProgram(p);
	//Program: Step4
    glUseProgram(p);
 
	//Get Uniform Variables Location
	textureUniformY = glGetUniformLocation(p, "tex_y");
	textureUniformU = glGetUniformLocation(p, "tex_u");
	textureUniformV = glGetUniformLocation(p, "tex_v");
    
	static const GLfloat vertexVertices[] = {
        //顶点坐标
	    -1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f,  1.0f,
		1.0f,  1.0f,
	};    

	static const GLfloat textureVertices[] = {
        //纹理坐标
		0.0f,  1.0f,
		1.0f,  1.0f,
		0.0f,  0.0f,
		1.0f,  0.0f,
	}; 
	//Set Arrays
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
	//Enable Arrays
    glEnableVertexAttribArray(ATTRIB_VERTEX);   
    glEnableVertexAttribArray(ATTRIB_TEXTURE);
 
	//Init Texture
    glGenTextures(1, &id_y); 
    glBindTexture(GL_TEXTURE_2D, id_y);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenTextures(1, &id_u);
    glBindTexture(GL_TEXTURE_2D, id_u);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenTextures(1, &id_v); 
    glBindTexture(GL_TEXTURE_2D, id_v);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool VideoRendererOpenGL::RenderYuvData(const char *data, unsigned int size, int video_width, int video_height, ScalingMode mode, uint32_t bg_color) {
    if (data) {
        if (window_forever_invalid) {
            return false;
        }
        data_y.clear();
        data_u.clear();
        data_v.clear();

        int data_u_offset = video_width * video_height;
        int data_v_offset = video_width * video_height + video_width * video_height / 4;
        data_y.insert(data_y.end(), data, data + video_width * video_height);
        data_u.insert(data_u.end(), data + data_u_offset, data + data_u_offset + video_width * video_height / 4);
        data_v.insert(data_v.end(), data + data_v_offset, data + data_v_offset + video_width * video_height / 4);

        XWindowAttributes window_attributes;
        XErrorTrap xtrap(display_);
        Status status = XGetWindowAttributes(display_, window_, &window_attributes);
        if (status == 0 || window_attributes.width == 0 || window_attributes.height == 0) {
            // 获取窗口属性失败，窗口无效
            std::cout << "XGetWindowAttributes failed, window_:" << window_ << std::endl;
            window_forever_invalid = true;
            return false;
        }
        int show_x = 0;
        int show_y = 0;
        int show_w = window_attributes.width;
        int show_h = window_attributes.height;
        
        int width = video_width; // video data width
        int height = video_height;// video data height

        if (last_window_width != show_w || 
                    last_window_height != show_h ||
                    last_render_mode != (int) mode ||
                    last_video_w != width ||
                    last_video_h != height) {
            last_window_width  = show_w;
            last_window_height = show_h;
            last_render_mode = mode;
            last_video_w = width;
            last_video_h = height;
            int dst_x = 0;
            int dst_y = 0;
            int dst_w = 0;
            int dst_h = 0;
            double wnd_ratio = show_w * 1.0 / show_h;
            double texture_ratio = width * 1.0 / height;
            if (mode == kScalingModeFit) {
                // adjust dst render rect
                if (wnd_ratio > texture_ratio) {
                    int rendering_box_width = show_h * width / height;
                    dst_w = rendering_box_width;
                    dst_h = show_h;
                    dst_x = show_x + (show_w - rendering_box_width) / 2;
                    dst_y = show_y;
                } else {
                    int rendering_box_height = show_w * height / width;
                    dst_w = show_w;
                    dst_h = rendering_box_height;
                    dst_x = show_x;
                    dst_y = show_y + (show_h - rendering_box_height) / 2;
                }
            } else {
                dst_w = show_w;
                dst_h = show_h;
                dst_x = show_x;
                dst_y = show_y;
            }
            glViewport(dst_x, dst_y, dst_w, dst_h);
        }
        Update(mode, bg_color);
    }

    return true;
}

void VideoRendererOpenGL::Update(ScalingMode mode, uint32_t bg_color)
{
    int red = bg_color >> 16;
    int green = (bg_color >> 8) & 0xff;
    int blue = bg_color & 0xff;

	//Clear
    glClearColor(red / 255.0, green / 255.0, blue / 255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // align to 1 bytes
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
	//Y
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id_y);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, last_video_w, last_video_h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data_y.data());
    glUniform1i(textureUniformY, 0); 
	//U
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, id_u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, last_video_w >> 1, last_video_h >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data_u.data());
    glUniform1i(textureUniformU, 1);
	//V
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, id_v);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, last_video_w >> 1, last_video_h >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data_v.data());
    glUniform1i(textureUniformV, 2);   
      
    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Show
	// Double
    // glXSwapBuffers(display_, window_);
	// Single
	glFlush();
}