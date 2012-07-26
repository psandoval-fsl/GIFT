/*
 * OpenGL ES 2.0 Asset Import loader
 *
 * Draws models with phong shading
 * b20593@freescale.com
 */

/* TODO
	->optimize
	->get more and fancier shaders
	->animation
	->load light(s) position(s)
*/

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <sys/time.h>

#include "obj3d.h"

#include "fslutil.h"
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

int preRender()
{

	// load and compiler vertex/fragment shaders.
	LoadShaders("shaders/vs_phong.vert", "shaders/fs_phong.frag", g_hPShaderProgram);
	LoadShaders("shaders/vs_skybox.vert", "shaders/fs_skybox.frag", g_hSBShaderProgram);
	LoadShaders("shaders/vs_texture.vert", "shaders/fs_texture.frag", g_hTXShaderProgram );

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
	return 0;

}

// sets matrices, renders objects
void Render(obj3d *assets, float Xrot, float Yrot, float Zrot, float zoomtr)
{
	// Clear background.
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

	float matModelView[16] = {0};
	fslLoadIdentityMatrix4x4 (matModelView);
	
	// Build a perspective projection matrix. You should consider using ortho
	//instead of perspective for clusters.
	float matProj[16] = {0};
	fslPerspectiveMatrix4x4 ( matProj, 70.f, 1.3333f, 0.1f, 200.f);
	fslRotateMatrix4x4(matModelView, 180, FSL_Z_AXIS);
	
	renderSkybox(sbTxHandle, g_hSBShaderProgram, sbVMLoc, sbPMLoc,
			matModelView, matProj, sbPosLoc, sbVBO);

	fslLoadIdentityMatrix4x4 (matModelView);
	fslTranslateMatrix4x4 (matModelView, 0, -2, -10);

	//fslRotateMatrix4x4 (matModelView, -Zrot, FSL_Z_AXIS);
	//fslRotateMatrix4x4 (matModelView, Xrot, FSL_X_AXIS);
	fslRotateMatrix4x4 (matModelView, -Yrot, FSL_Y_AXIS);
	fslRotateMatrix4x4 (matModelView, 90, FSL_X_AXIS);
	
	glUseProgram(g_hPShaderProgram);
	glUniformMatrix4fv( viewMatrixLoc, 1, 0, matModelView );
	glUniformMatrix4fv( projMatrixLoc, 1, 0, matProj );

	//Renders the car
    	aiMatrix4x4 id(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
	recursive_render(id, assets[0].scene->mRootNode, assets[0]);
   
	// swap buffers
	glFlush();
	
}

void RenderCleanup(obj3d *assets)
{
	// Clear background.
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   	eglSwapBuffers(eglDisplay, eglSurface);
	
	// free assImp scene resources
	aiReleaseImport(assets[0].scene);

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
	unsigned int fpsStart = 0;
	unsigned int fpsEnd = 0;
	unsigned int miliseconds = 0;
	float Xrotation, Yrotation, Zrotation, zoom = 0;
	obj3d assets[1];

	EGLinit(eglDisplay, eglSurface);

	if (1==preRender())
	{
		EGLdeinit(eglDisplay);
		return 1;
	}

	shaderInit(g_hPShaderProgram, assets[0]);

	//loadAsset("models/jeep1.3ds", assets[0]);
    	loadAsset("models/porsche82.3ds", assets[0]);		

	if (!assets[0].scene)
	{
		printf("scene could not be loaded\n");	
		return 1;
	}
	printf("scene loaded\n");
	assets[0].cubeHandle = CreateStaticCubemap();
	sbTxHandle = assets[0].cubeHandle;

	// Main loop
	//for (int x = 0;x<1;x++) 
	for (;;)
	{
   		if (1==runTouch(Xrotation, Yrotation, Zrotation, zoom, width, height))
		{
			break;
		}
		fpsStart = fslGetTickCount();
		Render(assets, Xrotation, Yrotation, Zrotation, zoom);
		fpsEnd = fslGetTickCount();
		miliseconds = fpsEnd - fpsStart;
		if (miliseconds<17) //17 so we cap the framerate to ~60
		{
			usleep((17-miliseconds)*1000);
		}
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
