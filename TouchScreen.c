
/**
  Copyright (c) 2009 Freescale Semiconductor
* 
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
* Auth: D.Bogavac 2011-12-04
* Touch screen driver for Hannstar 10.1" capacitive touch.
* Could not find description for the output so I did some reverse engineering and guessing.
* This is a "hack" and should be treated as that
*/

#include <stdint.h>

#include <linux/input.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include "TouchScreen.h"

#ifndef EV_SYN
#define EV_SYN 0
#endif

#define DWIDTH	1024
#define DHEIGHT	768


struct input_event devData[11];
Touch_s mTouch[2];
Touch_s mTouch_l[2];
char firstTouch[2];
int tsDev = 0;
int tsCH = 0;
int tsCounts = 0;

/*-------------------------------------------------*/
/* Opens the touchscreen device */
/*-------------------------------------------------*/
void InitTouch(){

 // open framebuffer device and read out info

 tsDev = open("/dev/input/ts0", O_RDONLY | O_NONBLOCK);
 if (tsDev < 0) printf("device open failed");
}  
/*-------------------------------------------------*/
/* Closes the touchscreen device */
/*-------------------------------------------------*/

void killTouch(void){
	close(tsDev);
}

/*----------------------------------------------------------------*/
/* Reads the touch screen device and updates the touch data struct*/
/*----------------------------------------------------------------*/

int updateTouch(void){
	int amount;
	unsigned int i;
	//fseek(tsDev, 0, SEEK_END);
	amount = read(tsDev, devData, sizeof(struct input_event) * 11);	
	if (amount < (int) sizeof(struct input_event)) {
		if (amount > 0) printf("Device read error\n");
		return 1;
	}
	//if (amount < 0x10) printf("got %d bytes\n",amount);
	for (i = 0; i < amount / sizeof(struct input_event); i++){
		if (devData[i].type){
			switch (devData[i].code){

				case 0x39: // code ch no. 
					tsCH = devData[i].value;
					tsCounts++;
				break;
				case 0x30: // code ch down/up
					if((mTouch[tsCH].down == 0) && (devData[i].value))  
						firstTouch[tsCH] = 2;
					//else firstTouch[tsCH] = 0;
					mTouch_l[tsCH].down = devData[i].value;
				break;
				case 0x35: // code x value
					mTouch_l[tsCH].x = devData[i].value;
				break;
				case 0x36: // code y value
					mTouch_l[tsCH].y = devData[i].value;
				break;
			}
		}
		else {
			if (devData[i].code == 2) // Sync, 1 ch read done (Config sync)
			{
			}
			if (devData[i].code == 0) // Report sync. e.g. all active ch have been read
			{
				mTouch[0].x = mTouch_l[0].x / (0x8000/DWIDTH);
				mTouch[0].y = mTouch_l[0].y  / (0x8000/DHEIGHT);
				mTouch[1].x = mTouch_l[1].x / (0x8000/DWIDTH);
				mTouch[1].y = mTouch_l[1].y  / (0x8000/DHEIGHT);
				if ((mTouch[0].down == 0) && (mTouch_l[0].down == 1)) {
					mTouch[0].tap = 1;
					mTouch[0].fx = mTouch[0].x;
					mTouch[0].fy = mTouch[0].y;
				}
				if ((mTouch[1].down == 0) && (mTouch_l[1].down == 1)) {
					mTouch[1].tap = 1;
					mTouch[1].fx = mTouch[1].x;
					mTouch[1].fy = mTouch[1].y;
				}
				mTouch[0].down = mTouch_l[0].down;
				mTouch[1].down = mTouch_l[1].down;
				if (mTouch[0].down){
					mTouch[0].delta_x = mTouch[0].x - mTouch[0].fx;
					mTouch[0].delta_y = mTouch[0].y - mTouch[0].fy;
				}
				else mTouch[0].delta_x = mTouch[0].delta_y = 0;				

				if (mTouch[1].down){
					mTouch[1].delta_x = mTouch[1].x - mTouch[1].fx;
					mTouch[1].delta_y = mTouch[1].y - mTouch[1].fy;
				}
				else mTouch[1].delta_x = mTouch[1].delta_y = 0;				
			}
		}

	}
	return 0;
}

//--------------------------------------------------------------------------------------
// Name: MultiTouchScaleFactor()
// Desc: Returns a scale change factor. Ex. 0.2 = 20% larger and the negative value
// means 20% smaller. The scale is relative to the arguments width/height normally
// representing the screen res.
// Assumes that a Multitouch event is ongoing.  
//--------------------------------------------------------------------------------------

