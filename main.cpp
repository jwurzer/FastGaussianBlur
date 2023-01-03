#include <iostream>
#include <chrono>

// image io
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// fast blur
#include "fast_gaussian_blur_template.h"

typedef unsigned char uchar;

// #define USE_FLOAT

int main(int argc, char * argv[])
{   
    // helper
    if( argc < 4 )
    {
        printf("%s [input] [output] [sigma] [passes - optional] [border policy - optional]\n", argv[0]);
        exit(1);
    }

    // load image
    int width, height, channels;
    uchar * image_data = stbi_load(argv[1], &width, &height, &channels, 0);
    printf("Source image: %s %dx%d (%d)\n", argv[1], width, height, channels);

    // read parameters
    const float sigma = std::atof(argv[3]);
    const int passes = argc > 4 ? std::atoi(argv[4]) : 3;
    const std::string policy = argc > 5 ? std::string(argv[5]) : "mirror";
    BorderPolicy ipolicy;
    if (policy == "mirror") ipolicy = BorderPolicy::kMirror;
    if (policy == "extend") ipolicy = BorderPolicy::kExtend;
    if (policy == "kcrop")  ipolicy = BorderPolicy::kKernelCrop;
    if (policy == "wrap")   ipolicy = BorderPolicy::kWrap;
    
    // temporary data
    std::size_t size = width * height * channels;
#ifdef USE_FLOAT
    float * new_image = new float[size];
    float * old_image = new float[size];
#else
    uchar * new_image = new uchar[size];
    uchar * old_image = new uchar[size];
#endif
    
    // channels copy r,g,b
    for(std::size_t i = 0; i < size; ++i)
    {
#ifdef USE_FLOAT
        old_image[i] = (float)image_data[i] / 255.f;
#else
        old_image[i] = image_data[i];
#endif
    }
    
    // stats
    auto start = std::chrono::system_clock::now(); 
    
    // perform gaussian blur
    // note: the implementation can work on any buffer types (uint8, uint16, uint32, int, float, double)
    // note: both old and new buffer are modified
    fast_gaussian_blur(old_image, new_image, width, height, channels, sigma, passes, ipolicy);
    
    // stats
    auto end = std::chrono::system_clock::now();
    float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    printf("Time %.4fms\n", elapsed);

    // convert result
    for(std::size_t i = 0; i < size; ++i)
    {
#ifdef USE_FLOAT
        image_data[i] = (uchar)(new_image[i] * 255.f);
#else
        image_data[i] = (uchar)(new_image[i]);
#endif
    }

    // save image
    std::string file(argv[2]), ext = file.substr(file.size()-3);
    if( ext == "bmp" )
        stbi_write_bmp(argv[2], width, height, channels, image_data);
    else if( ext == "jpg" )
        stbi_write_jpg(argv[2], width, height, channels, image_data, 90);
    else
    {
        if( ext != "png" )
        {
            printf("Image format '%s' not supported, writing default png\n", ext.c_str()); 
            file = file.substr(0, file.size()-4) + std::string(".png");
        }
        stbi_write_png(file.c_str(), width, height, channels, image_data, channels*width);
    }
    
    // clean memory
    stbi_image_free(image_data);
    delete[] new_image;
    delete[] old_image;

    return 0;
}
