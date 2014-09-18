opencv_dollar_detector
======================

This is a work-in-progress port of the Dóllar pedestrian detector to OpenCV in C++





To Do List:
======================

Current Total: 13

Detector.cpp:
	acfDetect:
		line 130: test why contents of chns are different sometimes
	nmsMax:
		line 248: sort bounding boxes
		line 253: add test for greediness
		line 256: test if result[j] was discarded
		line 291: discard the bounding box
		line 296: adjust result

ColorChannel.cpp:
	OK!

GradientMagnitudeChannel.cpp:
	OK!
	
QuantizedGradientChannel.cpp:
	OK!

Pyramid.cpp:
	computeMultiScaleChannelFeaturePyramid:
		investigate why convolution is not blurring images the same way as matlab is
		change literal number of channels to nChannels whenever needed
		line 113: the whole for statement needs work, write a substitution for is=is(2:3) 
		line 140: compute lambdas
		line 189: fix approximated (resampled) scales of H
	computeSingleScaleChannelFeatures:
		line 247: possibly add computation of custom channels

pNms.cpp:
	check if it is necessary!

utils.cpp:
	OK!
