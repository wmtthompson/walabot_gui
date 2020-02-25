/*
 * wbt_stuff.h
 *
 *  Created on: Sep 6, 2019
 *      Author: william
 */

#ifndef WBT_STUFF_H_
#define WBT_STUFF_H_

#include <QObject>
#include "WalabotAPI.h"
#include <iostream>

class QStateMachine;
class QState;
class QFinalState;

#define CONFIG_FILE_PATH "/etc/walabotsdk.conf"

#define CHECK_WALABOT_RESULT(result, func_name)					\
{																\
	if (result != WALABOT_SUCCESS)								\
	{															\
		const char* errorStr = Walabot_GetErrorString();		\
		std::cout << std::endl << func_name << " error: "       \
                  << errorStr << std::endl;                     \
		std::cout << "Press enter to continue ...";				\
		std::string dummy;										\
		std::getline(std::cin, dummy);							\
		return;													\
	}															\
}

void PrintSensorTarget(SensorTarget* targets, int numTargets);



class ExpObj: public QObject
{
	Q_OBJECT
public:
	// Walabot_GetRawImageSlice - output parameters
	int*	rasterImage = nullptr;
	int*	raster3dImage = nullptr;
	int		sizeX = 0;
	int		sizeY = 0;
	int		sizeZ = 0;
	double	sliceDepth = 0.0;
	double	power = 0.0;
	// Walabot_GetSensorTargets - output parameters
	SensorTarget* targets = nullptr;
	int numTargets = 0;
	APP_PROFILE wlbt_profile = PROF_SHORT_RANGE_IMAGING;
	ExpObj(QObject *parent=nullptr);
	~ExpObj(){}

public slots:
	void start(); // sets up the state machine, starts the state machine, then trys to connect to walabot.
	void wlbt_initialize();
	void wlbt_set_profile();
	void wlbt_start();
	void wlbt_calibrate();
	void wlbt_image_processing();


public:
	//get and set method for profile
	void set_profile(APP_PROFILE prof);
	APP_PROFILE get_profile();
	QFinalState *final_state = nullptr;


signals:
	void sig_connected();
	void sig_disconnected();
	void sig_profile_set();
	void sig_change_profile();
	void sig_walabot_started();
	void sig_walabot_stopped();
	void sig_scanning();
	void sig_calibrate();
	void sig_display_image(int* raster, int x, int y);
	void sig_display_data(int* raster3d, int x, int y, int z, double power);
	void sig_finished();

private:
	QStateMachine *machine = nullptr;
	QState *connected_state = nullptr;
	QState *disconnected_state = nullptr;
	QState *profile_set_state = nullptr;
	QState *profile_not_set_state = nullptr;
	QState *walabot_started_state = nullptr;
	QState *walabot_stopped_state = nullptr;
	QState *processing_data_state = nullptr;
	QState *calibrating_state = nullptr;


	qint32 myvar;
	WALABOT_RESULT res;

	// Walabot_GetStatus - output parameters
	APP_STATUS appStatus;
	double calibrationProcess = 0; // Percentage of calibration completed, if status is STATUS_CALIBRATING

	// Walabot_SetArenaR - input parameters
	double minInCm = 30;
	double maxInCm = 200;
	double resICm = 3;
	// Walabot_SetArenaTheta - input parameters
	double minIndegrees = -15;
	double maxIndegrees = 15;
	double resIndegrees = 5;
	// Walabot_SetArenaPhi - input parameters
	double minPhiInDegrees = -60;
	double maxPhiInDegrees = 60;
	double resPhiInDegrees = 5;
	bool mtiMode = false;
};



#endif /* WBT_STUFF_H_ */