float MultiTouchScaleFactor(unsigned short width, unsigned short height){
	unsigned short x = 0,y = 0,fx = 0,fy = 0;
	//short dd = 0;
	float screenDiagonal, dd, result = 0.0;
	screenDiagonal = sqrt((width * width) + (height * height));
	if (mTouch[0].x > mTouch[1].x) x = mTouch[0].x - mTouch[1].x;
	else x = mTouch[1].x - mTouch[0].x;
	if (mTouch[0].y > mTouch[1].y) y = mTouch[0].y - mTouch[1].y;
	else y = mTouch[1].y - mTouch[0].y;

	if (mTouch[0].fx > mTouch[1].fx) fx = mTouch[0].fx - mTouch[1].fx;
	else fx = mTouch[1].fx - mTouch[0].fx;
	if (mTouch[0].fy > mTouch[1].fy) fy = mTouch[0].fy - mTouch[1].fy;
	else fy = mTouch[1].fy - mTouch[0].fy;

	dd = (sqrt((x * x) + (y * y))) - (sqrt((fx * fx) + (fy * fy)));
	result = dd / screenDiagonal;
	//printf("dd %d, screenDiagonal %d, result %f\n",dd, screenDiagonal, x);
	return result;
} 

float MultiTouchRotation(){
	#define OCTRAD 0.78539816339744f
	char octant_s, octant_c;
	unsigned short x = 0,y = 0,fx = 0,fy = 0;
	float angle_s, angle_c, angle_r;
	
	if (mTouch[0].x > mTouch[1].x) x = mTouch[0].x - mTouch[1].x;
	else x = mTouch[1].x - mTouch[0].x;
	if (mTouch[0].y > mTouch[1].y) y = mTouch[0].y - mTouch[1].y;
	else y = mTouch[1].y - mTouch[0].y;
	x /= 2;
	y /= 2;

	if (mTouch[0].fx > mTouch[1].fx) fx = mTouch[0].fx - mTouch[1].fx;
	else fx = mTouch[1].fx - mTouch[0].fx;
	if (mTouch[0].fy > mTouch[1].fy) fy = mTouch[0].fy - mTouch[1].fy;
	else fy = mTouch[1].fy - mTouch[0].fy;
	fx /= 2;
	fy /= 2;

	if ((mTouch[0].x > mTouch[1].x) && (mTouch[0].y > mTouch[1].y) && (x > y)) octant_c = 5;
	else if ((mTouch[0].x < mTouch[1].x) && (mTouch[0].y > mTouch[1].y) && (x > y)) octant_c = 2; 
	else if ((mTouch[0].x > mTouch[1].x) && (mTouch[0].y < mTouch[1].y) && (x > y)) octant_c = 6; 
	else if ((mTouch[0].x > mTouch[1].x) && (mTouch[0].y > mTouch[1].y) && (x < y)) octant_c = 4; 
	else if ((mTouch[0].x < mTouch[1].x) && (mTouch[0].y < mTouch[1].y) && (x > y)) octant_c = 1; 
	else if ((mTouch[0].x < mTouch[1].x) && (mTouch[0].y > mTouch[1].y) && (x < y)) octant_c = 3; 
	else if ((mTouch[0].x > mTouch[1].x) && (mTouch[0].y < mTouch[1].y) && (x < y)) octant_c = 7; 
	else octant_c = 0; 

	if ((mTouch[0].fx > mTouch[1].fx) && (mTouch[0].fy > mTouch[1].fy) && (fx > fy)) octant_s = 5;
	else if ((mTouch[0].fx < mTouch[1].fx) && (mTouch[0].fy > mTouch[1].fy) && (fx > fy)) octant_s = 2; 
	else if ((mTouch[0].fx > mTouch[1].fx) && (mTouch[0].fy < mTouch[1].fy) && (fx > fy)) octant_s = 6; 
	else if ((mTouch[0].fx > mTouch[1].fx) && (mTouch[0].fy > mTouch[1].fy) && (fx < fy)) octant_s = 4; 
	else if ((mTouch[0].fx < mTouch[1].fx) && (mTouch[0].fy < mTouch[1].fy) && (fx > fy)) octant_s = 1; 
	else if ((mTouch[0].fx < mTouch[1].fx) && (mTouch[0].fy > mTouch[1].fy) && (fx < fy)) octant_s = 3; 
	else if ((mTouch[0].fx > mTouch[1].fx) && (mTouch[0].fy < mTouch[1].fy) && (fx < fy)) octant_s = 7; 
	else octant_s = 0; 
	
	if ((octant_s == 5) || (octant_s == 2) || (octant_s == 6) || (octant_s == 1)) 
			angle_s = atan((float)fy /(float)fx);
	else angle_s = atan((float)fx /(float)fy);		

	if ((octant_c == 5) || (octant_c == 2) || (octant_c == 6) || (octant_c == 1)) 
			angle_c = atan((float)y /(float)x);
	else angle_c = atan((float)x /(float)y);		

	if ((octant_s == 1) || (octant_s == 3) || (octant_s == 5) || (octant_s == 7)) 
			angle_s = (OCTRAD * octant_s) + (OCTRAD - angle_s);
	else angle_s += OCTRAD * octant_s;		

	if ((octant_c == 1) || (octant_c == 3) || (octant_c == 5) || (octant_c == 7)) 
			angle_c = (OCTRAD * octant_c) + (OCTRAD - angle_c);
	else angle_c += OCTRAD * octant_c;		

	//printf("x %d, y%d, fx%d, fy%d, octant_s %d, octant_c %d, a_s%f, a_c%f\n", x, y,
	// fx,fy,octant_s,octant_c, angle_s, angle_c); 
	angle_r = angle_c - angle_s;
	if (angle_s < (4 * OCTRAD)){ 
		if (angle_r > (4 * OCTRAD)) return (angle_r - (8 * OCTRAD));
		else return angle_r; 
	}
	else {
		if (angle_r < 0.0f) {
			if (angle_r < (-4 * OCTRAD)) return (angle_r + (8 * OCTRAD) - angle_c);
			else return angle_r;
		}
		else return angle_r;
	}
}

