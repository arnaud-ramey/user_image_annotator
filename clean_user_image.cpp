/*!
  \file        clean_user_image.cpp
  \author      Arnaud Ramey <arnaud.a.ramey@gmail.com>
                -- Robotics Lab, University Carlos III of Madrid
  \date        2014/11/15

________________________________________________________________________________

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

\todo Description of the file
 */
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "contour_image_annotator.h"

void dilate_mask(const cv::Mat3b & img, cv::Scalar color,
                 cv::Mat1b & mask_buffer, const unsigned int kernel_size = 10) {
  unsigned int rows = img.rows, cols = img.cols;
  mask_buffer.create(img.size());
  mask_buffer.setTo(0);
  cv::Vec3b color3b(color[0], color[1], color[2]);
  for (int row = 0; row < rows; ++row) {
    const cv::Vec3b*row_ptr = img.ptr<cv::Vec3b>(row);
    uchar* mask_buffer_ptr = mask_buffer.ptr<uchar>(row);
    for (int col = 0; col < cols; ++col) {
      if (row_ptr[col] == color3b)
        mask_buffer_ptr[col] = 255;
    } // end loop col
  } // end loop row
  //cv::imshow("mask_buffer", mask_buffer); cv::waitKey(0);
  cv::morphologyEx(mask_buffer, mask_buffer, cv::MORPH_OPEN, 
                   cv::Mat(kernel_size, kernel_size, CV_8U, 255));
  //cv::imshow("mask_buffer_morph", mask_buffer); cv::waitKey(0);
} // end dilate()

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
  cv::Mat3b user_img, user_img_cleaned;
  cv::Mat1b mask_buffer;
  std::vector<int> params;
  params.push_back(CV_IMWRITE_PNG_COMPRESSION);
  params.push_back(9);
  
  for (unsigned int i = 1; i < argc; ++i) {
    std::string filename = std::string(argv[i]);
    bool success = false;
    try {
      user_img = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
      success = !user_img.empty();
    }
    catch (cv::Exception e) {
      printf("load_current_user_image(): exception '%s'\n", e.what());
    }
    if (!success) {
      printf("load_current_user_image(): could not load '%s'\n", 
             filename.c_str());
      continue;
    }
    // make region grow
    user_img_cleaned.create(user_img.size());
    user_img_cleaned.setTo(cv::Scalar::all(0));
    for(unsigned j = 0; j < NCOLORS ; ++j)  {
      dilate_mask(user_img, USER_COLOR[j], mask_buffer);
      user_img_cleaned.setTo(USER_COLOR[j], mask_buffer);
    }
    cv::imshow("user_img", user_img); 
    cv::imshow("user_img_cleaned", user_img_cleaned); cv::waitKey(10);
    std::string filename_out = remove_filename_extension(filename) + "_cleaned.png";
    printf("Saving file '%s'\n", filename_out.c_str());
    cv::imwrite(filename_out, user_img_cleaned, params);
  } // end loop i
}
