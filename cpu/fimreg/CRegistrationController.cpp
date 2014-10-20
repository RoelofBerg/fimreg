/* =====================================
=== FIMREG - Fast Image Registration ===
========================================

Written by Roelof Berg, Berg Solutions (rberg@berg-solutions.de) with support from
Lars Koenig, Fraunhofer MEVIS (lars.koenig@mevis.fraunhofer.de) Jan Ruehaak, Fraunhofer
MEVIS (jan.ruehaak@mevis.fraunhofer.de).

THIS IS A LIMITED RESEARCH PROTOTYPE. Documentation: www.berg-solutions.de/fimreg.html

------------------------------------------------------------------------------

Copyright (c) 2014, Roelof Berg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the owner nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------*/

/**
* fimreg.cpp : Defines the entry point for the console application.
*/

//precompiled header (MUST BE THE FIRST ENTRY IN EVERY CPP FILE)
#include "stdafx.h"

#include "CRegistrationController.h"
#include "CRegistrator.h"

#include "CThreadPool.h"
/**
* Threadpool. This is global because it is used from the c-style matlab generated code.
*/
CThreadPool* gpThreadPool=NULL;

#include "CHPRPCConnection.h"
#include "matlab_timing_utils.h"
//Global because Matlab Coder generated C code accesses this object while processing a matlab function
vector<CHPRPCConnection*> g_CHPRPCConnection;
uint32_t guCoreAmount=0;
uint32_t guClipDarkNoise=0;

/**
 * Common error message text when image loading fails.
 */
const string gcsCannotLoadImage = "Cannot load image.";

//our own log2 command
//t_reg_real log2(t_reg_real d) {return log(d)/log(t_reg_real(2)) ;}

CRegistrationController::CRegistrationController(CRegistrator& Registrator)
: m_uiMaxIter(0),
  m_iLevelCount(0),
  m_Registrator(Registrator),
  m_fStopSens(REG_REAL_NAN),
  m_fMaxRotation(0),
  m_fMaxTranslation(0),
  m_bAssumeNoLocalMinimum(false),
  m_bNoGui(false)
{
}

CRegistrationController::~CRegistrationController()
{
	DisconnectDSPs();
}

int CRegistrationController::Main(int argc, char *argv[])
{
	//Parse CMDLine
	if (!ParseParameters(argc, argv))
	{
		return APP_RET_ERROR;
	}

	//Create thread pool (only in DSP mode not in local mode)
	if (0 == m_uDSPAmount)
	{
		RegisterImage();
	}
	else
	{
		//Create our thread pool (This is global because it is used from the c-style matlab code.)
		gpThreadPool = new CThreadPool();

		//Initialize PCIe
		if (!CHPRPCConnection::InitPCIe())
		{
			CLogger::PrintError("PCIe communication initialization failed.");
			return APP_RET_ERROR;
		}
		else
		{
			try
			{
				ConnectDSPs();

				//Now the arguments are parsed and the communication layer is established -> Load the images and start the registration algorithm
				RegisterImage();
			}
			catch (...)
			{
				//Deinitialize PCIe
				CHPRPCConnection::ShutdownPCIe();
				throw;
			}
			//Deinitialize PCIe
			CHPRPCConnection::ShutdownPCIe();
		}
	}

	return APP_RET_SUCCESS;
}