int runTouch(float &Xrot, float &Yrot, float &Zrot, float &Zoom, int width, int height)
{
	static float zoomLatch = -2.0, zoom = -2.0f, Yrotation = 0.0f, zoomFactor = 0.0f, 
		XrotationLatch = 0.0f, rotationLatch = 0.0f, Zrotation = 0.0f, Xrotation = 0.0f;

	//Touch screen code	
	updateTouch();
	// first we check if we have multi touch
	if ((mTouch[0].down) && (mTouch[1].down)){
		zoomFactor = MultiTouchScaleFactor(width, height);
		Zrotation = MultiTouchRotation() * -57.2958;
		//printf("%f\n",Zrotation);
		zoom = zoomLatch + (zoomFactor * 230.0f);
		if (zoom < -230.0f) zoom = -230.0f;
		if (zoom > -2.0f) zoom = -2.0f; 
	}
	// and then if we have a single touch
	else if (mTouch[0].down){
		Yrotation = rotationLatch - (float)(mTouch[0].delta_x / 4.0f);
		Xrotation = XrotationLatch - (float)(mTouch[0].delta_y / 4.0f);
	}
	// and if neither then we have no touch
	else {
		if (zoomFactor != 0.0f) {
			zoomLatch = zoom;
			zoomFactor = 0.0f;
			mTouch[0].tap = 0; // invalidate the tap event because we used the touch action
		}
		if (Yrotation != rotationLatch){
			if ((mTouch[0].delta_x > 10) || (mTouch[0].delta_x < -10) || (mTouch[0].delta_y > 10) || 
			  (mTouch[0].delta_y < -10)) 
			    mTouch[0].tap = 0; // invalidate the tap event because touch moved more than 10 pixels
			rotationLatch = Yrotation;
		}
		if (Xrotation != XrotationLatch){
			if ((mTouch[0].delta_x > 10) || (mTouch[0].delta_x < -10) || (mTouch[0].delta_y > 10) || 
			  (mTouch[0].delta_y < -10)) 
			    mTouch[0].tap = 0; // invalidate the tap event because touch moved more than 10 pixels
			XrotationLatch = Xrotation;
		}
	}
	// if we have a tap screen event
	if (mTouch[0].tap){
			// check if tap occured in the bottom right corner
			if ((mTouch[0].fx > 900) && (mTouch[0].fy > 620)){
				//if (object) object = 0;
				//else object = 1;
				mTouch[0].tap = 0;
				return 2;
			}// if not there, what about lower left corner
			else if ((mTouch[0].fx < 100) && (mTouch[0].fy > 620)){
				mTouch[0].tap = 0;
				return 1;
			}//alright upper right then?
			else if ((mTouch[0].fx > 900) && (mTouch[0].fy < 50)){
				mTouch[0].tap = 0;
				return 3;
			}
			//upper left?
			else if ((mTouch[0].fx < 100) && (mTouch[0].fy < 50)){
				mTouch[0].tap = 0;
				return 4;
			}
		mTouch[0].tap = 0; //tap event consumed, reset it regardless if it's in the monitored area or not.
	}
	Xrot = Xrotation;
	Yrot = Yrotation/100;
	Zrot = Zrotation;
	Zoom = zoomFactor;
	return 0;
}
