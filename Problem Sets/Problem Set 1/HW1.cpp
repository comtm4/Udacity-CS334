#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "utils.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include <string>

cv::Mat imageRGBA;
cv::Mat imageGrey;

uchar4        *d_rgbaImage__;
unsigned char *d_greyImage__;

size_t numRows() { return imageRGBA.rows; }
size_t numCols() { return imageRGBA.cols; }

//return types are void since any internal error will be handled by quitting
//no point in returning error codes...
//returns a pointer to an RGBA version of the input image
//and a pointer to the single channel grey-scale output
//on both the host and device
void preProcess(uchar4 **inputImage, unsigned char **greyImage,
                uchar4 **d_rgbaImage, unsigned char **d_greyImage,
                const std::string &filename) {
  //make sure the context initializes ok
  checkCudaErrors(cudaFree(0));

  cv::Mat image;
  image = cv::imread(filename.c_str(), CV_LOAD_IMAGE_COLOR);
  if (image.empty()) {
    std::cerr << "Couldn't open file: " << filename << std::endl;
    exit(1);
  }

  cv::cvtColor(image, imageRGBA, CV_BGR2RGBA);

  //allocate memory for the output
  imageGrey.create(image.rows, image.cols, CV_8UC1);

  //This shouldn't ever happen given the way the images are created
  //at least based upon my limited understanding of OpenCV, but better to check
  if (!imageRGBA.isContinuous() || !imageGrey.isContinuous()) {
    std::cerr << "Images aren't continuous!! Exiting." << std::endl;
    exit(1);
  }

  *inputImage = (uchar4 *)imageRGBA.ptr<unsigned char>(0);  //指向读入的内存
  *greyImage  = imageGrey.ptr<unsigned char>(0);            //指向被分配的内存

  const size_t numPixels = numRows() * numCols();
  //allocate memory on the device for both input and output
  //GpuTimer timer;
  //timer.Start();
  float timer = clock();
  checkCudaErrors(cudaMalloc(d_rgbaImage, sizeof(uchar4) * numPixels));     //分配gpu内存
  checkCudaErrors(cudaMalloc(d_greyImage, sizeof(unsigned char) * numPixels));  //分配gpu内存
  checkCudaErrors(cudaMemset(*d_greyImage, 0, numPixels * sizeof(unsigned char))); //make sure no memory is left laying around  内存清0

  //copy input array to the GPU
  checkCudaErrors(cudaMemcpy(*d_rgbaImage, *inputImage, sizeof(uchar4) * numPixels, cudaMemcpyHostToDevice));//cpu的rgba的内存赋值到gpu上

  printf("The time of allocate the memroy and copy memory from host to device is: %f msecs.\n", (clock()-timer)/CLOCKS_PER_SEC/1000);

  d_rgbaImage__ = *d_rgbaImage;
  d_greyImage__ = *d_greyImage;
}

void postProcess(const std::string& output_file, unsigned char* data_ptr) {
  cv::Mat output(numRows(), numCols(), CV_8UC1, (void*)data_ptr);

  //output the image
  cv::imwrite(output_file.c_str(), output);
}

void cleanup()
{
  //cleanup
  cudaFree(d_rgbaImage__);
  cudaFree(d_greyImage__);
}

void generateReferenceImage(std::string input_filename, std::string output_filename)
{
  cv::Mat reference = cv::imread(input_filename, CV_LOAD_IMAGE_GRAYSCALE);

  cv::imwrite(output_filename, reference);

}
