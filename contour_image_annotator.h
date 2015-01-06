/*!
  \file        contour_image_annotator.h
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

#ifndef CONTOUR_IMAGE_ANNOTATOR_H
#define CONTOUR_IMAGE_ANNOTATOR_H

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include "depth_canny.h"
#include "cv_conversion_float_uchar.h"
#include "contour_image_annotator_path.h"
#if USE_IMAGEMAGICK_FOR_16_COLORS_CONVERSION
#include "convert_n_colors.h"
#endif


//#define DEBUG_PRINT(...)   {}
#define DEBUG_PRINT(...)   printf(__VA_ARGS__)

static const unsigned int NCOLORS = 13;
#if USE_PCL_FOR_GROUND_PLANE // add the 'compute ground' button
static const unsigned int NSTATIC_BUTTONS = 7;
#else
static const unsigned int NSTATIC_BUTTONS = 6;
#endif
static const unsigned int BUTTONWIDTH = 32, NBUTTONS = NSTATIC_BUTTONS + NCOLORS;
static const char* BUTTONS_NAMES[NSTATIC_BUTTONS] =
{"exit", "first", "prev", "next", "last", "clear"
#if USE_PCL_FOR_GROUND_PLANE
  , "ground"};
#else
};
#endif
static const cv::Scalar USER_COLOR [NCOLORS] = {
  cv::Scalar(0, 0, 0), // black = eraser
  cv::Scalar(0, 0, 255), cv::Scalar(0, 255, 0), cv::Scalar(255, 0, 0),

  cv::Scalar(255, 255, 0), cv::Scalar(255, 0, 255), cv::Scalar(0, 255, 255),

  cv::Scalar(255, 160, 0), cv::Scalar(160, 255, 0),
  cv::Scalar(255, 0, 160), cv::Scalar(160, 0, 255),
  cv::Scalar(0, 160, 255), cv::Scalar(0, 255, 160)
};

////////////////////////////////////////////////////////////////////////////////
/// from string_utils
////////////////////////////////////////////////////////////////////////////////

/*! find all the iterations of a pattern in a string and replace
 * them with another pattern
 * \param stringToReplace the string to change
 * \param pattern what we want to replace
 * \param patternReplacement what we replace with
 * \return the number of times we replaced pattern
 */
inline int find_and_replace(std::string& stringToReplace,
                            const std::string & pattern,
                            const std::string & patternReplacement) {
  size_t j = 0;
  int nb_found_times = 0;
  for (; (j = stringToReplace.find(pattern, j)) != std::string::npos;) {
    //cout << "found " << pattern << endl;
    stringToReplace.replace(j, pattern.length(), patternReplacement);
    j += patternReplacement.length();
    ++ nb_found_times;
  }
  return nb_found_times;
}

////////////////////////////////////////////////////////////////////////////////

/*!
 * Remove the extension from a filename
 * \param path
 *  the full path
 * \example
 *  "/foo/bar" -> "/foo/bar"
 *  "/foo/bar.dat" -> "/foo/bar"
 *  "/foo.zim/bar.dat" -> "/foo.zim/bar"
 *  "/foo.zim/bar" -> "/foo.zim/bar"
 */
inline std::string remove_filename_extension
(const std::string & path) {
  std::string::size_type dot_pos = path.find_last_of('.');
  if (dot_pos == std::string::npos)
    return path;
  std::string::size_type slash_pos = path.find_last_of('/');
  if (slash_pos != std::string::npos && slash_pos > dot_pos) // dot before slash
    return path;
  return path.substr(0, dot_pos);
}

////////////////////////////////////////////////////////////////////////////////

class ContourImageAnnotator {
public:

  ContourImageAnnotator(const std::string & user_image_suffix = "_ground_truth_user") :
      WINNAME("ContourImageAnnotator"),
      _user_image_suffix(user_image_suffix)
  {
    DEBUG_PRINT("ctor\n");
    // declare window
    cv::namedWindow(WINNAME);
    cv::setMouseCallback(WINNAME, ContourImageAnnotator::win_cb, this);
    // create default images
    _user_image.create(480, 640); // rows, cols
    _user_image.setTo(cv::Scalar::all(0));
    _contours.create(_user_image.size());
    _contours.setTo(cv::Scalar::all(255));
    // load button images into _buttons
    _buttons.create(BUTTONWIDTH, NBUTTONS * BUTTONWIDTH); // rows, cols
    // static buttons
    for(int i = 0; i < NSTATIC_BUTTONS; ++i) {
      cv::Mat3b button_resized;
      std::ostringstream button_name;
      button_name << CONTOUR_IMAGE_ANNOTATOR_PATH << "icons/" << BUTTONS_NAMES[i] << ".png";
      DEBUG_PRINT("buttons_name:%s\n", button_name.str().c_str());
      cv::Mat3b button_src = cv::imread(button_name.str(), CV_LOAD_IMAGE_COLOR);
      if (button_src.empty())
        button_src.create(BUTTONWIDTH, BUTTONWIDTH);
      cv::resize(button_src, button_resized, cv::Size(BUTTONWIDTH, BUTTONWIDTH));
      // cv::imshow("button_resized", button_resized); cv::waitKey(0);
      cv::Mat3b button_roi_img = _buttons(button_roi(i));
      button_resized.copyTo(button_roi_img);
    }
    // user colors
    for(int i = 0; i < NCOLORS; ++i) {
      cv::Mat3b button_roi_img = _buttons(button_roi(NSTATIC_BUTTONS+i));
      button_roi_img.setTo(USER_COLOR[i]);
    }
    _selected_color = 1;
    // playlist
    _playlist_idx = 0;

    // cv::imshow("buttons", _buttons); cv::waitKey(0);
    _rgb_ok = false;
    redraw_final_window();
  } // end ctor

