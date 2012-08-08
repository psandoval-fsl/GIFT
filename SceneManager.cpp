/*
 * SceneManager.cpp
 *
 *  Created on: Aug 8, 2012
 *      Author: fsl
 */

#include "SceneManager.h"

SceneManager::SceneManager() {


}

SceneManager::~SceneManager() {
	// TODO Auto-generated destructor stub
}

Scene SceneManager::createScene(vector3d_f rot, vector3d_i trans, uint frames, Obj3d * obj){
	Scene scene;
		scene.frames = frames;
		scene.vecRotation = rot;
		scene.vecTranslation = trans;
		scene.obj3d = obj;
	return scene;
}

bool SceneManager::setScene(Scene scene, float * matMV){

	return false;
}
