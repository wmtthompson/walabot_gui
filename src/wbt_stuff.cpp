/*
 * wbt_stuff.cpp
 *
 *  Created on: Sep 6, 2019
 *      Author: william
 */

#include "wbt_stuff.h"
//#include "WalabotAPI.h
#include <stdio.h>
#include <string>
#include <iostream>
#include <QTimer>
#include <QDebug>
#include <QStateMachine>
#include <QState>
#include <QFinalState>
#include <unistd.h>
using namespace std;

static const char * EnumStrings[] = {"CLEAN", "INITIALIZED", "CONNECTED", "CONFIGURED", "SCANNING", "CALIBRATING", "CALIBRATING_NO_MOVEMENT"};

const char * getTextForEnum(int enumVal)
{
	return EnumStrings[enumVal];
}

void PrintSensorTargets(SensorTarget* targets, int numTargets)
{
	int targetIdx;

printf("\033[2J\033[1;1H");



	if (numTargets > 0)
	{
		for (targetIdx = 0; targetIdx < numTargets; targetIdx++)
		{
			printf("Target #%d: \nX = %lf \nY = %lf \nZ = %lf \namplitude = %lf\n\n\n ",
					targetIdx,
					targets[targetIdx].xPosCm,
					targets[targetIdx].yPosCm,
					targets[targetIdx].zPosCm,
					targets[targetIdx].amplitude);
		}
	}
	else
	{
		printf("No target detected\n");
	}
}
//TODO: Fill out all the state machine code and make new methods
//TODO: Reference the following https://api.walabot.com/_sample.html
//TODO: Each method will process its part and then emit a signal when its done,
//TODO: each signal can be connected to a state so that the signal triggers the appropriate state.
ExpObj::ExpObj(QObject *parent):QObject(parent)
{
	myvar = 1;
	appStatus = STATUS_CLEAN;
	res = WALABOT_RESULT::WALABOT_ERROR;

}


void ExpObj::start()
{
	machine = new QStateMachine();

	//Connected state is reached after getting the connected signal
	connected_state = new QState();

	//Disconnected is the first state
	disconnected_state = new QState();

	// profile set state is reached after getting a signal that the profile has been set
	profile_set_state = new QState(connected_state);

	// profile not set is the entry point after the connected state
	profile_not_set_state = new QState(connected_state);
	connected_state->setInitialState(profile_not_set_state);

	// walabot stopped is the initial state from profile set state
	walabot_started_state = new QState(profile_set_state);

	// walabot stopped state is the entry point after profile is set
	walabot_stopped_state = new QState(profile_set_state);
	profile_set_state->setInitialState(walabot_stopped_state);

	processing_data_state = new QState(walabot_started_state);

	// calibrating will be the entry point of the walabot started state
	calibrating_state = new QState(walabot_started_state);
	walabot_started_state->setInitialState(calibrating_state);

	machine->addState(connected_state);
	machine->addState(disconnected_state);
	machine->setInitialState(disconnected_state);

	// when entering final state, a signal will be emitted.
	final_state = new QFinalState();
	machine->addState(final_state);

	//disconnected state transitions to started state on sig_connected
	disconnected_state->addTransition(this, &ExpObj::sig_connected, connected_state);
	//disconnected state can transition to final state if a terminate signal is received
	disconnected_state->addTransition(this, &ExpObj::sig_finished, final_state);

	//on entry into connected state, the child state should automatically be initiated,
	//the child state is profile not set state
	connected_state->addTransition(this, &ExpObj::sig_disconnected, disconnected_state);

	//the profile not set state transitions to the profile set state on signal that profile is set
	profile_not_set_state->addTransition(this, &ExpObj::sig_profile_set, profile_set_state);
	//on entry into the profile not set state, slot will be called to set the profile
	connect(this->profile_not_set_state, &QState::entered, this, &ExpObj::wlbt_set_profile); //TODO: define the wlbt set profile slot

	//the profile set state transitions to the profile not set state on signal to change profile
	profile_set_state->addTransition(this, &ExpObj::sig_change_profile, profile_not_set_state);
	// the initial state set from the profile set state will be walabot stopped state.
	connect(this->walabot_stopped_state, &QState::entered, this, &ExpObj::wlbt_start); //TODO: define the wlbt start slot

	//the walabot stopped state transitions to started state when the signal for walabot started is received.
	walabot_stopped_state->addTransition(this, &ExpObj::sig_walabot_started, walabot_started_state);

	//calibrating state transitions to processing data state when the scanning signal is received.
	calibrating_state->addTransition(this, &ExpObj::sig_scanning, processing_data_state);

	//on entering the calibrating state, the calibrating slot is called
	connect(this->calibrating_state, &QState::entered, this, &ExpObj::wlbt_calibrate);

	//processing data state transitions to the calibrating state when the calibrate signal is received.
	processing_data_state->addTransition(this, &ExpObj::sig_calibrate, calibrating_state);

	//on entering the processing data state, the image processing slot will be called.
	connect(this->processing_data_state, &QState::entered, this, &ExpObj::wlbt_image_processing);
	machine->start();

	//need to initialize walabot through the event loop to make sure that the event loop is running
	QTimer::singleShot(1, this, &ExpObj::wlbt_initialize);

}
//void ExpObj::trigger()
//{
//	//	5) Trigger: Scan(sense) according to profile and record signals to be
//	//	available for processing and retrieval.
//	//	====================================================================
//	res = Walabot_Trigger();
//	CHECK_WALABOT_RESULT(res, "Walabot_Trigger");
//
//	while(appStatus != 4)
//	{
//		res = Walabot_Trigger();
//		CHECK_WALABOT_RESULT(res, "Walabot_Trigger");
//		res = Walabot_GetStatus(&appStatus, &calibrationProcess);
//		CHECK_WALABOT_RESULT(res, "Walabot_GetStatus");
//		qDebug() << "Calibration progress = " << calibrationProcess;
//		qDebug() << "Status = " << getTextForEnum(appStatus);
////		usleep(3000000);
//		//		count += 1;
//	}
//	if (res == WALABOT_SUCCESS)
//	{
//		//emit signal that triggering was a success
//		qDebug() << "Walabot Triggered";
//		emit sig_triggered();
//	}
//}