  //////////////////////////////////////////////////////////////////////////////

  inline bool load_playlist_images(const std::vector<std::string> & playlist) {
    DEBUG_PRINT("load_playlist_images(%i images)\n", playlist.size());
    if (playlist.empty()) {
      printf("Cannot load an empty playlist! Exiting.\n");
      quit(false);
    }
    _playlist = playlist;
    return goto_playlist_image(0, false);
  }

  //////////////////////////////////////////////////////////////////////////////

  inline bool goto_next_playlist_image() {
    unsigned int image_idx = (_playlist_idx + 1) % _playlist.size();
    return goto_playlist_image(image_idx);
  }
  inline bool goto_prev_playlist_image() {
    unsigned int image_idx = (_playlist_idx + _playlist.size() - 1) % _playlist.size();
    return goto_playlist_image(image_idx);
  }
  inline bool goto_playlist_image(unsigned int playlist_idx,
                                  bool save_before = true) {
    if (playlist_idx < 0 || playlist_idx >= _playlist.size())
      return false;
    if (save_before)
      save_current_user_image();
    _playlist_idx = playlist_idx;
    load_playlist_image(get_current_filename());
  }

  //////////////////////////////////////////////////////////////////////////////

  inline void run() {
    while(true) {
      cv::imshow(WINNAME, _final_window);
      char c = cv::waitKey(50);
      int i = (int) c;
      //DEBUG_PRINT("c:%c = %i\n", c, i);
      // 48->57 = 0->9 numbers on the topleft part keyboard (on top of QWERTY)
      if (i >= 48 && i <= 57)
        select_color(i - 48);
      // -80->-71 = 0->9 keypad
      else if (i >= -80 && i <= -71)
        select_color(i + 80);
      // french keyboard - topleft part keyboard (on top of AZERTY)
      else if (i == -32) select_color(0);
      else if (i == 38)  select_color(1);
      else if (i == -23) select_color(2);
      else if (i == 34)  select_color(3);
      else if (i == 39)  select_color(4);
      else if (i == 40)  select_color(5);
      else if (i == 45)  select_color(6);
      else if (i == -24) select_color(7);
      else if (i == 95)  select_color(8);
      else if (i == -25) select_color(9);
      else if (c == 'p' || c == 8) // || c == 81 || i == 85) // 81:left | 85:PageUp | 8:backspace
        goto_prev_playlist_image();
      else if (c == 'n' || c == ' ') // || c == 83 || i == 86) // 83:right | 86:PageDown
        goto_next_playlist_image();
      else if (c == 'c')
        clear_user_image();
      else if (c == 27 || c == 'q') {
        quit();
        break;
      }
      else custom_key_handler(c);
    }
  } // end run()

  //////////////////////////////////////////////////////////////////////////////

protected:

  inline virtual bool load_playlist_image(const std::string & filename) {
    DEBUG_PRINT("load_playlist_image('%s')\n", filename.c_str());
    cv::Mat1b contour = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
    if (contour.empty())
      return false;
    load_current_user_image();
    return set_images(_user_image, contour);
  }
  inline std::string get_current_filename() const {
    return _playlist[_playlist_idx];
  }
  inline std::string get_current_user_filename() const {
    return remove_filename_extension(get_current_filename()) + _user_image_suffix + ".png";
  }

  //////////////////////////////////////////////////////////////////////////////

