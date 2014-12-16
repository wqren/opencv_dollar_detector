#include "Detector.h"

// i dont know if its gonna be needed but this is start
void Detector::exportDetectorModel(cv::String fileName)
{
	cv::FileStorage xml;
	
	xml.open(fileName, cv::FileStorage::WRITE);

	xml << "opts" << "{";
		xml << "pPyramid" << "{";
			xml << "pChns" << "{";
				xml << "shrink" << opts.pPyramid.pChns.shrink;
				xml << "pColor" << "{";
					xml << "enabled" << opts.pPyramid.pChns.pColor.enabled;
				xml << "}";
			xml << "}";
		xml << "}";
	xml << "stride" << opts.stride;
	xml << "}";
	
	//xml << "clf" << this->clf;

	xml.release();
}

// reads the detector model from the xml model
// for now, it must be like this since the current model was not written by this program 
// this will change after we are set on a class structure
void Detector::importDetectorModel(cv::String fileName)
{
	cv::FileStorage xml;

	xml.open(fileName, cv::FileStorage::READ);

	if (!xml.isOpened())
	{
		std::cerr << " # Failed to open " << fileName << std::endl;
	}
	else
	{
		opts.readOptions(xml["detector"]["opts"]);
	
		xml["detector"]["clf"]["fids"] >> fids;
		xml["detector"]["clf"]["child"] >> child;
		xml["detector"]["clf"]["thrs"] >> thrs;
		xml["detector"]["clf"]["hs"] >> hs;
		xml["detector"]["clf"]["weights"] >> weights;
		xml["detector"]["clf"]["depth"] >> depth;
		xml["detector"]["clf"]["errs"] >> errs;
		xml["detector"]["clf"]["losses"] >> losses;		
		xml["detector"]["clf"]["treeDepth"] >> treeDepth;	

		timeSpentInDetection = 0;

		xml.release();
	}
}

void showDetections(cv::Mat I, BB_Array detections, cv::String windowName)
{
	cv::Mat img = I.clone();
	for (int j = 0; j<detections.size(); j++) 
		detections[j].plot(img, cv::Scalar(0,255,0));

	cv::imshow(windowName, img);
}

void printDetections(BB_Array detections, int frameIndex)
{
	std::cout << "Detections in frame " << frameIndex << ":\n";
	for (int i=0; i < detections.size(); i++)
		std::cout << detections[i].toString(frameIndex) << std::endl;
	std::cout << std::endl;
}

// this procedure was just copied verbatim
inline void getChild(float *chns1, uint32 *cids, uint32 *fids, float *thrs, uint32 offset, uint32 &k0, uint32 &k)
{

  /*std::cout << "fids " << fids[k] << std::endl;
  std::cout << "cids com fids " << cids[fids[k]] << std::endl;
  std::cout << "ftr " << chns1[cids[fids[k]]] << std::endl;*/

  float ftr = chns1[cids[fids[k]]];
  k = (ftr<thrs[k]) ? 1 : 2;
  k0=k+=k0*2; k+=offset;
}




BB_Array Detector::generateCandidates(int imageHeight, int imageWidth, cv::Mat_<float> &P, 
							float meanHeight/* = 1.7m*/, float stdHeight/* = 0.1m*/, float factorStdHeight/* = 2.0*/) 
{

	// there is a set of parameters here that are hard coded, but should
	// be read from a file or something...
	cv::Mat_<float> area(2,2);
	area(0,0) = -30000;
	area(0,1) =  30000;
	area(1,0) = -30000;
	area(1,1) =  30000;

	float step = 400;
	float aspectRatio = 0.5; // same as model

	float stepHeight = 100;

	float minImageHeight = 50;


	BB_Array candidates;
	for (float x = area(0,0); x < area(0,1); x += step) {
		for (float y = area(1,0); y < area(1,1); y += step) {
			// for all points in the ground plane (according to the area), I try all
			// the heights that I need to

			for (float h = -stdHeight * factorStdHeight; h <= stdHeight * factorStdHeight; h+= stepHeight) {
				float wHeight = meanHeight + h;

				cv::Point2f wPoint(x, y);
				BoundingBox bb = wcoord2bbox(wPoint, P, wHeight, aspectRatio);

				// only put if it is inside the visible image
				if (bb.topLeftPoint.x >= 0 && bb.topLeftPoint.x+bb.width < imageWidth && 
					    bb.topLeftPoint.y >= 0 && bb.topLeftPoint.y+bb.height < imageHeight &&
					    bb.height >= minImageHeight) {
					candidates.push_back(bb);
				}
			}
			
		}
	}

	return candidates;
}

