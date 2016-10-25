                  +----------------------+
                  | user_image_annotator |
                  +----------------------+

[![Build Status](https://travis-ci.org/arnaud-ramey/user_image_annotator.svg)](https://travis-ci.org/arnaud-ramey/user_image_annotator)

A series of tools for annotating images using floodfill and indexed images.

License :                  see the LICENSE file.
Authors :                  see the AUTHORS file.
How to build the program:  see the INSTALL file.

A first tool, called "contour_image_annotator",
is made for annotating binary contour images.
is particuarly handy for annotating
images where the contour image has already been generated, for instance
with an edge detector.
It generates annotated images with a given prefix,
by default "<input image>_user_illus.png"

Another tool, called "user_image_annotator",
is an extension of the "contour_image_annotator" for depth images:
it first generates the contour image from the depth image using edge detection
(namely, Canny filters).
For instance, this tool can ease the annnotation of ground truth user positions
in each frame of a data recording.
It can use the RGB image for display.

________________________________________________________________________________

How to use the program
________________________________________________________________________________
To display the help, just launch the program in a terminal.
It will display the help of the program.

== Synopsis ==

$ contour_image_annotator CONTOURIMAGES

where CONTOURIMAGES is the list of binary contour images.

$ user_image_annotator PREFIXIMAGES

where PREFIXIMAGES is a file or list of files
of depth images that can be accessed using a depth-to-uchar technique.
If RGB images are available, they will be shown in the GUI.
Note that "_depth.png" and "_rgb.png" are automatically
removed from PREFIXIMAGES to obtain prefixes.

== Keyboard shortcuts ==
For both "contour_image_annotator" and "user_image_annotator":
* 0 -> 9 keypad       select color 0 -> 9
* 'p', BackSpace      go to previous image
* 'n', Space          go to next image
* 'c'                 clear user image
* 'q', Esc            quit

For "user_image_annotator", if USE_PCL_FOR_GROUND_PLANE:
* 'g'                 compute ground plane using PCL


== Examples ==
* To annotate images where the contour images has already been generated
by another method:
$ contour_image_annotator contour*.png

* To generate contour images from coupled rgb+depth images and annotate them:
$ user_image_annotator *rgb.png

If we want to annotate the following files:
  samples/sample1_depth.png
  samples/sample1_depth_params.yaml
  samples/sample1_rgb.png
  samples/sample2_depth.png
  samples/sample2_depth_params.yaml
  samples/sample2_rgb.png
$ user_image_annotator samples/*rgb.png

________________________________________________________________________________

Samples
________________________________________________________________________________

Using the sample images given with the tools:
$ contour_image_annotator ../samples/sample?.*
$ user_image_annotator samples/*rgb.png
