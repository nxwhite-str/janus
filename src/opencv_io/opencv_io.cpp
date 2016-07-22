#include <iostream>
#include <opencv2/highgui/highgui.hpp>

#include "../janus_io.cpp"
#include "iarpa_janus_io.h"

#include <ctime>

using namespace cv;

class log_function final {
  public:
    log_function(string name) : mName(name), start(clock()) {}

    ~log_function() {
      cout << "[" << mName << "] complete in "
        << (1000.0 * (clock() - start) / CLOCKS_PER_SEC)
        << " ms" << endl;
    }

    log_function(log_function&) = delete;
    log_function& operator=(log_function&) = delete;

  private:
    std::string mName;
    clock_t start;
};

static janus_image janusFromOpenCV(const Mat &mat)
{
    assert(mat.data && mat.isContinuous());
    janus_image image;
    image.width = mat.cols;
    image.height = mat.rows;
    image.color_space = (mat.channels() == 3 ? JANUS_BGR24 : JANUS_GRAY8);
    image.data = (janus_data*)malloc(image.width * image.height * (image.color_space == JANUS_BGR24 ? 3 : 1));
    memcpy(image.data, mat.data, image.width * image.height * (image.color_space == JANUS_BGR24 ? 3 : 1));
    return image;
}

janus_error janus_read_image(const char *file_name, janus_image *image)
{
    log_function lf("janus_read_image:"+string(file_name));
    const Mat mat = imread(file_name);
    if (!mat.data) {
        fprintf(stderr, "Fatal - Janus failed to read: %s\n", file_name);
        return JANUS_INVALID_IMAGE;
    }
    *image = janusFromOpenCV(mat);
    return JANUS_SUCCESS;
}

void janus_free_image(janus_image image)
{
    log_function lf("janus_free_image");
    free(image.data);
}

janus_error janus_open_video(const char *file_name, janus_video *video)
{
    *video = reinterpret_cast<janus_video>(new VideoCapture(file_name));
    return JANUS_SUCCESS;
}

janus_error janus_read_frame(janus_video video, janus_image *image)
{
    Mat mat;
    reinterpret_cast<VideoCapture*>(video)->read(mat);
    if (!mat.data)
        return JANUS_INVALID_VIDEO;
    *image = janusFromOpenCV(mat);
    return JANUS_SUCCESS;
}

void janus_close_video(janus_video video)
{
    delete reinterpret_cast<VideoCapture*>(video);
}