/*
    This function return the scale in which the pyramid will be more fitted
*/
int Detector::findClosestScaleFromBbox(std::vector<Info> &pyramid, BoundingBox &bb,
												int modelHeight, int imageHeight)
{

	// TODO: make a vector with ranges of scales!

	//std::cout << "height of the bb" << bb.height << std::endl;

	//TODO: should consider the shrink size
	//float shrinkedModelHeight = (float)modelHeight / 4.0;


	// actually here is the size of the the image that changes, the model stays the same
	// to see the best fit for the bounding box, one must find the relation between the original
	// and then find the closest to the size of the bounding box
	float min_dist = imageHeight;
	int i_min = -1;

	for (int i = 0; i < opts.pPyramid.computedScales; i++) {




		// size of the downsample image
		int height = pyramid[i].image.rows;
		int width = pyramid[i].image.cols;

				

		// relation between the the image and the downsampled version of it
		float factor_height = (float)imageHeight/(height*4.0);
		// float factor_width  = imageWidth/(float)width;

		float height_model = modelHeight*factor_height;


		//std::cout << height_model << std::endl;

		float diff = fabs(height_model - bb.height);

		if (diff < min_dist) {
			i_min = i;
			min_dist = diff;
		}

	}
	//std::cout << i_min << std::endl;

	//exit(42);

	return i_min;



}

void Detector::bbTopLeft2PyramidRowColumn(int *r, int *c, BoundingBox &bb, int modelHt, int modelWd, int ith_scale, int stride) {


	double s1, s2, sw, sh, tlx, tly;

	s1 = (modelHt-double(opts.modelDs[0]))/2-opts.pPyramid.pad[0];
	s2 = (modelWd-double(opts.modelDs[1]))/2-opts.pPyramid.pad[1];

	sw = opts.pPyramid.scales_w[ith_scale];
	sh = opts.pPyramid.scales_h[ith_scale];

	tlx = (double)bb.topLeftPoint.x;
	tly = (double)bb.topLeftPoint.y;

	double fc = (sw*tlx - s2)/(double)stride;
	double fr = (sh*tly - s1)/(double)stride;

	*r = (int)fr;
	*c = (int)fc;
}


BoundingBox Detector::pyramidRowColumn2BoundingBox(int r, int c,  int modelHt, int modelWd, int ith_scale, int stride) {

	double shift[2];
	shift[0] = (modelHt-double(opts.modelDs[0]))/2-opts.pPyramid.pad[0];
	shift[1] = (modelWd-double(opts.modelDs[1]))/2-opts.pPyramid.pad[1];

	BoundingBox bb;
	bb.topLeftPoint.x = c*stride;
	bb.topLeftPoint.x = (bb.topLeftPoint.x+shift[1])/opts.pPyramid.scales_w[ith_scale];
	bb.topLeftPoint.y = r*stride;
	bb.topLeftPoint.y = (bb.topLeftPoint.y+shift[0])/opts.pPyramid.scales_h[ith_scale];
	bb.height = opts.modelDs[0]/opts.pPyramid.scales[ith_scale];
	bb.width = opts.modelDs[1]/opts.pPyramid.scales[ith_scale];
	bb.scale = ith_scale;

	return bb;
}


