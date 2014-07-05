#include <train.h>


typedef struct trainStatus{
	int 	previousTrainSpeed;
	int 	currentTrainSpeed;
	
	int		lastSpeedChange; 		//Time();
	
	int 	trainDirection;
}trainStatus;

typedef struct trackServerData{
	float 		trainsActualSpeeds[2][14];
	
	trainStatus trainsStatus[MAX_TRAINS];
	int			switchesStatus[MAX_SWITCHES];
	int 		lastTriggeredSensor;
	int 		initTrainNum;
}trackServerData;


static void
initTrackServerData(trackServerData* trkSvrData){
	int i, j;
	
	trkSvrData->trainsActualSpeeds[0][7] = 38.269; 
	trkSvrData->trainsActualSpeeds[0][8] = 42.781;
	
	
	for (i = 0; i < MAX_TRAINS; i++)
	{
		(trkSvrData->trainsStatus[i]).previousTrainSpeed = 0;
		(trkSvrData->trainsStatus[i]).currentTrainSpeed  = 0;
		
		(trkSvrData->trainsStatus[i]).lastSpeedChange  = 0;
		
		(trkSvrData->trainsStatus[i]).trainDirection	= FORWARD;
		
		putc(COM1, 0); 
		putc(COM1, i + 45);
	}
	for (i = 0; i < MAX_SWITCHES; i++)
	{
		if (i <= 17)
		{
			(trkSvrData->switchesStatus[i]) = CURVED;
			
			putc(COM1, CURVED);
			putc(COM1, i + 1);
		}
		else if (i == 18 || i == 20)
		{
			(trkSvrData->switchesStatus[i]) = CURVED;
			
			putc(COM1, CURVED);
			putc(COM1, i + 135);			
		}
		else if (i == 19 || i == 21)
		{
			(trkSvrData->switchesStatus[i]) = STRAIGHT;
			
			putc(COM1, STRAIGHT);
			putc(COM1, i + 135);			
		}
		putc(COM1, 32);				
	}

	trkSvrData->initTrainNum = -1;
	trkSvrData->lastTriggeredSensor = -1;
}

void
trackServer(){
	trackServerData trkSvrData;
	
	int requester;
	trackServerRequest req;
	
	putc(COM1, 96);	
	initTrackServerData(&trkSvrData);
	
	sprintf(COM2, "\033[5;70H%d", (int)trainsActualSpeeds[0][7]);
	
	while (1)
	{
		Receive(&requester, &req, sizeof(trackServerRequest));
		
		switch (req.trkSvrReq_uid)
		{
			case TRACKSERVER_INIT_TRAIN:
			{
				trkSvrData.initTrainNum = req.target;
				Reply(requester, NULL, 0);
			}
			case TRACKSERVER_SPEED_GET:
			{			
				Reply(requester, &(trkSvrData.trainsStatus[req.target-45].currentTrainSpeed), sizeof(int));
				break;
			}
			case TRACKSERVER_SPEED_CHANGE:
			{
				trainStatus *targetTrainStatus = &(trkSvrData.trainsStatus[req.target-45]);

				targetTrainStatus->previousTrainSpeed = targetTrainStatus->currentTrainSpeed;
				targetTrainStatus->lastSpeedChange = Time();
				targetTrainStatus->currentTrainSpeed = req.value % 15;

				sprintf(COM2, "\033[5;70H%d", req.value % 15);
				
				// if (req.value == 15){
				// 	targetTrainStatus->trainDirection = 
				// 		(targetTrainStatus->trainDirection == FORWARD) ? BACKWARD : FORWARD;
				// }
							
				Reply(requester, NULL, 0);									
				break;		
			}	
			case TRACKSERVER_SWITCH_GET:
			{
				if(req.target <= 18)
					Reply(requester, &(trkSvrData.switchesStatus[req.target-1]), sizeof(int));
				else
					Reply(requester, &(trkSvrData.switchesStatus[req.target-135]), sizeof(int));					
				break;	
			}
			case TRACKSERVER_SWITCH_CHANGE:
			{
				if(req.target <= 18)
					trkSvrData.switchesStatus[req.target-1] = req.value;
				else
					trkSvrData.switchesStatus[req.target-135] = req.value;
					
				Reply(requester, NULL, 0);									
				break;	
			}
			case TRACKSERVER_REVERSE:
			{
				if (trkSvrData.trainsStatus[req.target-45].trainDirection == FORWARD){
					trkSvrData.trainsStatus[req.target-45].trainDirection = BACKWARD;
					sprintf(COM2, "\033[4;70HBackward");
				}
				else{
					trkSvrData.trainsStatus[req.target-45].trainDirection = FORWARD;
					sprintf(COM2, "\033[4;70HForward");
				}
				
				Reply(requester, NULL, 0);
			}
			case TRACKSERVER_UPDATE_LAST_SENSOR:
			{
				trkSvrData.lastTriggeredSensor = req.value;
				int n = trkSvrData.initTrainNum;
				if (n != -1 && req.value == ('B' * 17 + 2)){
					putc(COM1, 0);
					putc(COM1, n);
					
					Delay(100);

					putc(COM1, 15);
					putc(COM1, n);
					
					putc(COM1, trkSvrData.trainsStatus[n-45].currentTrainSpeed);
					putc(COM1, n);
					trkSvrData.initTrainNum = -1;
				}
				else if (n != -1 && req.value == ('D' * 17 + 14)){
					trkSvrData.initTrainNum = -1;
				}
				Reply(requester, NULL, 0);
			}	
		}
	}
}

