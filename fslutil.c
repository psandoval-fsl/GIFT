
/* File: fslutil.c
 *
 * Copyright 09 Freescale Semiconductor
 */

#define GL_FUNCS 1

//#ifdef GL_FUNCS
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
//#include <GLES/gl.h>
//#endif

#include <EGL/egl.h>
#include <fslutil.h>
#include <IL/il.h>

int ImageLoad(char *filename, Image *image) {
	FILE *file;
	unsigned long size;                 // size of the image in bytes.
	unsigned long i;                    // standard counter.
	unsigned short int planes;          // number of planes in image (must be 1) 
	unsigned short int bpp;             // number of bits per pixel (must be 24)
	char temp;                          // temporary color storage for bgr-rgb conversion.

    // make sure the file is there.
	if ((file = fopen(filename, "rb"))==NULL)
	{
		printf("File Not Found : %s\n",filename);
		return 0;
	}
    
    // seek through the bmp header, up to the width/height:
	fseek(file, 18, SEEK_CUR);

    // read the width
	if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
		printf("Error reading width from %s.\n", filename);
		return 0;
	}
	//printf("Width of %s: %lu\n", filename, image->sizeX);
    
    // read the height 
	if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
		printf("Error reading height from %s.\n", filename);
		return 0;
	}
	//printf("Height of %s: %lu\n", filename, image->sizeY);
    
    // calculate the size (assuming 24 bits or 3 bytes per pixel).
	size = image->sizeX * image->sizeY * 3;
	//printf("size is %lu\n",size);

    // read the planes
	if ((fread(&planes, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);
		return 0;
	}
	if (planes != 1) {
		printf("Planes from %s is not 1: %u\n", filename, planes);
		return 0;
	}

    // read the bpp
	if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);
		return 0;
	}
	if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);
		return 0;
	}
	
    // seek past the rest of the bitmap header.
	fseek(file, 24, SEEK_CUR);

    // read the data. 
	image->data = (char *) malloc(size);
	if (image->data == NULL) {
		printf("Error allocating memory for color-corrected image data");
		return 0;	
	}

	if ((i = fread(image->data, size, 1, file)) != 1) {
		printf("Error reading image data from %s.\n", filename);
		return 0;
	}

	for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
		temp = image->data[i];
		image->data[i] = image->data[i+2];
		image->data[i+2] = temp;
	}
	
    
    // we're done.
	return 1;
}
  

//--------------------------------------------------------------------------------------
// Name: fslLoadCTES
// Desc: Helper function to load an compressed image file (ATC, ETC, etc.) from the compressenator
// 		 At exit nFormat contains the GL 
//--------------------------------------------------------------------------------------
char* fslLoadCTES( char* strFileName, unsigned int* pWidth, unsigned int* pHeight, 
                  unsigned int* nFormat, unsigned int* nSize )
{
	unsigned int nTotalBlocks, nBytesPerBlock, nHasAlpha;
	char* pBits8;

	struct CTES_HEADER 
	{
		unsigned int	signature;
		unsigned int	width;
		unsigned int	height;
		unsigned int	flags;
		unsigned int	dataOffset;  // From start of header/file
	} header;

	// Read the file
	FILE* file = fopen( strFileName, "rb" );
	if( NULL == file )
	{
		printf("Error loading file: %s \n",strFileName);
		return NULL;
	}


	if (fread( &header, sizeof(header), 1, file ) != 1) 
	{
		printf("Error loading file : %s \n",strFileName);
		fclose( file );
		return NULL;
	}

	nTotalBlocks = ((header.width + 3) >> 2) * ((header.height + 3) >> 2);
	nHasAlpha = header.flags & ATC_RGBA;
	nBytesPerBlock = (nHasAlpha) ? 16 : 8;

	(*nSize)   = nTotalBlocks * nBytesPerBlock;
	(*pWidth)  = header.width;
	(*pHeight) = header.height;

	switch (header.signature)
	{
		case ATC_SIGNATURE:
			if(nHasAlpha && (header.flags & ATC_ALPHA_INTERPOLATED))
				(*nFormat) = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD; 
			else if(nHasAlpha)
				(*nFormat) = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD; 
			else
				(*nFormat) = GL_ATC_RGB_AMD; 
			break;
		case ATI1N_SIGNATURE:	
			(*nFormat) = GL_3DC_X_AMD; 
			break;
		case ATI2N_SIGNATURE:
			(*nFormat) = GL_3DC_XY_AMD; 
			break;
		case ETC_SIGNATURE: 
			if(nHasAlpha)
			{
				printf("Unsupported texture format\n");
				return NULL;
			}
			(*nFormat) = GL_ETC1_RGB8_OES; 
			break;

		default:
			printf("Unsupported texture format\n");
			return NULL;
			break;
	}
	pBits8 = (char*)malloc(sizeof(char) * (*nSize));

	// Read the encoded image 
	fseek(file, header.dataOffset, SEEK_SET);
	if (fread(pBits8, *nSize, 1, file) != 1) 	{
		printf("Error loading file : %s \n",strFileName);
		fclose( file );
		free( pBits8 );
		return NULL;
	}

	fclose( file );

	return pBits8;
}