void ExpObj::wlbt_set_profile()
{
	qDebug() << "Beginning Walabot Configuration";
	//  2) Configure : Set scan profile and arena
	//	=========================================

	// Set Profile - to Sensor.
	//			Walabot recording mode is configured with the following attributes:
	//			-> Distance scanning through air;
	//			-> high-resolution images
	//			-> slower capture rate
	res = Walabot_SetProfile(PROF_SHORT_RANGE_IMAGING);
	CHECK_WALABOT_RESULT(res, "Walabot_SetProfile");

	//For short range imaging
	// Set arena by Cartesian coordinates, with arena resolution :
	// ------------------------
	// Initialize configuration
	// ------------------------
	double zArenaMin = 3;
	double zArenaMax = 8;
	double zArenaRes = 0.5;

	double xArenaMin = -3;
	double xArenaMax = 4;
	double xArenaRes = 0.5;

	double yArenaMin = -6;
	double yArenaMax = 4;
	double yArenaRes = 0.5;
	res = Walabot_SetArenaX(xArenaMin, xArenaMax, xArenaRes);
	CHECK_WALABOT_RESULT(res, "Walabot_SetArenaX");

	res = Walabot_SetArenaY(yArenaMin, yArenaMax, yArenaRes);
	CHECK_WALABOT_RESULT(res, "Walabot_SetArenaY");

	res = Walabot_SetArenaZ(zArenaMin, zArenaMax, zArenaRes);
	CHECK_WALABOT_RESULT(res, "Walabot_SetArenaZ");

	// Walabot filtering disable
	res = Walabot_SetDynamicImageFilter(FILTER_TYPE_NONE);
	CHECK_WALABOT_RESULT(res, "Walabot_SetDynamicImageFilter");

	// Setup arena - specify it by Cartesian coordinates(ranges and resolution on the x, y, z axes);
	//	In Sensor mode there is need to specify Spherical coordinates(ranges and resolution along radial distance and Theta and Phi angles).
	//Turn this back on for sensor targets
	//	res = Walabot_SetArenaR(minInCm, maxInCm, resICm);
	//	CHECK_WALABOT_RESULT(res, "Walabot_SetArenaR");
	//
	//	// Sets polar range and resolution of arena (parameters in degrees).
	//	res = Walabot_SetArenaTheta(minIndegrees, maxIndegrees, resIndegrees);
	//	CHECK_WALABOT_RESULT(res, "Walabot_SetArenaTheta");
	//
	//	// Sets azimuth range and resolution of arena.(parameters in degrees).
	//	res = Walabot_SetArenaPhi(minPhiInDegrees, maxPhiInDegrees, resPhiInDegrees);
	//	CHECK_WALABOT_RESULT(res, "Walabot_SetArenaPhi");
	//
	//	FILTER_TYPE filterType = mtiMode ?
	//			FILTER_TYPE_MTI :		//Moving Target Identification: standard dynamic-imaging filter
	//			FILTER_TYPE_NONE;
	//
	//	res = Walabot_SetDynamicImageFilter(filterType);
	//	CHECK_WALABOT_RESULT(res, "Walabot_SetDynamicImageFilter");

	if(res == WALABOT_SUCCESS)
	{
		qDebug() << "Walabot Configured";
		emit sig_profile_set();
	}
	else
	{
		qDebug() << "Walabot profile and configuration failed.";
		//TODO: Figure out what to do in case of configuration failed.
	}
}