int
getTrainSpeed(int trainNo){
	trackServerRequest req;
		
	req.trkSvrReq_uid = TRACKSERVER_SPEED_GET;
		
	req.target = trainNo;
	
	Send(TRACKSERVER_TID, &req, sizeof(trackServerRequest), &(req.retval), sizeof(int));	
	return req.retval;
}
void 
changeTrainSpeed(int trainNo, int trainSpeed){
	trackServerRequest req;
	
	putc(COM1, trainSpeed);
	putc(COM1, trainNo);
	
	req.trkSvrReq_uid = TRACKSERVER_SPEED_CHANGE;
		
	req.target 	= trainNo;
	req.value	= trainSpeed;
	
	Send(TRACKSERVER_TID, &req, sizeof(trackServerRequest), NULL, 0);	
}

void
reverseTrain(int trainNo){
	trackServerRequest req;

	putc(COM1, 0);								//soft-stop
	putc(COM1, trainNo);
	
	//Delay(200);									//delay 2s

	putc(COM1, 15);								//change direction and hard-stop
	putc(COM1, trainNo);
	
	putc(COM1, getTrainSpeed(trainNo));			//go back with prev speed
	putc(COM1, trainNo);

	req.trkSvrReq_uid = TRACKSERVER_REVERSE;
	req.target = trainNo;
	Send(TRACKSERVER_TID, &req, sizeof(trackServerRequest), NULL, 0);
}

int
getSwitchStatus(int switchNo){
	trackServerRequest req;
		
	req.trkSvrReq_uid = TRACKSERVER_SWITCH_GET;
		
	req.target = switchNo;
	
	Send(TRACKSERVER_TID, &req, sizeof(trackServerRequest), &(req.retval), sizeof(int));	
	
	return req.retval;
}
void
changeSwitchStatus(int switchNo, int switchStatus){
	trackServerRequest req;
	
	putc(COM1,switchStatus);
	putc(COM1,switchNo);
	putc(COM1, 32);
	
	req.trkSvrReq_uid = TRACKSERVER_SWITCH_CHANGE;
		
	req.target 	= switchNo;
	req.value	= switchStatus;
	
	Send(TRACKSERVER_TID, &req, sizeof(trackServerRequest), NULL, 0);	
}

void
updateLastTriggeredSensor(int sensorUID){
	trackServerRequest req;
	
	req.trkSvrReq_uid = TRACKSERVER_UPDATE_LAST_SENSOR;
		
	req.target 	= NULL;
	req.value	= sensorUID;
	
	Send(TRACKSERVER_TID, &req, sizeof(trackServerRequest), NULL, 0);
}

void 
initTrain(int trainNo){

	changeTrainSpeed(trainNo, 8);

	trackServerRequest req;
	
	req.trkSvrReq_uid = TRACKSERVER_INIT_TRAIN;	
	req.target 	= trainNo;

	Send(TRACKSERVER_TID, &req, sizeof(trackServerRequest), NULL, 0);
}