  bool set_images(const cv::Mat3b & user_image,
                  const cv::Mat1b & contours) {
    DEBUG_PRINT("set_images(user_iage:%ix%i, contours:%ix%i)\n",
                user_image.cols, user_image.rows, contours.cols, contours.rows);
    if (contours.empty()) {
      printf("Cannot set an empty contour image!\n");
      return false;
    }
    user_image.copyTo(_user_image);
    contours.copyTo(_contours);
    cv::threshold(_contours, _contours, 128, 255, CV_THRESH_BINARY);
    // resize user image to contour if needed
    if (contours.size() != _user_image.size())
      cv::resize(_user_image, _user_image, contours.size());
    redraw_final_window();
    return true;
  } // end set_images()

  //////////////////////////////////////////////////////////////////////////////

  inline bool load_current_user_image() {
    std::string filename = get_current_user_filename();
    DEBUG_PRINT("load_current_user_image() : Loading file '%s'\n", filename.c_str());
    bool success = false;
    cv::Mat3b user_image;
    try {
      user_image = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
      success = !user_image.empty();
    }
    catch (cv::Exception e) {
      printf("load_current_user_image(): exception '%s'\n", e.what());
    }
    if (!success) {
      printf("load_current_user_image(): could not load '%s'\n",
             get_current_user_filename().c_str());
      _user_image.setTo(cv::Scalar::all(0)); // clear user image
      return false;
    }
    //cv::imshow("user_image", user_image); cv::waitKey(0);
    user_image.copyTo(_user_image);
    return true;
  } // end load_current_user_image()

  //////////////////////////////////////////////////////////////////////////////

  inline bool save_current_user_image() const {
    std::string filename = get_current_user_filename();
    DEBUG_PRINT("save_current_user_image() - Saving file '%s'\n", filename.c_str());
    if (!cv::imwrite(filename, _user_image))
      return false;
#if USE_IMAGEMAGICK_FOR_16_COLORS_CONVERSION
    if (!convert_n_colors(filename, 16, filename))
      return false;
#endif
    return true;
  } // end save_current_user_image()

  //////////////////////////////////////////////////////////////////////////////

  //! a key handler for children classes
  virtual void custom_key_handler(char c) { }
  virtual void custom_button_handler(const std::string button_name) { }

  //////////////////////////////////////////////////////////////////////////////

  inline void quit(bool want_save = true) {
    printf("The application will shut down now. Have a nice day.\n");
    if (want_save)
      save_current_user_image();
    exit(0);
  } // end exit()

  //////////////////////////////////////////////////////////////////////////////

  void select_color(const unsigned int color_idx) {
    if (color_idx < 0 || color_idx >= NCOLORS)
      return;
    _selected_color = color_idx;
    redraw_final_window();
  }

  //////////////////////////////////////////////////////////////////////////////

  void redraw_final_window() {
    DEBUG_PRINT("redraw_final_window()\n");
    // create the image
    int cols = std::max(_buttons.cols, _user_image.cols);
    int rows = _buttons.rows + _user_image.rows;
    _final_window.create(rows, cols);
    _final_window.setTo(cv::Scalar::all(128));
    // copy the buttons
    cv::Mat3b buttons_dst = _final_window(buttons_roi());
    _buttons.copyTo(buttons_dst);
    // select the color
    cv::Rect selected_color_roi = button_roi(NSTATIC_BUTTONS + _selected_color);
    cv::Scalar selection_color = CV_RGB(200, 200, 200);
    cv::rectangle(buttons_dst, selected_color_roi, selection_color, 2);
    cv::line(buttons_dst, selected_color_roi.br(), selected_color_roi.tl(), selection_color, 2);
    cv::line(buttons_dst,
             selected_color_roi.tl()+cv::Point(BUTTONWIDTH, 0),
             selected_color_roi.br()-cv::Point(BUTTONWIDTH, 0), selection_color, 2);
    // copy the user_image_
    cv::Mat3b user_image_dst = _final_window(user_image_roi());
    //user_image_dst.setTo(cv::Scalar::all(0));
    _user_image.copyTo(user_image_dst);
    if (_rgb_ok) {
      cv::cvtColor(_user_image, _user_image_mask, CV_BGR2GRAY);
      _rgb.copyTo(user_image_dst, _user_image_mask == 0);
    }
    // cv::addWeighted(user_image_dst, 1, _rgb, .5, 0, user_image_dst);
    //_user_image.copyTo(user_image_dst, _user_image != cv::Scalar::all(0));
    // use the contour image
    user_image_dst.setTo(cv::Scalar::all(100), _contours == 0);
  } // end redraw_final_window();

  //////////////////////////////////////////////////////////////////////////////

