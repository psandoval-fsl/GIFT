/*
 * OpenGL ES 2.0 Asset Import loader
 *
 * Draws models with phong shading
 * b20593@freescale.com
 */

/* TODO
 * 	->test transparency
 * 	->make tires non reflective
 * 	->fix textures
 * 	->shader program per mesh
 * 	->fix rotation
 *	->optimize
 *	->get more and fancier shaders
 *	->per mesh animation
 *	->load light(s) position(s)
 */

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <sys/time.h>

#include "SceneManager.h"

#include "TouchScreen.h"

int width  = 1024; //1280
int height = 768;  //1024
int frames = 0;

//needed for assImp. info stream
struct aiLogStream stream;

//native EGL calls
EGLDisplay		eglDisplay;
EGLSurface		eglSurface;

// Global Variables, shader handle and program handle
GLuint       g_hPShaderProgram   = 0;
GLuint       g_hSBShaderProgram   = 0;
GLuint       g_hTXShaderProgram   = 0;

GLuint       viewMatrixLoc		= 0;
GLuint       projMatrixLoc      = 0;

GLuint sbVMLoc, sbPMLoc, sbPosLoc, sbTxHandle;
GLuint sbVBO[2];

float matProj[16] = {0};
float matModelView[16] = {0};
float matSkyBox[16] = {0};

SceneManager * mySceneManager;
Obj3d * assets;

int preRender()
{

	// load and compiler vertex/fragment shaders.
	LoadShaders("resources/shaders/vs_phong.vert", "resources/shaders/fs_phong.frag", g_hPShaderProgram);
	LoadShaders("resources/shaders/vs_skybox.vert", "resources/shaders/fs_skybox.frag", g_hSBShaderProgram);
	LoadShaders("resources/shaders/vs_texture.vert", "resources/shaders/fs_texture.frag", g_hTXShaderProgram );

	//init assImp stream
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
	aiAttachLogStream(&stream);

	//write stream to log 
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
	aiAttachLogStream(&stream);
	InitTouch();
	ilInit(); //if you use textures, do this.

	if (g_hSBShaderProgram  != 0)	
	{
		sbPMLoc= glGetUniformLocation(g_hSBShaderProgram, "projMatrix");
		sbVMLoc= glGetUniformLocation(g_hSBShaderProgram, "viewMatrix");
		sbPosLoc = glGetAttribLocation(g_hSBShaderProgram, "positionSB");
		initSkybox(sbVBO, sbPosLoc);
	} else {
		printf("skybox shader program not compiled/linked\n");
		return 1;
	}
	if (g_hPShaderProgram  != 0)
	{
		projMatrixLoc= glGetUniformLocation(g_hPShaderProgram, "projMatrix");
		viewMatrixLoc= glGetUniformLocation(g_hPShaderProgram, "viewMatrix");
	} else {
		printf("phong shader program not compiled/linked\n");
		return 1;
	}
	if (g_hTXShaderProgram  != 0)
	{
		projMatrixLoc= glGetUniformLocation(g_hTXShaderProgram, "projMatrix");
		viewMatrixLoc= glGetUniformLocation(g_hTXShaderProgram, "viewMatrix");
	} else {
		printf("Texture shader program not compiled/linked\n");
		return 1;
	}

	// Build a perspective projection matrix. You should consider using ortho
	//instead of perspective for clusters.
	fslPerspectiveMatrix4x4 ( matProj, 70.f, 1.3333f, 0.1f, 200.f);

	//create scenes for animations
	mySceneManager = new SceneManager();
	vector3d_f rotation;
	vector3d_f translation;

	rotation.x=-25; rotation.y=85; rotation.z=0;
	translation.x=0; translation.y=-0.3; translation.z=-4;
	mySceneManager->createScene(rotation, translation, 100, assets); //scene 0

	rotation.x=-25; rotation.y=-85; rotation.z=0;
	translation.x=0; translation.y=-0.3; translation.z=-4;
	mySceneManager->createScene(rotation, translation, 200, assets); //scene 1

	rotation.x=0; rotation.y=0; rotation.z=0;
	translation.x=0; translation.y=-0.3; translation.z=-5;
	mySceneManager->createScene(rotation, translation, 10, assets); //scene 2

	mySceneManager->startScene(2);
	fslLoadIdentityMatrix4x4 (matModelView);
	//fslTranslateMatrix4x4 (matModelView, 0, -1.0f, -3.0f); //(0, -2, -10)
	return 0;

}

