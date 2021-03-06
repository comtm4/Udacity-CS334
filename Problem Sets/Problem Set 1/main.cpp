//Udacity HW1 Solution

#include <iostream>
#include "timer.h"
#include "utils.h"
#include <string>
#include <stdio.h>
#include "reference_calc.h"
#include "compare.h"

void your_rgba_to_greyscale(const uchar4 * const h_rgbaImage, 
                            uchar4 * const d_rgbaImage,
                            unsigned char* const d_greyImage, 
                            size_t numRows, size_t numCols);

//include the definitions of the above functions for this homework
#include "HW1.cpp"


void TestSpeed () {
  //the size is larger than 557*313
  //but it spend less time
  static int x[313][557];
  static int y[313][557];
  srand(time(0));
  for (int i = 0; i < 313; ++ i)
    for (int j = 0; j < 557; ++ j) {
      x[i][j] = rand();
    }
  float t = clock();
  for (int i = 0; i < 313; ++ i)
    for (int j = 0; j < 557; ++ j) {
      y[i][j] = x[i][j] * 0.3 + x[i][j]*0.2 + x[i][j] * 0.5;
    }
  printf("the normal array  test speed -> the time is : %lf ms\n", (clock()-t)/ CLOCKS_PER_SEC * 1000);

}

int main(int argc, char **argv) {
  TestSpeed();
  uchar4        *h_rgbaImage, *d_rgbaImage;
  unsigned char *h_greyImage, *d_greyImage;

  std::string input_file;
  std::string output_file;
  std::string reference_file;
  double perPixelError = 0.0;
  double globalError   = 0.0;
  bool useEpsCheck = false;
  switch (argc)
  {
    case 2:
      input_file = std::string(argv[1]);
      output_file = "HW1_output.png";
      reference_file = "HW1_reference.png";
      break;
    case 3:
      input_file  = std::string(argv[1]);
      output_file = std::string(argv[2]);
      reference_file = "HW1_reference.png";
      break;
    case 4:
      input_file  = std::string(argv[1]);
      output_file = std::string(argv[2]);
      reference_file = std::string(argv[3]);
      break;
    case 6:
      useEpsCheck=true;
      input_file  = std::string(argv[1]);
      output_file = std::string(argv[2]);
      reference_file = std::string(argv[3]);
      perPixelError = atof(argv[4]);
      globalError   = atof(argv[5]);
      break;
    default:
      std::cerr << "Usage: ./HW1 input_file [output_filename] [reference_filename] [perPixelError] [globalError]" << std::endl;
      exit(1);
  }
  //load the image and give us our input and output pointers
  //h_rgbaImage  原始图像，保存在cpu上
  //h_greyImage,  分配好cpu内存的一段内存，貌似没啥用
  //d_rgbaImage    分配好gpu内存的一段空间，数值和cpu彩色图像是一致的
  //d_greyImage   分配好内存的gpu内存
  preProcess(&h_rgbaImage, &h_greyImage, &d_rgbaImage, &d_greyImage, input_file);

  GpuTimer timer;
  timer.Start();
  //call the students' code
  your_rgba_to_greyscale(h_rgbaImage, d_rgbaImage, d_greyImage, numRows(), numCols());
  timer.Stop();
  cudaDeviceSynchronize(); checkCudaErrors(cudaGetLastError());

  int err = printf("Your gpu code ran in: %f msecs.\n", timer.Elapsed());

  if (err < 0) {
    //Couldn't print! Probably the student closed stdout - bad news
    std::cerr << "Couldn't print timing information! STDOUT Closed!" << std::endl;
    exit(1);
  }

  size_t numPixels = numRows()*numCols();
  //把gpu算出的灰度图像转移到cpu内存的灰度图像上

  timer.Start();
  checkCudaErrors(cudaMemcpy(h_greyImage, d_greyImage, sizeof(unsigned char) * numPixels, cudaMemcpyDeviceToHost));
  timer.Stop();
  printf("the time of copy memory from gpu to cpu: %f msecs.\n", timer.Elapsed());
  //check results and output the grey image
  postProcess(output_file, h_greyImage);

  timer.Start();
  referenceCalculation(h_rgbaImage, h_greyImage, numRows(), numCols());
  timer.Stop();
  printf("the cpu code ran in: %f msecs.\n", timer.Elapsed());

  postProcess(reference_file, h_greyImage);

  //generateReferenceImage(input_file, reference_file);
  compareImages(reference_file, output_file, useEpsCheck, perPixelError,
                globalError);

  cleanup();

  return 0;
}
