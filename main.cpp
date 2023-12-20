#include "linux_opengl_render.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <string>

#define PATH_MAX 1024
std::string GetExeDirPath() {
  char buf[PATH_MAX];
  int ret = readlink("/proc/self/exe", buf, PATH_MAX);
  if (ret < 0 || (ret >= PATH_MAX)) { // error or truncated
      return "";
  } else {
    auto SplitFileName = [](const std::string &str)->std::string {
      auto found = str.find_last_of('/');
      return str.substr(0, found);
    };
    
    return SplitFileName(std::string(buf));
  }
}

#define pixel_w 1280
#define pixel_h 720

class IYuvDataSource
{
public:
    IYuvDataSource() {}

    virtual ~IYuvDataSource() {}

    bool OnYuvDataComming(char *data, unsigned int size)
    {
        if (!yuv_render_) {
            yuv_render_ = std::make_shared<VideoRendererOpenGL>(window_);
        }
        bool render_valid = yuv_render_->CheckRender(window_);
        if (!render_valid) {
            yuv_render_ = std::make_shared<VideoRendererOpenGL>(window_);
            render_valid = yuv_render_->CheckRender(window_);
        }
        if (render_valid) {
            if (yuv_render_->RenderYuvData((const char *) data, size, pixel_w, pixel_h, (ScalingMode) mode_, bg_color_)) {
                return true;
            }   
        }

        return false;
    }

    int SetRenderConfig(Window window, int mode, int bg_color)
    {
        window_ = window;
        mode_ = mode;
        bg_color_ = bg_color;
    }

private:
    std::shared_ptr<VideoRendererOpenGL> yuv_render_ = nullptr;
    Window window_ = 0;
    int mode_ = 0;
    int bg_color_ = 0;
};

class YuvDataSourceFile : public IYuvDataSource
{
public:
    YuvDataSourceFile() : stop_(false)
    {
        
    }

    void Start(const char* file_name)
    {
       file = fopen(file_name, "rb");
        if (!file) {
            perror("open file failed");
            return;
        }
        int buf_size = pixel_w * pixel_h * 3 / 2;
        buf = new unsigned char[buf_size];

        worker = std::thread([this, buf_size]() {
            int offset = 0;
            int cnt = 1;
            while (!stop_.load()) {
                if (fread(buf, 1, buf_size, file) != buf_size) {
                    // Loop
                    cnt = 1;
                    offset = 0;
                    fseek(file, 0, SEEK_SET);
                    fread(buf, 1, buf_size, file);
                }
                // std::cout << "OnYuvDataComming: " << cnt++ << std::endl;
                if (!OnYuvDataComming((char *)buf, buf_size)) {
                    break;
                }
                offset += buf_size;
                fseek(file, offset, SEEK_SET);
                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 20fps
            }
        });
    }

    void Stop()
    {
        stop_.store(true);

        if (worker.joinable()) {
            worker.join();
        }
    }

    virtual ~YuvDataSourceFile()
    {
        Stop();

        if (file) {
            fclose(file);
            file = nullptr;
        }
        if (buf) {
            delete [] buf;
            buf = nullptr;
        }
    }
private:
    FILE *file = nullptr;          //YUV file
    unsigned char *buf = nullptr;  //Buffer used to store the data to be read
    std::atomic_bool stop_;
    std::thread worker;
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <mode> <bg_color>" << std::endl;
        return -1;
    }

    // 1. Create a window
    // 连接X服务器
    Display *_display = XOpenDisplay(NULL);
    if (_display == NULL) {
        printf("XOpenDisplay failed\n");
        return -1;
    }
    // 创建窗口
    Window _window = XCreateSimpleWindow(_display, DefaultRootWindow(_display), 100, 100, 500, 500, 0, 0, 0);
    std::cout << "window: " << _window << std::endl;
    // 显示窗口
    XMapWindow(_display, _window);
    XStoreName(_display, _window, "Simplest Video Play OpenGL (Texture) - X11");
    XFlush(_display);

    // 2. Create a yuv data source
    YuvDataSourceFile yuv_data_source;
    int mode =std::stoi(argv[1], nullptr, 10);
    int bg_color = std::stoi(argv[2], nullptr, 16);
    yuv_data_source.SetRenderConfig(_window, mode, bg_color);
    yuv_data_source.Start((GetExeDirPath() + "/1280x720_20fps_I420.yuv").c_str());

    // 3. exit
    std::cout << "Press any key to exit..." << std::endl;
    std::cin.get();

    // 4. close
    yuv_data_source.Stop();
    XDestroyWindow(_display, _window);
    XCloseDisplay(_display);
    
    return 0;
}