// sets matrices, renders objects
void Render(Obj3d *assets, float Xrot, float Yrot, float Zrot, float zoomtr)
{
	// Clear background.
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

	//uncomment to draw skybox/cubemap
	fslLoadIdentityMatrix4x4 (matSkyBox);
	fslRotateMatrix4x4(matSkyBox, 180, FSL_Z_AXIS);
	renderSkybox(sbTxHandle, g_hSBShaderProgram, sbVMLoc, sbPMLoc,
			matSkyBox, matProj, sbPosLoc, sbVBO);

	mySceneManager->animate(matModelView);

	//fslRotateMatrix4x4 (matModelView, -Zrot, FSL_Z_AXIS);
	//fslRotateMatrix4x4 (matModelView, Xrot, FSL_X_AXIS);
	//fslRotateMatrix4x4 (matModelView, -Yrot, FSL_Y_AXIS);
	//fslRotateMatrix4x4 (matModelView, 90, FSL_X_AXIS);
	
	assets->draw(matModelView, matProj, viewMatrixLoc, projMatrixLoc);
   
	// swap buffers
	glFlush();
	
}

void RenderCleanup(Obj3d *assets)
{
	// Clear background.
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   	eglSwapBuffers(eglDisplay, eglSurface);
	
	// free assImp scene resources
	aiReleaseImport(assets->getScene());

	// detach assImp log
	aiDetachAllLogStreams();
}



// Cleanup the shaders.
void DestroyShaders()
{
	glDeleteProgram(g_hPShaderProgram );
	glDeleteProgram(g_hSBShaderProgram );
	glDeleteProgram(g_hTXShaderProgram );
	glUseProgram(0);
}

/*******************************************************************************
********************************************************************************/

// Program entry.
int main(int argc, char** argv)
{
	int frameCount = 0;
	unsigned int start = fslGetTickCount();
//	unsigned int fpsStart = 0;
//	unsigned int fpsEnd = 0;
//	unsigned int miliseconds = 0;
	float Xrotation, Yrotation, Zrotation, zoom = 0;
	assets = new Obj3d(false);
	int touch;

	EGLinit(eglDisplay, eglSurface);

	if (1==preRender())
	{
		EGLdeinit(eglDisplay);
		return 1;
	}

	//assets->start(g_hTXShaderProgram, "resources/models/jeep1.3ds", *assets);
    //assets->start(g_hPShaderProgram, "resources/models/porsche82.3ds", *assets);
    //assets->start(g_hPShaderProgram, "resources/models/Convertible.lwo", *assets);
	assets->start(g_hPShaderProgram, "resources/models/Mustang.lwo", *assets);
	//assets->start(g_hPShaderProgram, "resources/models/test.3DS", *assets);
    //assets->start(g_hPShaderProgram, "resources/models/camaro_2006.3ds", *assets);

    if(!assets->getScene())
	{
		printf("scene could not be loaded\n");	
		return 1;
	}
	printf("scene loaded\n");
	assets->setCubeHandle(CreateStaticCubemap());
	sbTxHandle = assets->getCubeHandle();

	// Main loop
	//for (int x = 0;x<4;x++)
	for (;;)
	{
		touch = runTouch(Xrotation, Yrotation, Zrotation, zoom, width, height);
   		if (1==touch) break;
   		if (2==touch){
   			mySceneManager->startScene(0);
   		}
   		if (3==touch){
   			mySceneManager->startScene(1);
   		}
   		if (4==touch){
   			mySceneManager->startScene(2);
   		}
		//fpsStart = fslGetTickCount();
		Render(assets, Xrotation, Yrotation, Zrotation, zoom);
		//fpsEnd = fslGetTickCount();
/*		miliseconds = fpsEnd - fpsStart;
		if (miliseconds<17) //17 so we cap the framerate to ~60
		{
			usleep((17-miliseconds)*1000);
		}*/
		//printf("frame %i\n",frameCount);
		++ frameCount;
		eglSwapBuffers(eglDisplay, eglSurface);
	}

	unsigned int end = fslGetTickCount();
	float fps = frameCount / ((end - start) / 1000.0f);
	printf("%d frames in %d ticks -> %.3f fps\n", frameCount, end - start, fps);
	RenderCleanup(assets);
	
	// cleanup
    killTouch();
	DestroyShaders();
	EGLdeinit(eglDisplay);

	return 0;
}