void CRegistrationController::RegisterImage()
{
	// load template image
	// ToDo: This class is too big. Refactor out an image class containing all image handling (maybe outlayering OpenCV) (and possibly also the cmdline param stuff at the bottom of this file).
	IplImage* imgTmp = 0; 
	imgTmp=cvLoadImage(m_sTFilename.c_str(), 0);
	if(!CheckImage(imgTmp, m_sTFilename))
		exit(0);
	uint32_t iDim = imgTmp->height;
	t_pixel* pixelBytesTmp = (t_pixel *)imgTmp->imageData;

	// load reference image
	IplImage* imgRef = 0; 
	imgRef=cvLoadImage(m_sRFilename.c_str(), 0);
	if(!CheckImage(imgRef, m_sRFilename, iDim))
		exit(0);
	t_pixel* pixelBytesRef = (t_pixel *)imgRef->imageData;

	//When levelcount is set to 0: Autodetect of amount of levels (multilevel pyramid)
	if(0 == m_iLevelCount)
	{
		m_iLevelCount = uint32_t(ceil(log2(t_reg_real(iDim / gui_LEVELCOUNT_AUTOTETECT_DIVISOR))));
		printf("Multilevel autodetection suggests %i levels.\n", m_iLevelCount);
	}

	//Allocate memory according to cmdline params for the SSD graph
	t_reg_real* afSSDDecay = new t_reg_real[m_uiMaxIter*m_iLevelCount];

	//Register Images
	const int ciRegParamCount = 3;
	t_reg_real fW[ciRegParamCount];
	uint32_t iNumIter = m_Registrator.RegisterImages(m_uDSPAmount, (uint32_t)iDim, m_uiMaxIter, m_fMaxRotation, m_fMaxTranslation, m_iLevelCount, m_fStopSens, m_bAssumeNoLocalMinimum, pixelBytesRef, pixelBytesTmp, fW, afSSDDecay);

	string sResult = (boost::format("Iterations = %1%, SSD = %2%, w = [%3% deg, %4%, %5%]") % iNumIter % afSSDDecay[iNumIter-1] % (fW[0]*180/M_PI) % fW[1] % fW[2]).str();
	printf("%s\n", sResult.c_str());

	//Release the DSP connections
	DisconnectDSPs();

	bool bNeedTransImage = false;
	IplImage* imgTmpTrns;
	if(!m_bNoGui)
	{

		bNeedTransImage = (!m_bNoGui) || (0<m_sSaveTransImage.size());
		if(bNeedTransImage)
		{
			// calculate transformed template image
			imgTmpTrns=cvCloneImage(imgTmp);
			m_Registrator.TransformReferenceImage(iDim, fW, (t_pixel *)imgTmp->imageData, (t_pixel *)imgTmpTrns->imageData);

			if(0<m_sSaveTransImage.size())
			{
				printf("Saving result to file '%s'.\n", m_sSaveTransImage.c_str());
				int iRetval = cvSaveImage(m_sSaveTransImage.c_str(), imgTmpTrns);
				/*if(0!=iRetval)
				{
					printf("ERROR, CANNOT SAVE IMAGE, CHECK FILENAME.\n");
				}*/
			}
		}

		// calculate difference image between ORIGINAL template image and reference image
		IplImage* imgDiffOrig;
		imgDiffOrig=cvCloneImage(imgTmp);
		m_Registrator.CalculateDiffImage(iDim, (t_pixel *)imgRef->imageData, (t_pixel *)imgTmp->imageData, (t_pixel *)imgDiffOrig->imageData);

		// calculate difference image between TRANSFORMED template image and reference image
		IplImage* imgDiffFinal;
		imgDiffFinal=cvCloneImage(imgTmp);
		m_Registrator.CalculateDiffImage(iDim, (t_pixel *)imgRef->imageData, (t_pixel *)imgTmpTrns->imageData, (t_pixel *)imgDiffFinal->imageData);

		// show images
		//Intelligent image display size (size of image but not more than a maximum)
		uint32_t iDisplayImgSize = (iDim>APP_MAX_IMG_SIZE) ? APP_MAX_IMG_SIZE : iDim;
		ShowImage(imgTmp, "Template Image", 0, 0, iDisplayImgSize);
		ShowImage(imgRef, "Reference Image", 1, 0, iDisplayImgSize);
		ShowImage(imgTmpTrns, "Registered Image", 2, 0, iDisplayImgSize);
		ShowImage(imgDiffOrig, "Difference BEFORE", 0, 1, iDisplayImgSize);
		ShowImage(imgDiffFinal, "Difference AFTER", 1, 1, iDisplayImgSize);

		// Give windows some time to paint the content
		cvWaitKey(1);

		// wait for a key
		cvWaitKey(0);

		// release the images
		cvReleaseImage(&imgDiffFinal);
		cvReleaseImage(&imgDiffOrig);
	}

	if(bNeedTransImage)
	{
		cvReleaseImage(&imgTmpTrns);
	}
	cvReleaseImage(&imgRef);
	cvReleaseImage(&imgTmp);
	//Output afSSDDecay here if you like. E.g. by using CVPlot. (Because of our multilevel scheme, however, this gives little information.)
	delete afSSDDecay;

	//Close thread pool (This is global because it is used from the c-style matlab generated code.)
	if(NULL != gpThreadPool)
	{
		delete gpThreadPool;
		gpThreadPool=0;
	}
}