fslBool fslUnProject(float winx,float winy, float winz, float modelMatrix[16],  float projMatrix[16],  int viewport[4], float *objx, float *objy, float *objz)
{
	float finalMatrix[16];
	float finalMatrixTmp[16];
	float in[4];
	float out[4];
	//printf("multiplying ModelMatrix \n");
	//fslPrintMatrix4x4(modelMatrix);
	//printf("by ProjMatrix \n");
	//fslPrintMatrix4x4(projMatrix);
	fslMultMatrix4x4(finalMatrixTmp,modelMatrix,projMatrix);
	//printf("result is \n");
	//fslPrintMatrix4x4(finalMatrix);
	if(!fslInvertMatrix4x4( finalMatrixTmp, finalMatrix))
	{
		printf("error inverting matrix \n");
		return FSL_FALSE;
	}
		
	in[0]=winx;
	in[1]=winy;
	in[2]=winz;
	in[3]=1.0;
	//map x and y from coordinates
	in[0]=(in[0]-viewport[0])/ viewport[2];
	in[1]=(in[1]-viewport[1])/ viewport[3];
	
	//map range to -1 to 1
	in[0]= in[0]*2-1;
	in[1]= in[1]*2-1;
	in[2]= in[2]*2-1;
	//printf("in: %f %f %f \n",in[0],in[1],in[2]);
	fslMultMatrix4x4Vec4x1 ( finalMatrix, in, out );
	
	
	if(out[3]== 0.0)
		return FSL_FALSE;
	
	
	out[0] /=out[3];
	out[1] /=out[3];
	out[2] /=out[3];
	
	
	*objx = out[0];
	*objy = out[1];
	*objz = out[2];
	
	return FSL_TRUE;
	
	
}


//--------------------------------------------------------------------------------------
// Name: fslEGLCheck
// Desc: Helper function to print EGL errors and exits application
//--------------------------------------------------------------------------------------
fslBool fslEGLCheck( fslBool bExitOnFailure)
{
        EGLint eglerr = eglGetError();

        if(eglerr != EGL_SUCCESS)
        {
			switch(eglerr){
				case EGL_NOT_INITIALIZED:
				fprintf(stdout, "EGL Fail = EGL_NOT_INITIALIZED (0x%x) \n", eglerr);
				break;
				case EGL_BAD_ACCESS:
				fprintf(stdout, "EGL Fail = EGL_BAD_ACCESS (0x%x) \n", eglerr);
				break;
				case EGL_BAD_ALLOC:
				fprintf(stdout, "EGL Fail = EGL_BAD_ALLOC (0x%x) \n", eglerr);
				break;
				case EGL_BAD_ATTRIBUTE:
				fprintf(stdout, "EGL Fail = EGL_BAD_ATTRIBUTE(0x%x) \n", eglerr);
				break;
				case EGL_BAD_CONFIG:
				fprintf(stdout, "EGL Fail = EGL_BAD_CONFIG (0x%x) \n", eglerr);
				break;
				case EGL_BAD_CONTEXT:
				fprintf(stdout, "EGL Fail = EGL_BAD_CONTEXT (0x%x) \n", eglerr);
				break;
				case EGL_BAD_CURRENT_SURFACE:
				fprintf(stdout, "EGL Fail = EGL_BAD_CURRENT_SURFACE (0x%x) \n", eglerr);
				break;
				case EGL_BAD_DISPLAY:
				fprintf(stdout, "EGL Fail = EGL_BAD_DISPLAY (0x%x) \n", eglerr);
				break;
				case EGL_BAD_MATCH:
				fprintf(stdout, "EGL Fail = EGL_BAD_MATCH (0x%x) \n", eglerr);
				break;
				case EGL_BAD_NATIVE_PIXMAP:
				fprintf(stdout, "EGL Fail = EGL_BAD_NATIVE_PIXMAP (0x%x) \n", eglerr);
				break;
				case EGL_BAD_NATIVE_WINDOW:
				fprintf(stdout, "EGL Fail = EGL_BAD_NATIVE_WINDOW (0x%x) \n", eglerr);
				break;
				case EGL_BAD_PARAMETER:
				fprintf(stdout, "EGL Fail = EGL_BAD_PARAMETER (0x%x) \n", eglerr);
				break;
				case EGL_BAD_SURFACE:
				fprintf(stdout, "EGL Fail = EGL_BAD_SURFACE (0x%x) \n", eglerr);
				break;
				case EGL_CONTEXT_LOST:
				fprintf(stdout, "EGL Fail = EGL_CONTEXT_LOST (0x%x) \n", eglerr);
				break;
				default:
						fprintf(stdout, "EGL Fail = 0x%x \n", eglerr);
			}

			if (bExitOnFailure)
				exit(EXIT_FAILURE);
			else
				return FSL_FALSE;
        }

	return FSL_TRUE;
}

