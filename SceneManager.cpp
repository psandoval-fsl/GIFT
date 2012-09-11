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

			rot.x = (myScenes[scene]->vecRotation.x - (asin(myScenes[scene]->obj3d->getRotMat()[9]) * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			rot.y = (myScenes[scene]->vecRotation.y - (asin(myScenes[scene]->obj3d->getRotMat()[8]) * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);
			rot.z = (myScenes[scene]->vecRotation.z - (asin(myScenes[scene]->obj3d->getRotMat()[1]) * 57.295779513))/(myScenes[scene]->frames-myScenes[scene]->passedFrames);

			fslTranslateMatrix4x4(matMV, trans.x, trans.y, trans.z);

			//handle rotation
			printf("before rot y: %f, %f degrees. \n",myScenes[scene]->obj3d->getRotMat()[8], rot.y);
			fslRotateMatrix4x4(myScenes[scene]->obj3d->getRotMat(), rot.x, FSL_X_AXIS);
			fslRotateMatrix4x4(myScenes[scene]->obj3d->getRotMat(), rot.y, FSL_Y_AXIS);
			fslRotateMatrix4x4(myScenes[scene]->obj3d->getRotMat(), rot.z, FSL_Z_AXIS);
			//TODO fix angle cornercases that make the model disappear.
			printf("after rot y: %f\n",myScenes[scene]->obj3d->getRotMat()[8]);
			printf("rad y[8]: %f \n \n",((asin(myScenes[scene]->obj3d->getRotMat()[8]))));
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
		//printf("scene: %i, frames: %i, passed: %i ",scene, myScenes[scene]->frames,myScenes[scene]->passedFrames);
		//printf("x[9]: %f, y[8]: %f, z[1]: %f \n",  (asin(myScenes[scene]->obj3d->getRotMat()[9]) * 57.295779513), (asin(myScenes[scene]->obj3d->getRotMat()[8]) * 57.295779513),  (asin(myScenes[scene]->obj3d->getRotMat()[1]) * 57.295779513));
		//printf("0: %f, 1: %f, 2: %f\n",myScenes[scene]->obj3d->getRotMat()[0],myScenes[scene]->obj3d->getRotMat()[1], myScenes[scene]->obj3d->getRotMat()[2]);
	}
	return false;
}
