/*!
  \file        user_image_annotator.cpp
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
#include <contour_image_annotator.h>
#if USE_PCL_FOR_GROUND_PLANE
#include <ground_plane_finder.h>
#endif // USE_PCL_FOR_GROUND_PLANE
#define TRACK_BAR_SCALE_FACTOR 25.f

class UserImageAnnotator : public ContourImageAnnotator {
public:
  UserImageAnnotator() {
    canny_tb1_value = DepthCanny::DEFAULT_CANNY_THRES1 * TRACK_BAR_SCALE_FACTOR;
    canny_tb2_value = DepthCanny::DEFAULT_CANNY_THRES2 * TRACK_BAR_SCALE_FACTOR;
    cv::createTrackbar("canny_param1", WINNAME, &canny_tb1_value, 100,
                       &UserImageAnnotator::trackbar_cb, this);
    cv::createTrackbar("canny_param2", WINNAME, &canny_tb2_value, 100,
                       &UserImageAnnotator::trackbar_cb, this);
  }

protected:
  inline virtual bool load_playlist_image(const std::string & filename) {
    printf("UserImageAnnotator::load_playlist_image('%s')\n", filename.c_str());
    // remove _depth.png if needed
    // read depth and rgb
    image_utils::read_rgb_and_depth_image_from_image_file
        (filename, &_rgb, &_depth);
    if (_depth.empty())
      return false;
    _rgb_ok = (!_rgb.empty());
    //if (_rgb_ok) cv::imshow("rgb", _rgb);
    load_current_user_image();
    compute_canny();
  }

  //////////////////////////////////////////////////////////////////////////////

  //! apply Canny to get contour
  bool compute_canny() {
    canny_param1 = 1.f *canny_tb1_value / TRACK_BAR_SCALE_FACTOR;
    canny_param2 = 1.f *canny_tb2_value / TRACK_BAR_SCALE_FACTOR;
    _canny.set_canny_thresholds(canny_param1, canny_param2);
    _canny.thresh(_depth);
    _canny.get_thresholded_image().copyTo(_contour);
    // use in interface
    return set_images(_user_image, _contour);
  }

  //////////////////////////////////////////////////////////////////////////////

  void compute_ground_plane() {
#if USE_PCL_FOR_GROUND_PLANE
    printf("Computing ground plane...\n");
    goto_playlist_image(_playlist_idx);
    _finder.compute_plane(_depth, GroundPlaneFinder::DEFAULT_DISTANCE_THRESHOLD_M,
                          .2);
    _finder.to_img(_depth, _plane, -1, -1, 0.1);
    _contour.setTo(0, _plane);
    set_images(_user_image, _contour);
    //cv::imshow("plane", _plane); cv::waitKey(0);
#else // no PCL
    printf("The project was compiled without PCL. Cannot compute ground plane.\n");
#endif // USE_PCL_FOR_GROUND_PLANE
  }

  //////////////////////////////////////////////////////////////////////////////

  virtual void custom_key_handler(char c) {
    if (c == 'g') compute_ground_plane();
  } // end custom_key_handler()

  //////////////////////////////////////////////////////////////////////////////

  virtual void custom_button_handler(const std::string button_name) {
    if (button_name == "ground") compute_ground_plane();
  }

  //////////////////////////////////////////////////////////////////////////////

  static void trackbar_cb(int pos, void* cookie) {
    ((UserImageAnnotator*) cookie)->compute_canny();
  }

protected:
  cv::Mat _depth;
  cv::Mat1b _contour, _plane;
#if USE_PCL_FOR_GROUND_PLANE
  GroundPlaneFinder _finder;
#endif // USE_PCL_FOR_GROUND_PLANE
  // edge detection
  double canny_param1, canny_param2;
  int canny_tb1_value, canny_tb2_value;
  DepthCanny _canny;
}; // end class UserImageAnnotator

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
  std::vector<std::string> filenames;
  for (unsigned int i = 1; i < argc; ++i) {
    std::string filename_clean(argv[i]);
    find_and_replace(filename_clean, "_depth.png", "");
    find_and_replace(filename_clean, "_rgb.png", "");
    filenames.push_back(filename_clean);
  }
  UserImageAnnotator annot;
  annot.load_playlist_images(filenames);
  annot.run();
}