#ifdef GL_FUNCS
fslBool fslInit2DBMPTextureGL(char* strFileName, GLuint *pTextureHandle){
	
	Image *image1;
    
    // allocate space for texture we will use
	image1 = (Image *) malloc(sizeof(Image));
	if (image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}

	if (!ImageLoad(strFileName, image1)) {
		exit(1);
	}        
	
	glGenTextures( 1, pTextureHandle );
	glBindTexture( GL_TEXTURE_2D, *pTextureHandle );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,image1->sizeX,image1->sizeY,0,GL_RGB,GL_UNSIGNED_BYTE,image1->data);
	free(image1);
	return FSL_TRUE;
	
}


//--------------------------------------------------------------------------------------
// Name: fslInit2DCTESTextureGL
// Desc: Helper function to load a CTES texture file and bind it to a given GL texture handle
//--------------------------------------------------------------------------------------
fslBool fslInit2DCTESTextureGL( char* strFileName, GLuint *pTextureHandle )
{
	unsigned int nWidth, nHeight, nFormat, nSize;
	char* pImageData = fslLoadCTES( strFileName, &nWidth, &nHeight, &nFormat, &nSize );
	
	if( NULL == pImageData )
	{
		printf("Error loading texture! \n");
		return FSL_FALSE;
	}
	else
	{
		printf("Texture [%s] succesully read (%d x %d , %d) \n",strFileName,nWidth,nHeight,nSize);
	}

	glGenTextures( 1, pTextureHandle );
	glBindTexture( GL_TEXTURE_2D, *pTextureHandle );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glCompressedTexImage2D( GL_TEXTURE_2D, 0, nFormat, nWidth, nHeight, 0, nSize, pImageData );
	free(pImageData);

	return FSL_TRUE;
}
#endif

#ifdef  FSL_EGL_USE_X11
//--------------------------------------------------------------------------------------
// Name: fslLoadFontX
// Desc: Helper function to load a X11 font into given struct
//--------------------------------------------------------------------------------------
void fslLoadFontX(Display *display, XFontStruct **font_info)
{
	char *fontname = "9x15";

	if ((*font_info=XLoadQueryFont(display,fontname)) == NULL)
	{
		printf("stderr, basicwin: cannot open 9x15 font\n");
		exit(EXIT_FAILURE);
	}
}

//--------------------------------------------------------------------------------------
// Name: fslErrorHandlerX
// Desc: Helper function to print incoming X11 server errors
//--------------------------------------------------------------------------------------
int fslErrorHandlerX( Display *display, XErrorEvent *error )
{
	char errorText[1024];
	XGetErrorText( display, error->error_code, errorText, sizeof(errorText) );
	printf( "\t --- X Error: %s ---\n", errorText );
	return 0;
}
#endif