void ExpObj::wlbt_calibrate()
{
	//  4) Start Calibration - only if MTI mode is not set - (there is no sense
	//	executing calibration when MTI is active)
	//	========================================================================
	// TODO: Look into MTI Mode
	qDebug() << "Starting Calibration";
	res = Walabot_StartCalibration();
	CHECK_WALABOT_RESULT(res, "Walabot_StartCalibration");
	res = Walabot_Trigger();
	CHECK_WALABOT_RESULT(res, "Walabot_Trigger");

	while(appStatus != 4)
	{
		res = Walabot_GetStatus(&appStatus, &calibrationProcess);
		CHECK_WALABOT_RESULT(res, "Walabot_GetStatus");
		qDebug() << "Calibration progress = " << calibrationProcess;
		qDebug() << "Status = " << getTextForEnum(appStatus);
		res = Walabot_Trigger();
		CHECK_WALABOT_RESULT(res, "Walabot_Trigger");
	}


	if(res == WALABOT_SUCCESS & appStatus == 4)
	{
		qDebug() << "Walabot Calibrated";
		emit sig_scanning();
	}
	else
	{
		qDebug() << "Walabot calibration failed.";
		//TODO: Figure out what to do if calibration fails.
	}
}

void ExpObj::wlbt_image_processing()
{

	res = Walabot_Trigger();
	CHECK_WALABOT_RESULT(res, "Walabot_Trigger");
	res = Walabot_GetRawImageSlice(&rasterImage, &sizeX, &sizeY, &sliceDepth, &power);
	CHECK_WALABOT_RESULT(res, "Walabot_GetRawImageSlice");

	if(res == WALABOT_SUCCESS)
	{
		emit sig_display_image(rasterImage, sizeX, sizeY);
//		emit sig_display_data(raster3dImage, sizeX, sizeY, sizeZ, power);
	}
	else
	{
		qInfo() << "Walabot trigger or get image was not succesful, possible failure,TODO stopping walabot.";
		//TODO: call walabot stop method or slot here.
	}

}


//void ExpObj::wlbt_stop()
//{
//	//	7) Stop and Disconnect.
//	//	======================
//	qDebug() << "Attempting to Stop walabot.";
//	res = Walabot_Stop();
//	CHECK_WALABOT_RESULT(res, "Walabot_Stop");
//
//	res = Walabot_Disconnect();
//	CHECK_WALABOT_RESULT(res, "Walabot_Disconnect");
//
//	Walabot_Clean();
//	CHECK_WALABOT_RESULT(res, "Walabot_Clean");
//
//	if(res == WALABOT_SUCCESS)
//	{
//		qDebug() << "Walabot Stopped";
//		emit sig_stopped();
//	}
//
//}

void ExpObj::wlbt_start()
{
	res = Walabot_Start();
	CHECK_WALABOT_RESULT(res, "Walabot_Start");
	if(res == WALABOT_SUCCESS)
	{
		qDebug() << "Started Success!";
		emit sig_walabot_started();
	}
	else
	{
		qDebug() << "Start failed"; //TODO: Deal with start failure
	}
}

void ExpObj::wlbt_initialize()
{
	res = Walabot_Initialize(CONFIG_FILE_PATH);
	CHECK_WALABOT_RESULT(res, "Walabot_Initialize");

	//	1) Connect : Establish communication with Walabot.
	//	==================================================
	res = Walabot_ConnectAny();
	CHECK_WALABOT_RESULT(res, "Walabot_ConnectAny");

	if(res == WALABOT_SUCCESS)
	{
		qDebug() << "Connected Success!";
		qDebug() << "State Machine Running? = " << machine->isRunning();
		//		QTimer::singleShot(0, this, &ExpObj::configure);
		emit sig_connected();
	}
	else
	{
		qDebug() << "Connect Failed"; //TODO: Deal with connection failure.
//		emit sig_connect_failed();
	}
}


