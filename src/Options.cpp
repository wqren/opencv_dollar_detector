#include "Options.h"

void Options::readOptions(FileNode optionsNode)
{
	pPyramid.readPyramid(optionsNode["pPyramid"]);

	pNms.readPNms(optionsNode["pNms"]);

	modelDs[0] = optionsNode["modelDs"][0];
	modelDs[1] = optionsNode["modelDs"][1];
	modelDsPad[0] = optionsNode["modelDsPad"][0];
	modelDsPad[1] = optionsNode["modelDsPad"][1];
	stride = optionsNode["stride"];
	cascadeThreshold = optionsNode["cascThr"];
	cascadeCalibration = optionsNode["cascCal"];
	nWeak[0] = optionsNode["nWeak"][0];
	nWeak[1] = optionsNode["nWeak"][1];
	nWeak[2] = optionsNode["nWeak"][2];
	nWeak[3] = optionsNode["nWeak"][3];
	seed = optionsNode["seed"];
	name = (string)optionsNode["name"];
	posGtDir = (string)optionsNode["posGtDir"];
	posImgDir = (string)optionsNode["posImgDir"];
	negImgDir = (string)optionsNode["negImgDir"];
	nPos = optionsNode["nPos"];
	nNeg = optionsNode["nNeg"];
	nPerNeg = optionsNode["nPerNeg"];
	nAccNeg = optionsNode["nAccNeg"];
	winsSave = optionsNode["winsSave"];
}