BB_Array Detector::applyDetectorToFrameSmart(std::vector<Info> pyramid, int shrink, int modelHt, int modelWd, int stride, float cascThr, float *thrs, float *hs, 
										uint32 *fids, uint32 *child, int nTreeNodes, int nTrees, int treeDepth, int nChns, int imageWidth, int imageHeight, cv::Mat_<float> &P, cv::Mat &debug_image)
{
	BB_Array result;


	// std::cout << "stride: " << stride << std::endl;
	// std::cout << "shrink: " << shrink << std::endl;


	BB_Array bbox_candidates = generateCandidates(imageHeight, imageWidth, P);
	std::cout << "Number of candidates: " << bbox_candidates.size() << std::endl;

	// create one candidate only for debug
	// BB_Array bbox_candidates;
	// BoundingBox bb;
	// bb.topLeftPoint.x = 486;
	// bb.topLeftPoint.y = 148;
	// bb.width = 57;
	// bb.height = 104;
	// bbox_candidates.push_back(bb);


	// plot the candidates, only for DEBUG
	// cv::waitKey(1000);
	// for (int i = 0; i < bbox_candidates.size(); ++i) {
		
	// 	std::cout << bbox_candidates[i].topLeftPoint << std::endl;
	//  	bbox_candidates[i].plot(debug_image, cv::Scalar(0, 255, 0));
	//  	cv::imshow("candidates", debug_image);
	//  	cv::waitKey(0);
	// }
	//cv::waitKey(40);

	std::vector<float*> scales_chns(opts.pPyramid.computedScales, NULL);
	std::vector<uint32*> scales_cids(opts.pPyramid.computedScales, NULL);

	// pre-compute the way we access the features for each scale
	for (int i=0; i < opts.pPyramid.computedScales; i++) {
		int height = pyramid[i].image.rows;
		int width = pyramid[i].image.cols;

		int channels = opts.pPyramid.pChns.pColor.nChannels + opts.pPyramid.pChns.pGradMag.nChannels + opts.pPyramid.pChns.pGradHist.nChannels;
		float* chns = (float*)malloc(height*width*channels*sizeof(float));
		features2floatArray(pyramid[i], chns, height, width,  opts.pPyramid.pChns.pColor.nChannels, opts.pPyramid.pChns.pGradMag.nChannels, opts.pPyramid.pChns.pGradHist.nChannels);
		scales_chns[i] = chns;


		int nFtrs = modelHt/shrink*modelWd/shrink*nChns;
	  	uint32 *cids = new uint32[nFtrs]; int m=0;
	  	/*for( int z=0; z<nChns; z++ ) {
	    	cids[m++] = z*width*height + c*height + r;
	    	std::cout << "cid " << cids[m-1] << std::endl;
	    }*/
	    for( int z=0; z<nChns; z++ ) {
	    	for( int cc=0; cc<modelWd/shrink; cc++ ) {
	      		for( int rr=0; rr<modelHt/shrink; rr++ ) {
	        		cids[m++] = z*width*height + cc*height + rr;
	        		// std::cout << "cids[m] " << cids[m-1] << std::endl;
	        	}
	        }
	    }

	    scales_cids[i] = cids;
	}








	float max_h = -1000;

	for (int i = 0; i < bbox_candidates.size(); ++i) {
		std::cout << i << std::endl;

		// see which scale is best suited to the candidate
		int ith_scale = findClosestScaleFromBbox(pyramid, bbox_candidates[i], modelHt, imageHeight);

		//ith_scale = 0;

		int height = pyramid[ith_scale].image.rows;
		int width = pyramid[ith_scale].image.cols;
		
		//bbox_candidates[i].plot(debug_image, cv::Scalar(0, 255, 0));
		

		// for debug got to r and c and then come back
		// std::cout << "Original bbox " << bbox_candidates[i].topLeftPoint.x << " " << bbox_candidates[i].topLeftPoint.y << " size: " 
		// 	<< bbox_candidates[i].width << " x " << bbox_candidates[i].height << std::endl;
		// int row, col;
		// bbTopLeft2PyramidRowColumn(&row, &col, bbox_candidates[i], modelHt, modelWd, ith_scale, stride);
		// std::cout << "r " << row << " c " << col << std::endl;
		// BoundingBox testbb = pyramidRowColumn2BoundingBox(row, col, modelHt, modelWd, ith_scale, stride);
		// std::cout << "New bbox " << testbb.topLeftPoint.x << " " << testbb.topLeftPoint.y << " size: " 
		// 	<< testbb.width << " x " << testbb.height << std::endl;


		//testbb.plot(debug_image, cv::Scalar(255, 0, 0));

		//ith_scale = opts.pPyramid.computedScales - 1;

	// for debug got to r and c and then come back
		// std::cout << "Original bbox " << bbox_candidates[i].topLeftPoint.x << " " << bbox_candidates[i].topLeftPoint.y << " size: " 
		// 	<< bbox_candidates[i].width << " x " << bbox_candidates[i].height << std::endl;
		// bbTopLeft2PyramidRowColumn(&row, &col, bbox_candidates[i], modelHt, modelWd, ith_scale, stride);
		// std::cout << "r " << row << " c " << col << std::endl;
		// testbb = pyramidRowColumn2BoundingBox(row, col, modelHt, modelWd, ith_scale, stride);
		// std::cout << "New bbox " << testbb.topLeftPoint.x << " " << testbb.topLeftPoint.y << " size: " 
		// 	<< testbb.width << " x " << testbb.height << std::endl;


		// testbb.plot(debug_image, cv::Scalar(0, 0, 255));


		// cv::imshow("candidates", debug_image);
		// cv::waitKey(0);
		//exit(42);	

		
		//std::cout << ith_scale << std::endl;

		// r and c are defined by the candidate itself
		int r, c;
		bbTopLeft2PyramidRowColumn(&r, &c, bbox_candidates[i], modelHt, modelWd, ith_scale, stride);

		// std::cout << "r: " << r << std::endl;
		// std::cout << "c: " << c << std::endl; 
		// std::cout << "chns " << *chns << std::endl;


		float h=0, *chns1=scales_chns[ith_scale]+(r*stride/shrink) + (c*stride/shrink)*height;
	    
	    if( treeDepth==1 ) {
	      // specialized case for treeDepth==1
	      for( int t = 0; t < nTrees; t++ ) {
	        uint32 offset=t*nTreeNodes, k=offset, k0=0;
	        getChild(chns1,scales_cids[ith_scale],fids,thrs,offset,k0,k);
	        h += hs[k]; if( h<=cascThr ) break;
	      }
	    } else if( treeDepth==2 ) {
	      // specialized case for treeDepth==2
	      for( int t = 0; t < nTrees; t++ ) {
	        uint32 offset=t*nTreeNodes, k=offset, k0=0;

	        getChild(chns1,scales_cids[ith_scale],fids,thrs,offset,k0,k);
	        getChild(chns1,scales_cids[ith_scale],fids,thrs,offset,k0,k);
	        
	        h += hs[k]; if( h<=cascThr ) break;
	      }
	    } else if( treeDepth>2) {
	      // specialized case for treeDepth>2
	      for( int t = 0; t < nTrees; t++ ) {
	        uint32 offset=t*nTreeNodes, k=offset, k0=0;
	        for( int i=0; i<treeDepth; i++ )
	          getChild(chns1,scales_cids[ith_scale],fids,thrs,offset,k0,k);
	        h += hs[k]; if( h<=cascThr ) break;
	      }
	    } else {
	      // general case (variable tree depth)
	      for( int t = 0; t < nTrees; t++ ) {
	        uint32 offset=t*nTreeNodes, k=offset, k0=k;
	        while( child[k] ) {
	          float ftr = chns1[scales_cids[ith_scale][fids[k]]];
	          k = (ftr<thrs[k]) ? 1 : 0;
	          k0 = k = child[k0]-k+offset;
	        }
	        h += hs[k]; if( h<=cascThr ) break;
	      }
	    }

	    if (h > max_h) {
	    	max_h = h;
	    }


	    if(h>3){//cascThr) { 
			std::cout << h << std::endl;
			// std::cout << "hey" << std::endl;
			cv::imshow("results", debug_image);
	    	result.push_back(bbox_candidates[i]);
	    	bbox_candidates[i].plot(debug_image, cv::Scalar(0, 255, 0));
	    	//cv::waitKey(100);
	    }
	
	}

	std::cout << "max h: " << max_h << std::endl;
	std::cout << "cascThr: " << cascThr << std::endl;
	

	cv::waitKey(0);
}