/**
* Instantiate a connection object for each DSP (will be deleted in destructor)
*/
void CRegistrationController::ConnectDSPs()
{
	for(uint32_t i=0; i<m_uDSPAmount; i++)
	{
		CHPRPCConnection* pConnection = new CHPRPCConnection(i, m_uDSPAmount);
		pConnection->ConnectToTarget();
		g_CHPRPCConnection.push_back(pConnection);
	}
}

/**
* Disconnect all remote DSPs (if any are connected)
*/
void CRegistrationController::DisconnectDSPs()
{
	//Delete any left over connection object
	BOOST_FOREACH(CHPRPCConnection* pConnection, g_CHPRPCConnection)
	{
		delete pConnection;
	}
	g_CHPRPCConnection.clear();
}

/**
* Check wether image file is valid. (Will also check image size)
*/
bool CRegistrationController::CheckImage(IplImage* Image, const string& sFilename, uint32_t Dim)
{
	//Show all errors at once to the user (e.g. wrong color, height and width)
	bool bRetVal = CheckImage(Image, sFilename);
	if(NULL == Image)
	{
		//Function call above allready reported an error
		return false;
	}

	//Above checks wether image is square
	if(Image->height != Dim)
	{
		printf("%s Width and height must be %i but are %i.\n", gcsCannotLoadImage.c_str(), Dim, Image->height);
		bRetVal=false;
	}

	return bRetVal;
}

/**
* Check wether image file is valid. (Will not check image size)
*/
bool CRegistrationController::CheckImage(IplImage* Image, const string& sFilename)
{
	if(NULL == Image)
	{
		printf("%s '%s'", gcsCannotLoadImage.c_str(), sFilename.c_str());
		return false;
	}

	//Show all errors at once to the user (e.g. wrong color, height and width)
	bool bRetVal=true;

	const int iAllowedChannelCount=1;
	if(Image->nChannels != iAllowedChannelCount)
	{
		printf("%s Color channel count must be %i but is %i.\n", gcsCannotLoadImage.c_str(), iAllowedChannelCount, Image->nChannels);
		bRetVal=false;
	}

	if(Image->height != Image->width)
	{
		printf("%s Image dimensions must be square (width=height) but size is %i x %i.\n", gcsCannotLoadImage.c_str(), Image->width, Image->height);
		bRetVal=false;
	}

	/*
	const int iAllowedWidthStep=iDim;
	if(Image->widthStep != iDim)
	{
	printf("%s Internal image property WidthStep must be %i but is %i.\n", gcsCannotLoadImage.c_str(), iDim, Image->widthStep);
	bRetVal=false;
	}
	*/

	if((Image->height%2)!=0)
	{
		printf("%s Image dimensions must be even but width and height are odd (%i).\n", gcsCannotLoadImage.c_str(), Image->height);
		bRetVal=false;
	}

	if(Image->height<=0)
	{
		printf("%s Image dimensions must be bigger than 0 (but are %i).\n", gcsCannotLoadImage.c_str(), Image->height);
		bRetVal=false;
	}

	return bRetVal;
}

/**
* Display image data of 'image' in window named 'WindowName' at the zero based image position xPos, yPos.
* The position is calculated based on the image dimensions WndSize (e.g. 1,2 an image in the second row and third column)
*/
void CRegistrationController::ShowImage(IplImage* Image, const string& WindowName, uint32_t xPos, uint32_t yPos, uint32_t WndSize)
{
	cvNamedWindow(WindowName.c_str(), 0); 
	MoveWindow(WindowName, xPos, yPos, WndSize);
	cvShowImage(WindowName.c_str(), Image );
}

