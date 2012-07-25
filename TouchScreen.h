#ifndef _TOUCHSCREEN_H
#define _TOUCHSCREEN_H

typedef struct {
unsigned short x;   //current x
unsigned short y;	//current x
unsigned short fx;	//x recorded after first touch
unsigned short fy;	//y recorded after first touch
short delta_x;		// x - fx, during touch 
short delta_y;		// y - fy, during touch
unsigned char tap;  // set after release. Sticky value set by driver and reset by application 
unsigned char down; // Touch down
}Touch_s;

extern void InitTouch();
extern void killTouch();
extern int updateTouch(void);
extern float MultiTouchScaleFactor(unsigned short width, unsigned short height);
extern float MultiTouchRotation();
extern Touch_s mTouch[2];
extern int runTouch(float &Xrot, float &Yrot, float &Zrot, float &Zoom, int width, int height);

#endif