BB_Array Detector::applyDetectorToFrame(std::vector<Info> pyramid, int shrink, int modelHt, int modelWd, int stride, float cascThr, float *thrs, float *hs, 
										uint32 *fids, uint32 *child, int nTreeNodes, int nTrees, int treeDepth, int nChns)
{
	BB_Array result;


	printf("Number of scales: %d\n", opts.pPyramid.computedScales);
	printf("Model size: %d x %d\n", modelWd, modelHt);
	// this became a simple loop because we will apply just one detector here, 
	// to apply multiple detector models you need to create multiple Detector objects. 
	float max_h = -10000;
	for (int i = 0; i < opts.pPyramid.computedScales; i++)
	{
		// in the original file: *chnsSize = mxGetDimensions(P.data{i});
		// const int height = (int) chnsSize[0];
  		// const int width = (int) chnsSize[1];
  		// const int nChns = mxGetNumberOfDimensions(prhs[0])<=2 ? 1 : (int) chnsSize[2];
		int height = pyramid[i].image.rows;
		int width = pyramid[i].image.cols;
		printf("Size of the downsampled image: %d x %d\n", width, height);
		printf("Aspect ratio of the image: %f\n", height/(float)width);

		int height1 = (int)ceil(float(height*shrink-modelHt+1)/stride);
		int width1 = (int)ceil(float(width*shrink-modelWd+1)/stride);

		int channels = opts.pPyramid.pChns.pColor.nChannels + opts.pPyramid.pChns.pGradMag.nChannels + opts.pPyramid.pChns.pGradHist.nChannels;
		float* chns = (float*)malloc(height*width*channels*sizeof(float));
		features2floatArray(pyramid[i], chns, height, width,  opts.pPyramid.pChns.pColor.nChannels, opts.pPyramid.pChns.pGradMag.nChannels, opts.pPyramid.pChns.pGradHist.nChannels);
		
		/*
		// debug: read chns from file
	  	cv::Mat scalei;
	  	std::string scaleName;

	  	scaleName += "scale";
	  	if (i < 9)
	  		scaleName += "0";
	  	std::ostringstream scaleNumber;
        scaleNumber << (i+1);
	  	scaleName += scaleNumber.str();

		xml["pyramid"][scaleName] >> scalei;
		//float* floatScale = cvImage2floatArray(scalei, 1);
		//printElements(floatScale, scalei.rows, scaleName + " read from xml file");
		free(chns);
		chns = (float*) malloc((ch1Size+ch2Size+ch3Size)*sizeof(float));
		chns = cvImage2floatArray(scalei, 1);
		// debug */

		
		// construct cids array
	  	int nFtrs = modelHt/shrink*modelWd/shrink*nChns;
	  	uint32 *cids = new uint32[nFtrs]; int m=0;
	  	for( int z=0; z<nChns; z++ ) {
	    	for( int c=0; c<modelWd/shrink; c++ ) {
	      		for( int r=0; r<modelHt/shrink; r++ ) {
	        		cids[m++] = z*width*height + c*height + r;
	        		// std::cout << "cids[m] " << cids[m-1] << std::endl;
	        	}
	        }
	    }

		/*
		// debug: prints values of several variables, all of these return correct results
		// shrink=4, modelHt=128, modelWd=64, stride=4, cascThr=-1.000000, treeDepth=2
		// height=152, width=186, nChns=10, nTreeNodes=7, nTrees=2048, height1=121, width1=171, nFtrs=5120
		std::cout << "shrink=" << shrink << ", modelHt=" << modelHt << ", modelWd=" << modelWd << ", stride=" << stride << ", cascThr=" << cascThr << ", treeDepth=" << treeDepth <<  ", modelDs=(" <<
			opts.modelDs[0] << "," << opts.modelDs[1] << ")" << std::endl;
		std::cout << "height=" << height << ", width=" << width << ", nChns=" << nChns <<  ", nTreeNodes=" << nTreeNodes << ", nTrees=" << nTrees << ", height1=" << height1 << 
			", width1=" << width1 << ", nFtrs=" << nFtrs << std::endl;
		// debug */

		// apply classifier to each patch
  		std::vector<int> rs, cs; std::vector<float> hs1;
  		for( int c=0; c<width1; c++ ) 
  		{
  			for( int r=0; r<height1; r++ ) 
  			{
  				bool debug = false;
  				if (r==37 && c==121) {
  					//debug = true;
  				}

			    float h=0, *chns1=chns+(r*stride/shrink) + (c*stride/shrink)*height;
			    if (debug) {
			    	std::cout << "*chns1 not smart " << *chns1 << std::endl;
			    }
			    if( treeDepth==1 ) {
			      // specialized case for treeDepth==1
			      for( int t = 0; t < nTrees; t++ ) {
			        uint32 offset=t*nTreeNodes, k=offset, k0=0;
			        getChild(chns1,cids,fids,thrs,offset,k0,k);
			        h += hs[k]; if( h<=cascThr ) break;
			      }
			    } else if( treeDepth==2 ) {
			      // specialized case for treeDepth==2
			      for( int t = 0; t < nTrees; t++ ) {
			        uint32 offset=t*nTreeNodes, k=offset, k0=0;
			        getChild(chns1,cids,fids,thrs,offset,k0,k);
			        getChild(chns1,cids,fids,thrs,offset,k0,k);
			        h += hs[k]; if( h<=cascThr ) break;
			      }
			    } else if( treeDepth>2) {
			      // specialized case for treeDepth>2
			      for( int t = 0; t < nTrees; t++ ) {
			        uint32 offset=t*nTreeNodes, k=offset, k0=0;
			        for( int i=0; i<treeDepth; i++ )
			          getChild(chns1,cids,fids,thrs,offset,k0,k);
			        h += hs[k]; if( h<=cascThr ) break;
			      }
			    } else {
			      // general case (variable tree depth)
			      for( int t = 0; t < nTrees; t++ ) {
			        uint32 offset=t*nTreeNodes, k=offset, k0=k;
			        while( child[k] ) {
			          float ftr = chns1[cids[fids[k]]];
			          k = (ftr<thrs[k]) ? 1 : 0;
			          k0 = k = child[k0]-k+offset;
			        }
			        h += hs[k]; if( h<=cascThr ) break;
			      }
		    }

		    if (h>max_h)
		    	max_h = h;

		    if(h>cascThr) { cs.push_back(c); rs.push_back(r); hs1.push_back(h); }

		    if (debug) {
  					double shift[2];
					shift[0] = (modelHt-double(opts.modelDs[0]))/2-opts.pPyramid.pad[0];
					shift[1] = (modelWd-double(opts.modelDs[1]))/2-opts.pPyramid.pad[1];

					BoundingBox bb;
					bb.topLeftPoint.x = c*stride;
					bb.topLeftPoint.x = (bb.topLeftPoint.x+shift[1])/opts.pPyramid.scales_w[i];
					bb.topLeftPoint.y = r*stride;
					bb.topLeftPoint.y = (bb.topLeftPoint.y+shift[0])/opts.pPyramid.scales_h[i];
					bb.height = opts.modelDs[0]/opts.pPyramid.scales[i];
					bb.width = opts.modelDs[1]/opts.pPyramid.scales[i];
					bb.scale = i;


  				}
		  }
		}
		delete [] cids;
		free(chns);
		m=cs.size();

		// shift=(modelDsPad-modelDs)/2-pad;
		// double shift[2];
		// shift[0] = (modelHt-double(opts.modelDs[0]))/2-opts.pPyramid.pad[0];
		// shift[1] = (modelWd-double(opts.modelDs[1]))/2-opts.pPyramid.pad[1];

		for(int j=0; j<m; j++ )
		{
			BoundingBox bb = pyramidRowColumn2BoundingBox(rs[j], cs[j],  modelHt, modelWd, i, stride) ;

			// bb.topLeftPoint.x = cs[j]*stride;
			// bb.topLeftPoint.x = (bb.topLeftPoint.x+shift[1])/opts.pPyramid.scales_w[i];
			// bb.topLeftPoint.y = rs[j]*stride;
			// bb.topLeftPoint.y = (bb.topLeftPoint.y+shift[0])/opts.pPyramid.scales_h[i];
			// bb.height = opts.modelDs[0]/opts.pPyramid.scales[i];
			// bb.width = opts.modelDs[1]/opts.pPyramid.scales[i];
			// bb.score = hs1[j];
			// bb.scale = i;
			result.push_back(bb);
		}

		cs.clear();
		rs.clear();
		hs1.clear();
	}

	std::cout << "max_h " << max_h << std::endl;

	return result;
}





