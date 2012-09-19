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
	//TODO check for rotation angles to be between 0 and 359, positive.
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
		vector3d_f trans, angles;
		float rot[6];
		if (myScenes[scene]->frames-myScenes[scene]->passedFrames!=0){
			trans.x = (myScenes[scene]->vecTranslation.x-matMV[12])/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			trans.y = (myScenes[scene]->vecTranslation.y-matMV[13])/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			trans.z = (myScenes[scene]->vecTranslation.z-matMV[14])/(myScenes[scene]->frames-myScenes[scene]->passedFrames);

			getEulerAnglesFromMVMatrix(myScenes[scene]->obj3d->getRotMat(), rot);
			angles.x = (myScenes[scene]->vecRotation.x - (rot[0] * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			angles.y = (myScenes[scene]->vecRotation.y - (rot[1] * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			angles.z = (myScenes[scene]->vecRotation.z - (rot[2] * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);

			fslTranslateMatrix4x4(matMV, trans.x, trans.y, trans.z);
			printf("frame %i-----------\n", myScenes[scene]->passedFrames);
			printf("current rot1: x%f, y%f, z%f\n", rot[0] * 57.295779513, rot[1]* 57.295779513, rot[2]* 57.295779513);
			printf("current rot2: x%f, y%f, z%f\n", rot[3] * 57.295779513, rot[4]* 57.295779513, rot[5]* 57.295779513);
			printf("to rotate: x%f, y%f, z%f\n", angles.x, angles.y, angles.z);

			//handle rotation

			fslRotateMatrix4x4(myScenes[scene]->obj3d->getRotMat(), angles.x, FSL_X_AXIS);
			fslRotateMatrix4x4(myScenes[scene]->obj3d->getRotMat(), angles.y, FSL_Y_AXIS);
			fslRotateMatrix4x4(myScenes[scene]->obj3d->getRotMat(), angles.z, FSL_Z_AXIS);
			getEulerAnglesFromMVMatrix(myScenes[scene]->obj3d->getRotMat(), rot);
			printf("rotated1: x%f, y%f, z%f\n", rot[0]* 57.295779513, rot[1]* 57.295779513, rot[2]* 57.295779513);
			printf("rotated2: x%f, y%f, z%f\n\n", rot[3]* 57.295779513, rot[4]* 57.295779513, rot[5]* 57.295779513);
			//handle translation

		}
		//if animation completed, return done, reset scene
		if(myScenes[scene]->passedFrames == myScenes[scene]->frames){
			//myScenes[scene]->passedFrames = 0;
			myScenes[scene]->passedFrames++;
			return true;
		}
		myScenes[scene]->passedFrames++;
		//printf("scene: %i, frames: %i, passed: %i ",scene, myScenes[scene]->frames,myScenes[scene]->passedFrames);
		//printf("x[9]: %f, y[8]: %f, z[1]: %f \n",  (asin(myScenes[scene]->obj3d->getRotMat()[9]) * 57.295779513), (asin(myScenes[scene]->obj3d->getRotMat()[8]) * 57.295779513),  (asin(myScenes[scene]->obj3d->getRotMat()[1]) * 57.295779513));
		//printf("0: %f, 1: %f, 2: %f\n",myScenes[scene]->obj3d->getRotMat()[0],myScenes[scene]->obj3d->getRotMat()[1], myScenes[scene]->obj3d->getRotMat()[2]);
	}
	return false;
}