//--------------------------------------------------------------------------------------
// Name: fslGetTickCount
// Desc: Helper function to get current time
//--------------------------------------------------------------------------------------
unsigned int fslGetTickCount()
{
	struct timeval tv;
	if(gettimeofday(&tv, NULL) != 0)
	{
		return 0;
	}

	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

//--------------------------------------------------------------------------------------
// Name: fslMulMatrix4x4
// Desc: 4x4 Matix Muliply
//--------------------------------------------------------------------------------------
void fslMultMatrix4x4( float *matC, float *matA, float *matB)
{
	matC[ 0] = matA[ 0] * matB[ 0] + matA[ 1] * matB[ 4] + matA[ 2] * matB[ 8] + matA[ 3] * matB[12];
	matC[ 1] = matA[ 0] * matB[ 1] + matA[ 1] * matB[ 5] + matA[ 2] * matB[ 9] + matA[ 3] * matB[13];
	matC[ 2] = matA[ 0] * matB[ 2] + matA[ 1] * matB[ 6] + matA[ 2] * matB[10] + matA[ 3] * matB[14];
	matC[ 3] = matA[ 0] * matB[ 3] + matA[ 1] * matB[ 7] + matA[ 2] * matB[11] + matA[ 3] * matB[15];
	matC[ 4] = matA[ 4] * matB[ 0] + matA[ 5] * matB[ 4] + matA[ 6] * matB[ 8] + matA[ 7] * matB[12];
	matC[ 5] = matA[ 4] * matB[ 1] + matA[ 5] * matB[ 5] + matA[ 6] * matB[ 9] + matA[ 7] * matB[13];
	matC[ 6] = matA[ 4] * matB[ 2] + matA[ 5] * matB[ 6] + matA[ 6] * matB[10] + matA[ 7] * matB[14];
	matC[ 7] = matA[ 4] * matB[ 3] + matA[ 5] * matB[ 7] + matA[ 6] * matB[11] + matA[ 7] * matB[15];
	matC[ 8] = matA[ 8] * matB[ 0] + matA[ 9] * matB[ 4] + matA[10] * matB[ 8] + matA[11] * matB[12];
	matC[ 9] = matA[ 8] * matB[ 1] + matA[ 9] * matB[ 5] + matA[10] * matB[ 9] + matA[11] * matB[13];
	matC[10] = matA[ 8] * matB[ 2] + matA[ 9] * matB[ 6] + matA[10] * matB[10] + matA[11] * matB[14];
	matC[11] = matA[ 8] * matB[ 3] + matA[ 9] * matB[ 7] + matA[10] * matB[11] + matA[11] * matB[15];
	matC[12] = matA[12] * matB[ 0] + matA[13] * matB[ 4] + matA[14] * matB[ 8] + matA[15] * matB[12];
	matC[13] = matA[12] * matB[ 1] + matA[13] * matB[ 5] + matA[14] * matB[ 9] + matA[15] * matB[13];
	matC[14] = matA[12] * matB[ 2] + matA[13] * matB[ 6] + matA[14] * matB[10] + matA[15] * matB[14];
	matC[15] = matA[12] * matB[ 3] + matA[13] * matB[ 7] + matA[14] * matB[11] + matA[15] * matB[15];
}

fslBool fslInvertMatrix4x4( float *src, float *inverse)
{
    int i, j, k, swap;
    double t;
    double temp[4][4];

    for (i=0; i<4; i++) {
	for (j=0; j<4; j++) {
	    temp[i][j] = src[i*4+j];
	}
    }

		inverse[ 0] = 1.0f;
		inverse[ 1] = 0.0f;
		inverse[ 2] = 0.0f;
		inverse[ 3] = 0.0f;
		inverse[ 4] = 0.0f;
		inverse[ 5] = 1.0f;
		inverse[ 6] = 0.0f;
		inverse[ 7] = 0.0f;
		inverse[ 8] = 0.0f;
		inverse[ 9] = 0.0f;
		inverse[10] = 1.0f;
		inverse[11] = 0.0f;
		inverse[12] = 0.0f;
		inverse[13] = 0.0f;
		inverse[14] = 0.0f;
		inverse[15] = 1.0f;


    for (i = 0; i < 4; i++) {
	/*
	** Look for largest element in column
	*/
	swap = i;
	for (j = i + 1; j < 4; j++) {
	    if (fabs(temp[j][i]) > fabs(temp[i][i])) {
		swap = j;
	    }
	}

	if (swap != i) {
	    /*
	    ** Swap rows.
	    */
	    for (k = 0; k < 4; k++) {
		t = temp[i][k];
		temp[i][k] = temp[swap][k];
		temp[swap][k] = t;

		t = inverse[i*4+k];
		inverse[i*4+k] = inverse[swap*4+k];
		inverse[swap*4+k] = t;
	    }
	}

	if (temp[i][i] == 0) {
	    /*
	    ** No non-zero pivot.  The matrix is singular, which shouldn't
	    ** happen.  This means the user gave us a bad matrix.
	    */
	    return FSL_FALSE;
	}

	t = temp[i][i];
	for (k = 0; k < 4; k++) {
	    temp[i][k] /= t;
	    inverse[i*4+k] /= t;
	}
	for (j = 0; j < 4; j++) {
	    if (j != i) {
		t = temp[j][i];
		for (k = 0; k < 4; k++) {
		    temp[j][k] -= temp[i][k]*t;
		    inverse[j*4+k] -= inverse[i*4+k]*t;
		}
	    }
	}
    }
    return FSL_TRUE;
}

void fslPerspectiveMatrix4x4 ( float *m, float fov, float aspect, float zNear, float zFar)
{
	const float h = 1.0f/tan(fov*PI_OVER_360);
	float neg_depth = zNear-zFar;
	
	m[0] = h / aspect;
	m[1] = 0;
	m[2] = 0;
	m[3] = 0;
	
	m[4] = 0;
	m[5] = h;
	m[6] = 0;
	m[7] = 0;
	
	m[8] = 0;
	m[9] = 0;
	m[10] = (zFar + zNear)/neg_depth;
	m[11] = -1;
	
	m[12] = 0;
	m[13] = 0;
	m[14] = 2.0f*(zNear*zFar)/neg_depth;
	m[15] = 0;

}

void fslMultMatrix4x4Vec4x1 ( float *matA, float *vecA, float *vecB )
{
    int i;

    for ( i = 0; i < 4; i++ ) 
	{
		vecB[i] = vecA[0] * matA[0*4+i] + vecA[1] * matA[1*4+i] + vecA[2] * matA[2*4+i] + vecA[3] * matA[3*4+i];
    }
}

void fslRotateMatrix4x4 (float *m, float angle, fslAxis axis)
{
	float radians = PI_OVER_360*2*angle;
   
 	float rotate[16] = {0};
	float store[16] = {0};

 	fslLoadIdentityMatrix4x4(rotate);

	switch (axis)
	{
		case FSL_X_AXIS:
			rotate[5] = cosf(radians);
			rotate[6] = -sinf(radians);
			rotate[9] = sinf(radians);
			rotate[10] = cosf(radians);
			fslMultMatrix4x4(store, rotate, m);
         memcpy( m, store, 16*sizeof(float) );
		break;
		case FSL_Y_AXIS:
			rotate[0] = cos(radians);
			rotate[2] = -sin(radians);
			rotate[8] = sin(radians);
			rotate[10] = cos(radians);
			fslMultMatrix4x4(store, rotate, m);
         memcpy( m, store, 16*sizeof(float) );
		break;		
		case FSL_Z_AXIS:
			rotate[0] = cos(radians);
			rotate[1] = sin(radians);
			rotate[4] = -sin(radians);
			rotate[5] = cos(radians);
			fslMultMatrix4x4(store, rotate, m);
         memcpy( m, store, 16*sizeof(float) );
		break;		
		default:
		printf("invalid axis \n");
		break;

	}


}

void fslTranslateMatrix4x4 (float *m, float transX, float transY, float transZ)
{
 	float trans[16] = {0};
 	fslLoadIdentityMatrix4x4(trans);

 	trans[12]=transX;
 	trans[13]=transY;
 	trans[14]=transZ;

 	fslMultMatrix4x4(m, trans, m);
}

void fslScaleMatrix4x4 (float *m, float scaleX, float scaleY, float scaleZ)
{
 	float scale[16] = {0};
 	fslLoadIdentityMatrix4x4(scale);

	scale[0]=scaleX;
	scale[5]=scaleY;
	scale[10]=scaleZ;

 	fslMultMatrix4x4(m, scale, m);
}

void fslLoadIdentityMatrix4x4 (float *m)
{
	m[0] = 1;
	m[1] = 0;
	m[2] = 0;
	m[3] = 0;
	
	m[4] = 0;
	m[5] = 1;
	m[6] = 0;
	m[7] = 0;
	
	m[8] = 0;
	m[9] = 0;
	m[10] = 1;
	m[11] = 0;
	
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1;
}

float fslInnerProduct( float *v, float *q)
{
	return v[0]*q[0]+v[1]*q[1]+v[2]*q[2];
}
//result = endpoint-startPoint
void fslDirectionVector(float *result, float *endPoint, float *startPoint)
{
	result[0]= endPoint[0]-startPoint[0];
	result[1]= endPoint[1]-startPoint[1];
	result[2]= endPoint[2]-startPoint[2];
}
//a = crossProduct(b,c)
void fslCrossProduct(float *result, float *b, float* c)
{
	result[0]= b[1]*c[2]-c[1]*b[2];
	result[1]= b[2]*c[0]-c[2]*b[0];
	result[2]= b[0]*c[1]-c[0]*b[1];
}
//p is the startpoint, d is the direction vector, v0,v1,v2 represent the triangle
int fslRayIntersectsTriangle(float *p, float *d,float *v0, float *v1, float *v2)
{
	float e1[3]={0.0, 0.0, 0.0};
	float e2[3]={0.0, 0.0, 0.0};
	float h[3] = {0.0, 0.0, 0.0};
	float s[3] = {0.0, 0.0, 0.0};
	float q[3] = {0.0, 0.0, 0.0};
	float a,f,u,v,t;
	fslDirectionVector(e1,v1,v0);
	fslDirectionVector(e2,v2,v0);
	fslCrossProduct(h,d,e2);
	//get the inner product
	a=fslInnerProduct(e1,h);
	if(a>-0.00001&& a<0.00001)
		return 0;
	f= 1/a;
	fslDirectionVector(s,p,v0);
	u=f*(fslInnerProduct(s,h));
	
	if(u<0.0 || u>1.0)
		return 0;
	
	fslCrossProduct(q,s,e1);
	v = f*fslInnerProduct(d,q);
	if(v< 0.0 || u+v > 1.0)
		return 0;
	//we can compute t to find out where the intersection point is on the line
	 t= f*fslInnerProduct(e2,q);
	if(t > 0.00001) //ray intersection
		return 1;
	else
		return 0;

	
}
void fslNormalize(float *v)
{
	float mag =v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
	sqrt(mag);
	v[0]=v[0]/mag;
	v[1]=v[1]/mag;
	v[2]=v[2]/mag;
	//printf("vector Normalized is %f,%f,%f\n",v[0],v[1],v[2]);	
}
void fslPrintMatrix4x4(float *m){
	
	printf(" %f %f %f %f \n", m[0], m[1], m[2], m[3]);
	printf(" %f %f %f %f \n", m[4], m[5], m[6], m[7]);
	printf(" %f %f %f %f \n", m[8], m[9], m[10], m[11]);
	printf(" %f %f %f %f \n", m[12], m[13], m[14], m[15]);
	
}
//not sure if working
void fslCalculateNormals(float *triangleArray, int size, float *normalArray)
{
	printf("enter fslCalculateNormals \n");

	
      int index =0;

      int i=0;
     //for(i; i< size/3; i++){

	 float v1[3]={0.0f};
	 float v2[3]={0.0f};
	 float v3[3]={0.0f};
	 //float v4[3]={0.0f};
    
         float A[3]={0.0f};
         float B[3]={0.0f};
         //float C[3]={0.0f};
	      //float D[3]={0.0f};

         v1[0]=triangleArray[9*i];//9 is for 9 values (3 vertices) used per loop
         v1[1]=triangleArray[9*i+1];
         v1[2]=triangleArray[12*i+2];
  
         v2[0]=triangleArray[9*i+3];
         v2[1]=triangleArray[9*i+4];
         v2[2]=triangleArray[9*i+5];

         v3[0]=triangleArray[9*i+6];
         v3[1]=triangleArray[9*i+7];
         v3[2]=triangleArray[9*i+8];


         A[0]=v1[0]-v2[0];
         A[1]=v1[1]-v2[1];
         A[2]=v1[2]-v2[2]; 

         B[0]=v2[0]-v3[0];
         B[1]=v2[1]-v3[1];
         B[2]=v2[2]-v3[2]; 
       float result[3]={0.0f};

         printf("adding normal  %i\n",index);
         fslCrossProduct(result, A, B);
         fslNormalize(result);
         normalArray[3*i]=result[0];
         normalArray[3*i+1]=result[1];
         normalArray[3*i+2]=result[2];
   	
          printf("adding normal:  %f %f %f \n",result[0],result[1],result[2]);
         index++;



    //}
  
     /*
    int i =0;
      for(i; i< size/4; i++){

         float v1[3]={0.0f};
	 float v2[3]={0.0f};
	 float v3[3]={0.0f};
	 float v4[3]={0.0f};
    
         float A[3]={0.0f};
         float B[3]={0.0f};
         float C[3]={0.0f};
	 float D[3]={0.0f};

         v1[0]=triangleArray[12*i];//12 is for 12 values (4 vertices) used per loop
         v1[1]=triangleArray[12*i+1];
         v1[2]=triangleArray[12*i+2];
  
         v2[0]=triangleArray[12*i+3];
         v2[1]=triangleArray[12*i+4];
         v2[2]=triangleArray[12*i+5];

         v3[0]=triangleArray[12*i+6];
         v3[1]=triangleArray[12*i+7];
         v3[2]=triangleArray[12*i+8];

         v4[0]=triangleArray[12*i+9];
         v4[1]=triangleArray[12*i+10];
         v4[2]=triangleArray[12*i+11];

         A[0]=v1[0]-v2[0];
         A[1]=v1[1]-v2[1];
         A[2]=v1[2]-v2[2]; 

         B[0]=v2[0]-v3[0];
         B[1]=v2[1]-v3[1];
         B[2]=v2[2]-v3[2]; 
         
         C[0]=v3[0]-v2[0];
         C[1]=v3[1]-v2[1];
         C[2]=v3[2]-v2[2]; 
       
         D[0]=v2[0]-v4[0];
         D[1]=v2[1]-v4[1];
         D[2]=v2[2]-v4[2]; 

      printf("calculating normals for face %i \n",i);
     //n1= (v1-v2)x(v2-v3)
         float result[3]={0.0f};

         

         printf("adding normal  %i\n",index);
         fslCrossProduct(result, A, B);
         fslNormalize(result);
         normalArray[6*i]=result[0];
         normalArray[6*i+1]=result[1];
         normalArray[6*i+2]=result[2];
   	
          printf("adding normal:  %f %f %f \n",result[0],result[1],result[2]);
         index++;
        
        fslCrossProduct(result, C, D);
	fslNormalize(result);
         normalArray[6*i+3]=result[0];
         normalArray[6*i+4]=result[1];
         normalArray[6*i+5]=result[2];
         printf("adding normal:  %f %f %f \n",result[0],result[1],result[2]);
         index++;
	
		
    }

*/


}

/***************************************************************************************
* LoadShaders and CompileShader should be added to fslutils
***************************************************************************************/
//compiles either a fragment or vertex shader
int CompileShader(const char * FName, GLuint ShaderNum)
{
	FILE * fptr = NULL;
	fptr = fopen(FName, "rb");
	if (fptr == NULL)
	{
		return 0;
	}

	int length;
	fseek(fptr, 0, SEEK_END);
	length = ftell(fptr);
	fseek(fptr, 0 ,SEEK_SET);

	char * shaderSource = (char*)malloc(sizeof (char) * length);
	if (shaderSource == NULL)
	{
		fprintf(stderr, "Out of memory.\n");
		return 0;
	}

	size_t successful_read = fread(shaderSource, length, 1, fptr);
	if (!successful_read)
	{
		printf("failed to read shader source file\n");
	}
	glShaderSource(ShaderNum, 1, (const char**)&shaderSource, &length);
	glCompileShader(ShaderNum);

	free(shaderSource);
	fclose(fptr);

	GLint compiled = 0;
	glGetShaderiv(ShaderNum, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		// Retrieve error buffer size.
		GLint errorBufSize, errorLength;
		glGetShaderiv(ShaderNum, GL_INFO_LOG_LENGTH, &errorBufSize);

		char * infoLog = (char*)malloc(errorBufSize * sizeof(char) + 1);
		if (!infoLog)
		{
			// Retrieve error.
			glGetShaderInfoLog(ShaderNum, errorBufSize, &errorLength, infoLog);
			infoLog[errorBufSize + 1] = '\0';
			//fprintf(stderr, "%s\n", infoLog);
			printf("Compile error: %s\n",infoLog);
			free(infoLog);
		}
		return 0;
	}

	return 1;
}

// links vertex and fragment shader then links a shader program
void LoadShaders(const char * vShaderFName, const char * pShaderFName, GLuint & g_hShaderProgram)
{
	GLuint vertShaderNum = glCreateShader(GL_VERTEX_SHADER);
	GLuint pixelShaderNum = glCreateShader(GL_FRAGMENT_SHADER);

	if (CompileShader(vShaderFName, vertShaderNum) == 0)
	{
		printf("vshader compile failed\n");
		return;
	}

	if (CompileShader(pShaderFName, pixelShaderNum) == 0)
	{
		printf("fshader compile failed\n");
		return;
	}

	g_hShaderProgram = glCreateProgram();

	glAttachShader(g_hShaderProgram , vertShaderNum);
	glAttachShader(g_hShaderProgram , pixelShaderNum);

	glLinkProgram(g_hShaderProgram );
	// Check if linking succeeded.
	GLint linked = false;
	glGetProgramiv(g_hShaderProgram , GL_LINK_STATUS, &linked);
	if (!linked)
	{
		// Retrieve error buffer size.
		GLint errorBufSize, errorLength;
		glGetShaderiv(g_hShaderProgram , GL_INFO_LOG_LENGTH, &errorBufSize);

		char * infoLog = (char*)malloc(errorBufSize * sizeof (char) + 1);
		if (!infoLog)
		{
			// Retrieve error.
			glGetProgramInfoLog(g_hShaderProgram , errorBufSize, &errorLength, infoLog);
			infoLog[errorBufSize + 1] = '\0';
			fprintf(stderr, "%s", infoLog);

			free(infoLog);
		}
		printf("shader program link failed\n");
		return;
	}
	glDeleteShader(vertShaderNum);
	glDeleteShader(pixelShaderNum);
}

//vanilla egl initialization stuff
int EGLinit(EGLDisplay &eglDisplay, EGLSurface &eglSurface)
{
   EGLContext		eglContext;
   NativeDisplayType 	display;
   NativeWindowType 	window;

	printf("entering init\n");
	
	EGLint configAttribs[] =
	{
		EGL_RED_SIZE,       8,
		EGL_GREEN_SIZE,     8,
		EGL_BLUE_SIZE,      8,
		EGL_ALPHA_SIZE,     EGL_DONT_CARE,
		EGL_DEPTH_SIZE,     24,
		EGL_STENCIL_SIZE,   EGL_DONT_CARE,
		EGL_SAMPLE_BUFFERS, EGL_DONT_CARE,
		EGL_SAMPLES,        EGL_DONT_CARE, //2, 4, to enable FSAA, EGL_DONT_CARE to  disable
		EGL_SURFACE_TYPE,   EGL_WINDOW_BIT,
		EGL_MIN_SWAP_INTERVAL, 1,
		EGL_NONE
	};

	EGLint ctxAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLConfig configs[10];
	EGLint matchingConfigs;

	display = fbGetDisplay();
	
	int w, h, t, l;
	l=t=0;
	fbGetDisplayGeometry(display, &w, &h);
	window = fbCreateWindow(display, t, l, 1280 , 1024);
	
	eglDisplay = eglGetDisplay(display);
	if (!eglInitialize(eglDisplay, NULL, NULL))
	{
		return 1;
	}

	if (!eglChooseConfig(eglDisplay, configAttribs, &configs[0], 10, &matchingConfigs))
	{
		return 1;
	}

	if (matchingConfigs < 1)
	{
		return 1;
	}

	eglSurface = eglCreateWindowSurface(eglDisplay, configs[0], window, configAttribs);
	if (eglSurface == EGL_NO_SURFACE)
	{
		return 1;
	}

	eglContext = eglCreateContext(eglDisplay, configs[0], NULL, ctxAttribs);
	if (eglContext == EGL_NO_CONTEXT)
	{
		return 1;
	}

	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	
	//eglSwapInterval(eglDisplay, 1); not working, bug filled with altia. This should handle vsync
	
	return 0;
}

//vanilla egl deinitialization stuff
void EGLdeinit(EGLDisplay &eglDisplay)
{
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglTerminate(eglDisplay);
	eglReleaseThread();
}

GLuint CreateStaticCubemap()
{
	ILboolean success;
	char texName[] = "cm/1.jpg";

	/* create and fill array with DevIL texture ids */
	ILuint* imageIds = new ILuint[6];
	ilGenImages(6, imageIds); 

	/* create and fill array with GL texture ids */
	GLuint textureObject;
    glGenTextures(1, &textureObject);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureObject);
	/* get iterator */

	for (int i =0; i < 6 ; ++i)
	{
		texName[3] = i+49;
		ilBindImage(imageIds[i]); /* Binding of DevIL image name */
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_LOWER_LEFT); 
		success = ilLoadImage((ILstring)texName);
		
		if (success) {
			/* Convert image to RGBA */
			ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE); 

			/* Create and load textures to OpenGL */
			GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
			glTexImage2D(face, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		}
		else 
			printf("Couldn't load cubemap\n");
	}
	/* Because we have already copied image data into texture data
	we can release memory used by image. */
	ilDeleteImages(6, imageIds); 

	//Cleanup
	delete [] imageIds;
	return textureObject;
}

