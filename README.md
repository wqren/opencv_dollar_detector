opencv_dollar_detector
======================

This is a work-in-progress port of the Dóllar pedestrian detector to OpenCV in C++ by Charles Arnoud, under the mentorship of Cláudio Rosito Jüng and with the help of Gustavo Führ.  


Current Status  
======================  

Detection working, but has two memory leaks and is too slow. 


To Do List:  
======================  

Current Total: 11  

Opencv_Dollar_Detector.cpp:  
&nbsp;&nbsp;&nbsp;&nbsp;find out why the program is so slow!  
&nbsp;&nbsp;&nbsp;&nbsp;maybe change the way time is being calculated  
&nbsp;&nbsp;&nbsp;&nbsp;save detections to .txt file  

Detector.cpp:  
&nbsp;&nbsp;&nbsp;&nbsp;nonMaximalSuppression:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;possibly relocate suppression to other class  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;add the other two types of suppression    

ColorChannel.cpp:  
&nbsp;&nbsp;&nbsp;&nbsp;OK!  

GradientMagnitudeChannel.cpp:  
&nbsp;&nbsp;&nbsp;&nbsp;check if variable d is correct inside mGradMag  
&nbsp;&nbsp;&nbsp;&nbsp;check if mGradMag is not modifying the input  

QuantizedGradientChannel.cpp:  
&nbsp;&nbsp;&nbsp;&nbsp;OK!  

Pyramid.cpp:  
&nbsp;&nbsp;&nbsp;&nbsp;computeMultiScaleChannelFeaturePyramid:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;add calculation of lambdas  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;find sources of two memory leaks  
&nbsp;&nbsp;&nbsp;&nbsp;computeSingleScaleChannelFeatures:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;possibly add computation of custom channels  

Pyramid.cpp:  
&nbsp;&nbsp;&nbsp;&nbsp;decide what to do with the channelTypes variable   

utils.cpp:  
&nbsp;&nbsp;&nbsp;&nbsp;OK!  