//bb = acfDetect1(P.data{i},Ds{j}.clf,shrink,modelDsPad(1),modelDsPad(2),opts.stride,opts.cascThr);
void Detector::acfDetect(std::vector<std::string> imageNames, std::string dataSetDirectoryName, int firstFrame, int lastFrame)
{
	int shrink = opts.pPyramid.pChns.shrink;
	int modelHt = opts.modelDsPad[0];
	int modelWd = opts.modelDsPad[1];
	int stride = opts.stride;
	float cascThr = opts.cascadeThreshold;

	cv::Mat tempThrs;
	cv::transpose(this->thrs, tempThrs);
	float *thrs = (float*)tempThrs.data;

	cv::Mat tempHs;
	cv::transpose(this->hs, tempHs);
	float *hs = (float*)tempHs.data;
	
	cv::Mat tempFids;
	cv::transpose(this->fids, tempFids);
	uint32 *fids = (uint32*) tempFids.data;
	
	cv::Mat tempChild;
	cv::transpose(this->child, tempChild);
	uint32 *child = (uint32*) tempChild.data;

	// const mwSize *fidsSize = mxGetDimensions(mxGetField(trees,0,"fids"));
	// const int nTreeNodes = (int) fidsSize[0];
 	// const int nTrees = (int) fidsSize[1];
	int nTreeNodes = this->fids.rows;
	int nTrees = this->fids.cols;
	
	int treeDepth = this->treeDepth;
	int nChns = opts.pPyramid.pChns.pColor.nChannels + opts.pPyramid.pChns.pGradMag.nChannels + opts.pPyramid.pChns.pGradHist.nChannels; 

	for (int i = firstFrame; i < imageNames.size() && i < lastFrame; i++)
	{
		clock_t frameStart = clock();

		// this conversion is necessary, so we don't apply this transformation multiple times, which would break the image inside chnsPyramid
		cv::Mat image = cv::imread(dataSetDirectoryName + '/' + imageNames[i]);

		// if resize image is set different to 1.0, resize before computing the pyramid
		std::cout << "Resizing the image, if required..." << std::endl;
		if (config.resizeImage != 1.0) {
			cv::resize(image, image, cv::Size(), config.resizeImage, config.resizeImage);
		}

		cv::Mat I;
		// which one of these conversions is best?
		//image.convertTo(I, CV_32FC3, 1.0/255.0);
		cv::normalize(image, I, 0.0, 1.0, cv::NORM_MINMAX, CV_32FC3);

		// compute feature pyramid
		std::vector<Info> framePyramid;
		framePyramid = opts.pPyramid.computeMultiScaleChannelFeaturePyramid(I);
	
		clock_t detectionStart = clock();

		// BB_Array frameDetections = applyDetectorToFrame(framePyramid, shrink, modelHt, modelWd, stride, cascThr, thrs, hs, fids, child, nTreeNodes, nTrees, treeDepth, nChns);

		float fP[12] = 
   				{1.0e+06*0.000736749413407,  1.0e+06*-0.000223100219677,  1.0e+06*0.000082536752103,  1.0e+06*-1.241574319735051,
   				 1.0e+06*-0.000026514058126,  1.0e+06*0.000013789593083,  1.0e+06*0.000580062688244,  1.0e+06*-3.419049533403133,
    			 1.0e+06*0.000000736718019,  1.0e+06*0.000000533257141,  1.0e+06*0.000000146092928,  1.0e+06*-0.002129474400950};
    	cv::Mat_<float> P(3, 4, fP);

    	if (config.resizeImage != 1.0) {
    		float s = config.resizeImage;
    		float scale_matrix[9] = {s, 0.0, 0.0, 0.0, s, 0.0, 0.0, 0.0, 1.0};

    		cv::Mat_<float> S(3, 3, scale_matrix);
    		P = S * P;
    	}

    	print_fmatrix("Projection matrix", P);
		
		BB_Array frameDetections = applyDetectorToFrameSmart(framePyramid, shrink, modelHt, modelWd, stride, cascThr, thrs, hs, fids, child, nTreeNodes, nTrees, treeDepth, nChns, image.cols, image.rows, P, image);


		
		//BB_Array frameDetections = applyDetectorToFrame(framePyramid, shrink, modelHt, modelWd, stride, cascThr, thrs, hs, fids, child, nTreeNodes, nTrees, treeDepth, nChns);
		detections.push_back(frameDetections);
		frameDetections.clear(); //doesn't seem to make a difference

		clock_t detectionEnd = clock();
		timeSpentInDetection = timeSpentInDetection + (double(detectionEnd - detectionStart) / CLOCKS_PER_SEC);

		
		// debug: shows detections before suppression
		cv::imshow("source image", I);
		showDetections(I, detections[i], "detections before suppression");
		// debug 

		cv::waitKey();
		exit(0);


		detections[i] = nonMaximalSuppression(detections[i]);

		
		// debug: shows detections after suppression
		showDetections(I, detections[i], "detections after suppression");
		//printDetections(detections[i], i);
		cv::waitKey();
		// debug */

		
		// debug: saves image with embedded detections
		for (int j = 0; j<detections[i].size(); j++) 
			detections[i][j].plot(image, cv::Scalar(0,255,0));
		cv::imwrite("../datasets/results/"+imageNames[i], image);
		// debug */

		
		// experimental: do i need to clear these?
		for (int j=0; j < opts.pPyramid.computedScales; j++)
		{
			framePyramid[j].image.release();
			framePyramid[j].gradientMagnitude.release();
			framePyramid[j].gradientHistogram.clear();
		}
		image.release();
		I.release();
		// experimental */

		clock_t frameEnd = clock();
		double elapsed_secs = double(frameEnd - frameStart) / CLOCKS_PER_SEC;

		std::cout << "Frame " << i+1 << " of " << imageNames.size() << " was processed in " << elapsed_secs << " seconds.\n"; 
	}
}