/**
* Move a window to the zero based image position xPos, yPos.
* The position is calculated based on the image dimensions WndSize (e.g. 1,2 an image in the second row and third column)
*/
void CRegistrationController::MoveWindow(const string& WindowName,  uint32_t xPos, uint32_t yPos, uint32_t WndSize)
{
	const uint32_t iXSpacer=20;
	const uint32_t iYSpacer=40;

	uint32_t xPosPixel = xPos*(iXSpacer+WndSize);
	uint32_t yPosPixel = yPos*(iYSpacer+WndSize);
	cvMoveWindow(WindowName.c_str(), xPosPixel, yPosPixel);
	cvResizeWindow(WindowName.c_str(), WndSize, WndSize);
}

/**
 * Read and validate the shell command line parameters the user entered.
 * Print out an error report and usage-information if the parameter validation failed.
 * Print out only usage-information if the user requested this information.
 * If parameter parsing was successful: Print out parsed parameters for verification.
 *
 * \param argc amount of cmdline args
 * \param argv pointer to array of argument strings
 * \retval success
 */
bool CRegistrationController::ParseParameters(int argc, char ** argv)
{
	// DEFAULT CMDLINE PARAMETERS
	const uint32_t DEF_CMD_PARAM_MAXITER = 150;
	const uint32_t DEF_CMD_PARAM_LEVELCOUNT = 0;
	const t_reg_real DEF_CMD_PARAM_MAXROTATION = 10.0f;
	const t_reg_real DEF_CMD_PARAM_MAXTRANSLATION = 10.0f;
	const t_reg_real DEF_CMD_PARAM_STOPSENS = 0.7f;
	const uint32_t DEF_CMD_PARAM_COREAMOUNT=8;
	const uint8_t DEF_CMD_PARAM_CDN=10;
	const uint32_t MAX_CMD_PARAM_DSPS=4;	//max value
	const uint32_t DEF_CMD_PARAM_DSPS=0;	//default value

	//CMDLine parameter tokens
	const char csHelp[] = "help";
	const char csTFilename[] = "tfile";
	const char csRFilename[] = "rfile";
	const char csOutFilename[] = "outfile";
	const char csDSPs[] = "dsps";
	const char csCores[] = "cores";
	const char csMaxIter[] = "maxiter";
	const char csLevelCount[] = "levels";
	const char csMaxRotation[] = "maxrot";
	const char csMaxTranslation[] = "maxtrans";
	const char csStopSens[] = "stopsens";
	const char csAssumeNoLocalMinima[] = "nlm";
	const char csClipDarkNoise[] = "cdn";
	const char csNoGui[] = "nogui";

	//Define expected cmdline parameters to boost
	options_description desc("fimreg - Fast Image Registration\n"
		                     "Performs an image registration either on the local PC or on remotely connected DSP processors. "
		                     "The image files must be square and grayscale. Supported image formats: *.bmp, *.dib, *.jpeg, "
							 "*.jpg, *.jpe, *.jp2, *.png, *.pbm, *.pgm, *.ppm, *.sr, *.ras, *.tiff, *.tif.\n\n"
							 "Copyright 2014, Roelof Berg, Licensed under the 3-clause BSD license at "
							 "http://berg-solutions.de/fimreg-license.html" ".\n\n"
							 "Internally used libraries: OpenCV (http://opencv.org), Boost (http://boost.org).\n"
							 "\n\nUsage"
							 );

	desc.add_options()
		(csHelp, "Show usage information.")

		(csRFilename, value<string>(), "Template image file (will be transformed). The image must be grayscale colored and it must have even and square pixel dimensions.")

		(csTFilename, value<string>(), "Reference image file (will be matched against). The image must be grayscale colored and it must have even and square pixel dimensions.")

		(csOutFilename, value<string>(), "Output image file (optional). Gives the option to save the registered template image.")

		(csDSPs, value< uint32_t >(), (boost::format("Amount of DSP chips to be used (connected over PCIe). A value of zero indicates that the calculcation shall take place on the local processor only.\n"
									"[Optional parameter, range 0...%1%, default: %2%]") % MAX_CMD_PARAM_DSPS % DEF_CMD_PARAM_DSPS).str().c_str())

		(csCores, value<uint32_t>(), (boost::format("Amount of DSP calculation cores.\n"
									"[Optional parameter, range 1...8, default: %1%]") % DEF_CMD_PARAM_COREAMOUNT).str().c_str())

		(csMaxIter, value<uint32_t>(), (boost::format("Max. amount of iterations.\n"
									"[Optional parameter, default: %1%]") % DEF_CMD_PARAM_MAXITER).str().c_str())

		(csLevelCount, value<uint32_t>(), (boost::format("Amount of levels in the multilevel pyramid of shrinked image copies. 0 means autodetect by the formula log2(ImageWidth/%1%).\n"
									"[Optional parameter, default: %2%]") % gui_LEVELCOUNT_AUTOTETECT_DIVISOR % DEF_CMD_PARAM_LEVELCOUNT).str().c_str())

		(csMaxRotation, value<t_reg_real>(), (boost::format("Max. expected rotation in degrees. Used for internal optimizations in DSP calculation mode. "
									"If a rotation value will become necessary that is higher than specified here, the algorithm will still succeed "
									"but the calculation speed will slow down).\n"
									"[Optional parameter, default: %1%]") % DEF_CMD_PARAM_MAXROTATION).str().c_str())

		(csMaxTranslation, value<t_reg_real>(), (boost::format("Max. expected translation in percent. Used for internal optimizations in DSP calculation mode. "
									"If a translation value will become necessary that is higher than specified here, the algorithm will still succeed "
									"but the calculation speed will slow down).\n"
									"[Optional parameter, default: %1%]") % DEF_CMD_PARAM_MAXTRANSLATION).str().c_str())

		(csStopSens, value<t_reg_real>(), (boost::format("Sensitivity of the STOP criteria for the gauss-newton algorithm.\n"
									"[Optional parameter, use ranges between 1 (not sensitive, stops early) to 0.0001 (very sensitive) default: %1%]") % DEF_CMD_PARAM_STOPSENS).str().c_str())

		(csAssumeNoLocalMinima, "No Local Mimina: If this parameter is specified the algorithm stops allready when the SSD difference between two iterations becomes very small. "
		"If it is not specified additional rules will be applied to calculate better results in some special cases like escaping a local minimum.\n"
									"[Flag parameter]")

		(csClipDarkNoise, value<uint32_t>(), (boost::format("Clip dark pixels until (including) the given luminance to zero. This reduces image sensor noise in the usually uninteresting dark pixels. A high value optimizes data compression and calculation performance at the cost of calculation accuracy.\n"
									"[Optional parameter, range 0...255, 0 disables any clipping, default: %1%]") % (uint32_t)DEF_CMD_PARAM_CDN).str().c_str())

        (csNoGui, "If active no GUI output will be shown. Use this for batch registration of several images.\n"
									"[Flag parameter]")
	;

	try
	{
		variables_map vm;
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);

		//CMDLine parameter --help .......................................................................................
		if (0 < vm.count(csHelp))
		{
			CLogger::PrintUsage(desc);
			return false;
		}

		//CMDLine parameter --t .......................................................................................
		if (0 < vm.count(csTFilename))
		{
			m_sTFilename = vm[csTFilename].as<string>();
			boost::algorithm::trim(m_sTFilename);
		}
		else //== 0
		{
			m_sTFilename = "";	//Just to be stateless (unnecessary when method called only once)
		}

		if(0 == m_sTFilename.size())
		{
			//Argument missing: Error and exit (mandatory)
			CLogger::PrintError((boost::format("Missing argument '--%1%' (template image).") % csTFilename).str());
			CLogger::PrintUsage(desc);
			return false;
		}

		//CMDLine parameter --r .......................................................................................
		if (0 < vm.count(csRFilename))
		{
			m_sRFilename = vm[csRFilename].as<string>();
			boost::algorithm::trim(m_sRFilename);
		}
		else //== 0
		{
			m_sRFilename = "";	//Just to be stateless (unnecessary when method called only once)
		}

		if(0 == m_sRFilename.size())
		{
			//Argument missing: Error and exit (mandatory)
			CLogger::PrintError((boost::format("Missing argument '--%1%' (reference image).") % csRFilename).str());
			CLogger::PrintUsage(desc);
			return false;
		}

		//CMDLine parameter --out .......................................................................................
		if (0 < vm.count(csOutFilename))
		{
			m_sSaveTransImage = vm[csOutFilename].as<string>();
			boost::algorithm::trim(m_sSaveTransImage);
		}
		else //== 0
		{
			m_sSaveTransImage = "";	//Just to be stateless (unnecessary when method called only once)
		}

		if(0 == m_sRFilename.size())
		{
			//Argument missing: Error and exit (mandatory)
			CLogger::PrintError((boost::format("Missing argument '--%1%' (reference image).") % csRFilename).str());
			CLogger::PrintUsage(desc);
			return false;
		}

		//CMDLine parameter --dsps .......................................................................................
		if (0 < vm.count(csDSPs))
		{
			m_uDSPAmount = vm[csDSPs].as<uint32_t>();
			if(MAX_CMD_PARAM_DSPS<m_uDSPAmount)
			{
				CLogger::PrintError((boost::format("Invalid argument '--%1%'. Max. amount of DSPs must be within the range 0...%2%.") % csDSPs % MAX_CMD_PARAM_DSPS).str());
				CLogger::PrintUsage(desc);
				return false;
			}
		}
		else //== 0
		{
			//Argument missing: Use hardcoded default value
			m_uDSPAmount = DEF_CMD_PARAM_DSPS;
		}
		
		//CMDLine parameter --cores .......................................................................................
		if (0 < vm.count(csCores))
		{
			guCoreAmount = vm[csCores].as<uint32_t>();
			if(0 == m_uDSPAmount)
			{
				CLogger::PrintError((boost::format("Invalid argument '--%1%'. Max. amount of DSP cores can only be specified when argument '--%2%' specifies at least one DSP.") % csCores % csDSPs).str());
				CLogger::PrintUsage(desc);
				return false;
			}
			else if(0>=guCoreAmount || 8<guCoreAmount)
			{
				CLogger::PrintError((boost::format("Invalid argument '--%1%'. Max. amount of cores must be within the range 1...8 cores.") % csCores).str());
				CLogger::PrintUsage(desc);
				return false;
			}
		}
		else //== 0
		{
			//Argument missing: Use hardcoded default value
			guCoreAmount = DEF_CMD_PARAM_COREAMOUNT;
		}

		//CMDLine parameter --maxiter .......................................................................................
		if (0 < vm.count(csMaxIter))
		{
			m_uiMaxIter = vm[csMaxIter].as<uint32_t>();
			if(m_uiMaxIter==0)
			{
				CLogger::PrintError((boost::format("Invalid argument '--%1%'. Max. amount of iterations must be > 0 iterations.") % csMaxIter).str());
				CLogger::PrintUsage(desc);
				return false;
			}
		}
		else //== 0
		{
			//Argument missing: Use hardcoded default value
			m_uiMaxIter = DEF_CMD_PARAM_MAXITER;
		}


		//CMDLine parameter --levels .......................................................................................
		if (0 < vm.count(csLevelCount))
		{
			m_iLevelCount = vm[csLevelCount].as<uint32_t>();
			if(m_iLevelCount==0)
			{
				CLogger::PrintError((boost::format("Invalid argument '--%1%'. Max. amount of pyramid levels must be > 0.") % csLevelCount).str());
				CLogger::PrintUsage(desc);
				return false;
			}
		}
		else //== 0
		{
			//Argument missing: Use hardcoded default value
			m_iLevelCount = DEF_CMD_PARAM_LEVELCOUNT;
		}


		//CMDLine parameter --maxrot .......................................................................................
		if (0 < vm.count(csMaxRotation))
		{
			m_fMaxRotation = vm[csMaxRotation].as<t_reg_real>();
			uint32_t iMaxRotDeg=180;
			if(m_fMaxRotation<0 || m_fMaxRotation>iMaxRotDeg)
			{
				CLogger::PrintError((boost::format("Invalid argument '--%1%'. Max. rotaion must be in between the range [0 deg ... %2% deg].") % csMaxRotation % iMaxRotDeg).str());
				CLogger::PrintUsage(desc);
				return false;
			}
		}
		else //== 0
		{
			//Argument missing: Use hardcoded default value
			m_fMaxRotation = DEF_CMD_PARAM_MAXROTATION;
		}
		//Convert from degree to radians
		m_fMaxRotation = (t_reg_real)(m_fMaxRotation*M_PI/180);

		//CMDLine parameter --maxtrans .......................................................................................
		if (0 < vm.count(csMaxTranslation))
		{
			m_fMaxTranslation = vm[csMaxTranslation].as<t_reg_real>();
			if(m_fMaxTranslation<0)
			{
				CLogger::PrintError((boost::format("Invalid argument '--%1%'. Max. translation must be >= 0 pixel.") % csMaxTranslation).str());
				CLogger::PrintUsage(desc);
				return false;
			}
		}
		else //== 0
		{
			//Argument missing: Use hardcoded default value
			m_fMaxTranslation = DEF_CMD_PARAM_MAXTRANSLATION;
		}

		//CMDLine parameter --stopsens .......................................................................................
		if (0 < vm.count(csStopSens))
		{
			m_fStopSens = vm[csStopSens].as<t_reg_real>();
		}
		else //== 0
		{
			//Argument missing: Use hardcoded default value
			m_fStopSens = DEF_CMD_PARAM_STOPSENS;
		}
		
		//CMDLine parameter --assume-no-local-minima ..........................................................................
		m_bAssumeNoLocalMinimum = (0 < vm.count(csAssumeNoLocalMinima));

		//CMDLine parameter --CDN .......................................................................................
		if (0 < vm.count(csClipDarkNoise))
		{
			//Boost handles uint8_t as char. Hence we use uint32_t and check for uint8_t value range ourselves.
			uint32_t uClipDarkNoise_u32 = vm[csClipDarkNoise].as<uint32_t>();

			if(255<uClipDarkNoise_u32)
			{
				CLogger::PrintError((boost::format("Invalid argument '--%1%'. Max. clipping luminance can be 255.") % uClipDarkNoise_u32).str());
				CLogger::PrintUsage(desc);
				return false;
			}

			guClipDarkNoise = (uint8_t)uClipDarkNoise_u32;
		}
		else //== 0
		{
			//Argument missing: Use hardcoded default value
			guClipDarkNoise = DEF_CMD_PARAM_CDN;
		}

		//CMDLine parameter --nogui ..........................................................................
		m_bNoGui = (0 < vm.count(csNoGui));

	}
	catch(exception& e)
	{
		CLogger::PrintError((boost::format("Invalid arguments. ('%1%')") % e.what()).str());
		CLogger::PrintUsage(desc);
		return false;
	}

	//..........................................................................................................................
	//Reply parsed parameters to user console
	const char szEnabled[] = "enabled";
	const char szDisabled[] = "disabled";
	CLogger::PrintInfo((boost::format("Parameters: %1%=\"%2%\" %3%=\"%4%\" %5%=\"%6%\" %7%=%8%[DSPs] %9%=%10%[cores] %11%=%12%[iter.] %13%=%14%[levels, 0=autom.] %15%=%16%[deg.] %17%=%18%[percent] %19%=%20% %21%=%22% %23%=%24% %25%=%26%")
			% csTFilename % m_sTFilename
			% csRFilename % m_sRFilename
			% csOutFilename % m_sSaveTransImage
			% csDSPs % m_uDSPAmount
			% csCores % guCoreAmount
			% csMaxIter % m_uiMaxIter
			% csLevelCount % m_iLevelCount
			% csMaxRotation % (m_fMaxRotation*180/M_PI)	//Convert from rad to deg when displaying back to user
			% csMaxTranslation % m_fMaxTranslation
			% csStopSens % m_fStopSens
			% csAssumeNoLocalMinima % (m_bAssumeNoLocalMinimum ? szEnabled : szDisabled)
			% csClipDarkNoise % guClipDarkNoise
			% csNoGui % (m_bNoGui ? szEnabled : szDisabled)
			).str());
	return true;
}


