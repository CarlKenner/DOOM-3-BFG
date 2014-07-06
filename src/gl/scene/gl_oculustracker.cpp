#include "gl/scene/gl_oculustracker.h"

OculusTracker::OculusTracker() 
	: pitch(0)
	, roll(0)
	, yaw(0)
{
#ifdef HAVE_OCULUS_API
	ovr_Initialize();// OVR::System::Init();
	hmd = ovrHmd_Create(0);
	if (hmd) {
		ovrHmd_GetDesc(hmd, &hmdDesc);
		ovrHmd_StartSensor(hmd,
			ovrSensorCap_Orientation | ovrSensorCap_YawCorrection, // supported
			ovrSensorCap_Orientation); // required
	}

	/*
	pFusionResult = new OVR::SensorFusion();
	pManager = *OVR::DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
	if(pHMD)
	{
		InfoLoaded = pHMD->GetDeviceInfo(&Info);
		pSensor = pHMD->GetSensor();
	}
	else
	{
		pSensor = *pManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
	}

	if (pSensor)
	{
		pFusionResult->AttachToSensor(pSensor);
	}
	pFusionResult->SetPredictionEnabled(true);
	pFusionResult->SetPrediction(0.020, true); // Never hurts to be 20 ms in future?
	*/
#endif
}

OculusTracker::~OculusTracker() {
#ifdef HAVE_OCULUS_API
	ovrHmd_Destroy(hmd);
	ovr_Shutdown();
	// pSensor.Clear();
	// pHMD.Clear();
	// pManager.Clear();
	// delete pFusionResult;
	// OVR::System::Destroy();
#endif
}

bool OculusTracker::isGood() const {
#ifdef HAVE_OCULUS_API
	return hmd != NULL; // pSensor.GetPtr() != NULL;
#else
	return false;
#endif
}

void OculusTracker::report() const {
}

void OculusTracker::update() {
#ifdef HAVE_OCULUS_API
	bool usePredicted = false;
	OVR::Quatf quaternion;

	double predictionTime = 0.00;
	if (usePredicted)
		predictionTime = 0.000; // 20 milliseconds
	ovrSensorState sensorState = ovrHmd_GetSensorState(hmd, predictionTime);
	if (sensorState.StatusFlags & (ovrStatus_OrientationTracked) ) {
		ovrPosef pose = sensorState.Predicted.Pose;
		quaternion = pose.Orientation;
	}

	/*
	if (usePredicted)
		quaternion = pFusionResult->GetPredictedOrientation();
	else
		quaternion = pFusionResult->GetOrientation();
	*/

	// Compress head tracking orientation in Y, to compensate for Doom pixel aspect ratio
	/* */
	if (true) { // one aspect of aspect ratio correction
		const float pixelRatio = 1.20;
		OVR::Vector3<float> axis;
		float angle;
		quaternion.GetAxisAngle(&axis, &angle);
		axis.y *= 1.0f/pixelRatio; // 1) squish direction in Y
		axis.Normalize();
		float angleFactor = 1.0f + sqrt(1.0f - axis.y*axis.y) * (pixelRatio - 1.0f);
		angle = atan2(angleFactor * sin(angle), cos(angle)); // 2) Expand angle in Y
		OVR::Quatf squishedQuat(axis, angle);
		squishedQuat.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&yaw, &pitch, &roll);
	}
	/* */
	else {
		quaternion.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&yaw, &pitch, &roll);
	}
#endif
}