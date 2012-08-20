/*
 * SceneManager.cpp
 *
 *  Created on: Aug 8, 2012
 *      Author: fsl
 */

#include "SceneManager.h"

SceneManager::SceneManager() {
	for (int i=0; i<20; i++)
	{
		this->myScenes[i]=0;
	}

}

SceneManager::~SceneManager() {
	// TODO Auto-generated destructor stub
}

void SceneManager::createScene(vector3d_f rot, vector3d_f trans, uint frames, Obj3d * obj){
	uint index;
	for (index=0; index<20; index++)
	{
		if (this->myScenes[index]==0)
			break;
	}
	if (frames==0)
		frames=1;

	myScenes[index] = new Scene;
	myScenes[index]->frames = frames;
	myScenes[index]->passedFrames = 65535;
	myScenes[index]->vecRotation = rot;
	myScenes[index]->vecTranslation = trans;
	myScenes[index]->obj3d = obj;

}

void SceneManager::startScene(uint scene){
	if (myScenes[scene]!=0)
	myScenes[scene]->passedFrames = 0;
}

void SceneManager::animate(float * matMV){
	uint i = 0;
	while(myScenes[i]!=0)
	{
		if(setScene(i, matMV)) printf("end! scene: %i\n", i);
		i++;
	}

}

bool SceneManager::setScene(uint scene, float * matMV){

	if (myScenes[scene]->passedFrames <= myScenes[scene]->frames){
		//printf("passed frames: %i, frames %i \n", myScenes[scene]->passedFrames, myScenes[scene]->frames );
		vector3d_f trans, rot;
		if (myScenes[scene]->frames-myScenes[scene]->passedFrames!=0){
			trans.x = (myScenes[scene]->vecTranslation.x-matMV[12])/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			trans.y = (myScenes[scene]->vecTranslation.y-matMV[13])/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			trans.z = (myScenes[scene]->vecTranslation.z-matMV[14])/(myScenes[scene]->frames-myScenes[scene]->passedFrames);

			rot.x = (myScenes[scene]->vecRotation.x - (asin(matMV[9]) * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			rot.y = (myScenes[scene]->vecRotation.y - (asin(matMV[8]) * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			rot.z = (myScenes[scene]->vecRotation.z - (asin(matMV[1]) * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);

			fslTranslateMatrix4x4(matMV, trans.x, trans.y, trans.z);

			//handle rotation
			fslRotateMatrix4x4(matMV, rot.x, FSL_X_AXIS);
			fslRotateMatrix4x4(matMV, rot.y, FSL_Y_AXIS);
			fslRotateMatrix4x4(matMV, rot.z, FSL_Z_AXIS);
			//handle translation

		}
		//if animation completed, return done, reset scene
		if(myScenes[scene]->passedFrames == myScenes[scene]->frames){
			//myScenes[scene]->passedFrames = 0;
			myScenes[scene]->passedFrames++;
			printf("true!!!!!!!!!!!\n");
			return true;
		}
		myScenes[scene]->passedFrames++;
		printf("scene: %i, frames: %i, passed: %i\n",scene, myScenes[scene]->frames,myScenes[scene]->passedFrames);
	}
	return false;
}
