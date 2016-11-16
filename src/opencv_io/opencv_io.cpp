#include <iostream>
#include <sstream>
#include <opencv2/highgui/highgui.hpp>

#include "../janus_io.cpp"
#include "iarpa_janus_io.h"

using namespace std;
using namespace cv;

janus_error janus_load_media(const string &filename, janus_media &media)
{
    // NLC HACK: OpenCV doesn't read gifs, so use converted replacement file
    string fixed_filename(filename);
    const size_t gpos = fixed_filename.find(".gif") != string::npos ?
      fixed_filename.find(".gif") : fixed_filename.find(".GIF");

    if (gpos != string::npos) {
      fixed_filename.replace(gpos, 4, ".jpg");
    }
    //
    // FIXME: OpenCV 3.1 cv::imread()  uses EXIF rototion data to automatically rotate an image
    // This breaks some CS3 ground truth annotations
    // cvLoadImage ignores EXIF as a 'feature'
    CvMat* pmat = cvLoadImageM(fixed_filename.c_str());
    cv::Mat img;
    if (pmat) {
      img = cv::Mat(pmat->rows, pmat->cols, CV_8UC3, pmat->data.fl);
    }

    // NLC END HACK

#if 0
    // NLC DEBUG IMG
    static size_t img_cnt = 0;
    stringstream dbg_img_fn;
    dbg_img_fn << "opencv_io_" << (img_cnt++) << ".png";
    imwrite(dbg_img_fn.str(), img);
    /// END NLC DEBUG IMG
#endif


    if (!img.data) { // Couldn't load as an image maybe it's a video
        VideoCapture video(filename);
        if (!video.isOpened()) {
            fprintf(stderr, "Fatal - Janus failed to read: %s\n", filename.c_str());
            return JANUS_INVALID_MEDIA;
        }

        Mat frame;
        bool got_frame = video.read(frame);
        if (!got_frame)
            return JANUS_INVALID_MEDIA;

        media.width = frame.cols;
        media.height = frame.rows;
        media.color_space = frame.channels() == 3 ? JANUS_BGR24 : JANUS_GRAY8;

        do {
            janus_data *data = new janus_data[media.width * media.height * (media.color_space == JANUS_BGR24 ? 3 : 1)];
            memcpy(data, frame.data, media.width * media.height * (media.color_space == JANUS_BGR24 ? 3 : 1));
            media.data.push_back(data);
        } while (video.read(frame));
    }

    media.width = img.cols;
    media.height = img.rows;
    media.color_space = (img.channels() == 3 ? JANUS_BGR24 : JANUS_GRAY8);

    janus_data *data = new janus_data[media.width * media.height * (media.color_space == JANUS_BGR24 ? 3 : 1)];
    memcpy(data, img.data, media.width * media.height * (media.color_space == JANUS_BGR24 ? 3 : 1));
    media.data.push_back(data);

    cvReleaseMat(&pmat);

    return JANUS_SUCCESS;
}

janus_error janus_free_media(janus_media &media)
{
    for (size_t i = 0; i < media.data.size(); i++) {
        delete[] media.data[i];
    }
    return JANUS_SUCCESS;
}