// for each i suppress all j st j>i and area-overlap>overlap
BB_Array nmsMax(BB_Array source, bool greedy, double overlapArea, cv::String overlapDenominator)
{
	BB_Array result;
	BB_Array sortedArray;
	// bool discarded[source.size()];
	bool *discarded = (bool*)malloc(source.size()*sizeof(bool));

	for (int i=0; i < source.size(); i++)
	{
		sortedArray.push_back(source[i]);
		discarded[i] = false;
	}
 
	std::sort(sortedArray.begin(), sortedArray.begin()+sortedArray.size());
	
	for (int i = 0; i < sortedArray.size(); i++)
	{
		if (!greedy || !discarded[i]) // continue only if its not greedy or result[i] was not yet discarded
		{
			for (int j = i+1; j < sortedArray.size(); j++)
			{
				if (discarded[j] == false) // continue this iteration only if result[j] was not yet discarded
				{
					double xei, xej, xmin, xsMax, iw;
					double yei, yej, ymin, ysMax, ih;
					xei = sortedArray[i].topLeftPoint.x + sortedArray[i].width;
					xej = sortedArray[j].topLeftPoint.x + sortedArray[j].width;
					xmin = xej;			
					if (xei < xej)
						xmin = xei;
					xsMax = sortedArray[i].topLeftPoint.x;
					if (sortedArray[j].topLeftPoint.x > sortedArray[i].topLeftPoint.x)
						xsMax = sortedArray[j].topLeftPoint.x;
					iw = xmin - xsMax;
					yei = sortedArray[i].topLeftPoint.y + sortedArray[i].height;
					yej = sortedArray[j].topLeftPoint.y + sortedArray[j].height;
					ymin = yej;			
					if (yei < yej)
						ymin = yei;
					ysMax = sortedArray[i].topLeftPoint.y;
					if (sortedArray[j].topLeftPoint.y > sortedArray[i].topLeftPoint.y)
						ysMax = sortedArray[j].topLeftPoint.y;
					ih = ymin - ysMax;
					if (iw > 0 && ih > 0)
					{
						double o = iw * ih;
						double u;
						if (overlapDenominator == "union")
							u = sortedArray[i].height*sortedArray[i].width + sortedArray[j].height*sortedArray[j].width-o;
						else if (overlapDenominator == "min")
						{
							u = sortedArray[i].height*sortedArray[i].width;
							if (sortedArray[i].height*sortedArray[i].width > sortedArray[j].height*sortedArray[j].width)
								u = sortedArray[j].height*sortedArray[j].width;
						}
						o = o/u;
						if (o > overlapArea) // sortedArray[j] is no longer needed (is discarded)
							discarded[j] = true;
					}
				}
			}	
		}
	}
	
	// result keeps only the bounding boxes that were not discarded
	for (int i=0; i < sortedArray.size(); i++)
		if (!discarded[i])
			result.push_back(sortedArray[i]);

	free(discarded);

	return result;
}

BB_Array Detector::nonMaximalSuppression(BB_Array bbs)
{
	BB_Array result;

	//keep just the bounding boxes with scores higher than the threshold
	for (int i=0; i < bbs.size(); i++)
		if (bbs[i].score > opts.suppressionThreshold)
			result.push_back(bbs[i]);

	// bbNms would apply resize to the bounding boxes now
	// but our models dont use that, so it will be suppressed
		
	// since we just apply one detector model at a time,
	// our separate attribute would always be false
	// so the next part is simpler, nms1 follows
	
	// if there are too many bounding boxes,
	// he splits into two arrays and recurses, merging afterwards
	// this will be done if necessary
	
	// run actual nms on given bbs
	// other types might be added later
	switch (opts.suppressionType)
	{
		case MAX:
			result = nmsMax(result, false, opts.overlapArea, opts.overlapDenominator);
		break;
		case MAXG:
			result = nmsMax(result, true, opts.overlapArea, opts.overlapDenominator);
		break;
		case MS:
			// not yet implemented
		break;
		case COVER:
			// not yet implemented
		break;	
	}

	return result;
}