// assimp include files. These three are usually needed.
#include <ASSIMP/assimp.h>
#include <ASSIMP/aiPostProcess.h>
#include <ASSIMP/aiScene.h>

#include <IL/il.h>

#include <vector>
#include <map>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <stdio.h>

// Information to render each assimp node

struct MyMaterial{

	float diffuse[4];
	float ambient[4];
	float specular[4];
	float emissive[4];
	float shininess;
	int texCount;
};

struct MyMesh{
	
	GLuint VBO[4]; //to store faces, vertices, normals and texcoords
	GLuint texIndex;
	MyMaterial matInfo;
	int numFaces;
};

struct obj3d{

	std::vector<struct MyMesh> myMeshes;
	std::map<std::string, GLuint> textureIdMap;	
	const struct aiScene *scene;
	GLuint modelMatrixLoc;
	GLuint vertexLoc;
	GLuint normalLoc;
	GLuint texCoordLoc;

	GLuint diffuseLoc;
	GLuint ambientLoc;
	GLuint specularLoc;
	GLuint emissiveLoc;
	GLuint shininessLoc;
	GLuint texCountLoc;

	GLuint texUnitLoc;
	GLuint cubeHandle;
};

enum vboNames { VBO_faces,
				VBO_vertices,
				VBO_normals,
				VBO_texCoords
				};

void set_float4(float f[4], float a, float b, float c, float d);
void color4_to_float4(const struct aiColor4D *c, float f[4]);
void loadAsset(const char * path, obj3d &asset);
void shaderInit(GLuint &shaderProgram, obj3d &asset);
void recursive_render (aiMatrix4x4 currentTransform, const struct aiNode* nd, obj3d &asset);
int LoadGLTextures(const aiScene* scene, obj3d &asset);

