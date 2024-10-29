// CCalibration.h


#ifndef _CCALIBRATION_h
#define _CCALIBRATION_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Eigen.h>
#include <Eigen/QR>
#include <Eigen/Dense>
#include <cmath>

using namespace Eigen;
class CCalibration
{
protected:
private:
	double waveForce[3] = { 0.0, };

public:
	CCalibration();
	MatrixXd stiffMtx = MatrixXd(3, 4);
	Vector4d centerWave;
	Vector3d tempRatio;
	void calCalib(double wavelength[]);
	void getValue(double *val);
};


#endif

