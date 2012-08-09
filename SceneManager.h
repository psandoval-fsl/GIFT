/*
 * SceneManager.h
 *
 *  Created on: Aug 8, 2012
 *      Author: fsl
 */

#ifndef SCENEMANAGER_H_
#define SCENEMANAGER_H_

#include "obj3d.h"
#include "fslutil.h"

typedef	struct Scene_{
		Obj3d * obj3d;
		vector3d_f vecTranslation;
		vector3d_f vecRotation;
		uint frames;
		uint passedFrames;
		float desiredMatMV[16];
	}Scene;

class SceneManager {

	Scene * myScenes[20];

public:
	SceneManager();
	virtual ~SceneManager();

	uint createScene(vector3d_f rot, vector3d_f trans, uint frames, Obj3d * obj);
	bool setScene(uint scene, float * matMV);

};

#endif /* SCENEMANAGER_H_ */