GLuint CreateDynamicCubemap()
{
	ILboolean success;
	char texName[] = "cm/1.jpg";

	/* create and fill array with DevIL texture ids */
	ILuint* imageIds = new ILuint[6];
	ilGenImages(6, imageIds); 

	/* create and fill array with GL texture ids */
	GLuint textureObject;
    glGenTextures(1, &textureObject);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureObject);
	/* get iterator */

	for (int i =0; i < 6 ; ++i)
	{
		texName[3] = i+49;
		ilBindImage(imageIds[i]); /* Binding of DevIL image name */
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_LOWER_LEFT); 
		success = ilLoadImage((ILstring)texName);
		
		if (success) {
			/* Convert image to RGBA */
			ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE); 

			/* Create and load textures to OpenGL */
			GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
			glTexImage2D(face, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		}
		else 
			printf("Couldn't load cubemap\n");
	}
	/* Because we have already copied image data into texture data
	we can release memory used by image. */
	ilDeleteImages(6, imageIds); 

	//Cleanup
	delete [] imageIds;
	return textureObject;
}

void initSkybox(GLuint *sbVBO, GLuint &sbPosLoc)
{
	float cubeArray[] = {1, 1, 1,
						  -1, 1, 1,
						  -1, -1, 1,
						  1, -1, 1,
						  1, -1, -1,
						  1, 1, -1,
						  -1, 1, -1,
						  -1, -1, -1};

	GLubyte indexArray[] = {0, 1, 2, 0, 2, 3,
							 0, 3, 4, 0, 4, 5,
							 0, 5, 6, 0, 6, 1,
							 7, 6, 1, 7, 1, 2,
							 7, 4, 5, 7, 5, 6,
							 7, 2, 3, 7, 3, 4};

	glGenBuffers(2, sbVBO);
	glBindBuffer(GL_ARRAY_BUFFER, sbVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeArray), cubeArray, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sbVBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexArray), indexArray, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, sbVBO[0]);
	glVertexAttribPointer(sbPosLoc, 3, GL_FLOAT, 0, 0, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sbVBO[1]);
}

void renderSkybox(GLuint cubehandle, GLuint sbShaderProgram, GLuint sbVMLoc, GLuint sbPMLoc, 
				  float *matModelView, float *matProj, GLuint sbPosLoc, GLuint *sbVBO)
{
	glDisable(GL_DEPTH_TEST);   // skybox should be drawn behind anything else
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubehandle);
	glUseProgram(sbShaderProgram);   
	glUniformMatrix4fv( sbVMLoc, 1, 0, matModelView );
    glUniformMatrix4fv( sbPMLoc, 1, 0, matProj );
	
	glBindBuffer(GL_ARRAY_BUFFER, sbVBO[0]);
	glVertexAttribPointer(sbPosLoc, 3, GL_FLOAT, 0, 0, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sbVBO[1]);

	glEnableVertexAttribArray(sbPosLoc);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0);
	glDisableVertexAttribArray(sbPosLoc);
	glEnable(GL_DEPTH_TEST);
}