  static void win_cb(int event, int x, int y, int flags, void* cookie) {
    // DEBUG_PRINT("win_cb(%i, %i): event:%i, flag:%i\n", x, y, event, flags);
    if (event != CV_EVENT_LBUTTONDOWN
        && event != CV_EVENT_MBUTTONDOWN
        && event != CV_EVENT_RBUTTONDOWN
        && flags != 36) // middle button dragging
      return;
    ContourImageAnnotator* this_cb = ((ContourImageAnnotator*) cookie);
    // click on the image -> floodfill
    if (y > BUTTONWIDTH) {
      if (event == CV_EVENT_LBUTTONDOWN)
        this_cb->floodfill(x, y - BUTTONWIDTH);
      else if (event == CV_EVENT_MBUTTONDOWN || flags == 36) // middle button dragging
        this_cb->paint_contour(x, y - BUTTONWIDTH);
      else // right button
        this_cb->floodfill(x, y - BUTTONWIDTH, false, cv::Scalar::all(0));
      return;
    }

    // click on one of the buttons
    unsigned int button_idx = x / BUTTONWIDTH;
    //DEBUG_PRINT("win_cb(%i, %i): button #%i\n", x, y, button_idx);
    if (button_idx < 0 || button_idx >= NSTATIC_BUTTONS + NCOLORS)
      return;
    // color buttons
    if (button_idx >= NSTATIC_BUTTONS) {
      this_cb->select_color(button_idx - NSTATIC_BUTTONS);
      return;
    }
    // static buttons
    std::string button_name = BUTTONS_NAMES[button_idx];
    if (button_name == "exit")
      this_cb->quit();
    if (button_name == "next")
      this_cb->goto_next_playlist_image();
    else if (button_name == "prev")
      this_cb->goto_prev_playlist_image();
    else if (button_name == "first")
      this_cb->goto_playlist_image(0);
    else if (button_name == "last")
      this_cb->goto_playlist_image(this_cb->_playlist.size()-1);
    else if (button_name == "clear")
      this_cb->clear_user_image();
    else this_cb->custom_button_handler(button_name);
  } // end win_cb();

  //////////////////////////////////////////////////////////////////////////////

  void clear_user_image() {
    DEBUG_PRINT("clear_user_image()\n");
    load_playlist_image(get_current_filename()); // reload contour image
    _user_image.setTo(cv::Scalar::all(0));
    redraw_final_window();
  }

  //////////////////////////////////////////////////////////////////////////////

  void paint_contour(int x, int y, int radius = 3, cv::Scalar color = cv::Scalar::all(0)) {
    if (_contours(y, x) != 255) {
      printf("paint_contour(%i, %i) on edge! Doing nothing.\n", x, y);
      return;
    }
    DEBUG_PRINT("paint_contour(%i, %i)\n", x, y);
    cv::circle(_contours, cv::Point(x, y), radius, color, -1);
    redraw_final_window();
  }

  //////////////////////////////////////////////////////////////////////////////

  void floodfill(int x, int y, bool use_selected_color = true, cv::Scalar color = cv::Scalar()) {
    if (y < 0 || y >= _user_image.rows || x < 0 || x >= _user_image.cols) {
      printf("floodfill(%i, %i) out of bounds! Doing nothing.\n", x, y);
      return;
    }
    if (_contours(y, x) != 255) {
      printf("floodfill(%i, %i) on edge! Doing nothing.\n", x, y);
      return;
    }
    DEBUG_PRINT("floodfill(%i, %i)\n", x, y);
    // use a buffer image to get the floodfilled area
    if (use_selected_color)
      color = USER_COLOR[_selected_color];
    _contours.copyTo(_contours_clone);
    //cv::imshow("contours_clone", _contours_clone); cv::waitKey(0);
    cv::floodFill(_contours_clone, cv::Point(x, y), cv::Scalar::all(127));
    _user_image.setTo(color, _contours_clone == 127);
    redraw_final_window();
  }

  //////////////////////////////////////////////////////////////////////////////

  inline cv::Rect buttons_roi() const {
    return cv::Rect (0, 0, _buttons.cols, _buttons.rows);
  }
  inline cv::Rect button_roi(unsigned int i) const {
    return cv::Rect (i * BUTTONWIDTH, 0, BUTTONWIDTH, BUTTONWIDTH);
  }
  inline cv::Rect user_image_roi() const {
    return cv::Rect (0, _buttons.rows,
                     _user_image.cols, _user_image.rows);
  }

  //////////////////////////////////////////////////////////////////////////////

  cv::Mat3b _user_image; // does not include contours
  cv::Mat1b _user_image_mask;
  cv::Mat1b _contours, _contours_clone;
  cv::Mat3b _buttons;
  cv::Mat _rgb;
  bool _rgb_ok;
  cv::Mat3b _final_window;
  std::string WINNAME;
  unsigned int _selected_color;
  std::string _user_image_suffix;
  // playlist
  std::vector<std::string> _playlist;
  unsigned int _playlist_idx;
}; // en class ContourImageAnnotator

#endif // CONTOUR_IMAGE_ANNOTATOR_H
