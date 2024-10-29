// 
// 
// 

#include "CCalibration.h"

CCalibration::CCalibration()
{
	//stiffMtx << 1.9722, -4.2563, 1.7172, 0.0073183,
	//	-5.2241, 0.5265, 3.2435, -0.0006955,
	//	1.5886, 4.3034, 1.0509, -0.0052768;
/*
	stiffMtx << 0.2753, - 0.9464,    1.8197,
		- 0.5531, - 0.1659 ,   1.1854,
		0.4128,    0.5369 ,   3.6955,
		0.0014 ,   0.0062  ,  0.0029;
*/
	stiffMtx << 0, 0, 0,
		0, 0, 0,
		1.5792,   -0.4239,    1.4760;


	//stiffMtx << 1.9722, -4.2563, 1.7172, 0.0073183,
	//	-5.2241, 0.5265, 3.2435, -0.0006955,
	//	-4.1410, 0.8612, 0.8476, 0.0037136;

	tempRatio << 0.9911727, 1.0006271, 0.9885897;
	tempRatio << 0.99123843, 1.00057852, 0.98855525;
}

void CCalibration::calCalib(double wavelength[])
{
	Vector3d calForce;
	Vector4d waveLength;
	waveLength << wavelength[0], wavelength[1], wavelength[2], wavelength[3];


	Vector3d waveDiff;
	//waveDiff(0) = wavelength[0] - 0.99123843 * wavelength[3];
	//waveDiff(1) = wavelength[1] - 1.00057852 * wavelength[3];
	//waveDiff(2) = wavelength[2] - 0.98855525 * wavelength[3];
	waveDiff(0) = wavelength[0];
	waveDiff(1) = wavelength[1];
	waveDiff(2) = wavelength[2];

	calForce = stiffMtx * waveDiff;

	waveForce[0] = calForce(0);
	waveForce[1] = calForce(1);
	waveForce[2] = calForce(2);

	//for (int i = 0; i < 3; i++) {
	//	waveForce[i] = calForce(i);
	//}
}
void CCalibration::getValue(double *val)
{
	memcpy(val, waveForce, sizeof(waveForce));
}