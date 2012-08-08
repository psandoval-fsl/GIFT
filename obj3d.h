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

enum vboNames { VBO_faces,
				VBO_vertices,
				VBO_normals,
				VBO_texCoords
				};
class Obj3d{

	std::vector<struct MyMesh> myMeshes;
	std::map<std::string, GLuint> textureIdMap;	
	const struct aiScene *scene;
	GLuint modelMatrixLoc;
	GLuint vertexLoc;
	GLuint normalLoc;
	GLuint texCoordLoc;

	bool hasTextures;

	GLuint diffuseLoc;
	GLuint ambientLoc;
	GLuint specularLoc;
	GLuint emissiveLoc;
	GLuint shininessLoc;
	GLuint texCountLoc;

	GLuint texUnitLoc;
	GLuint cubeHandle;

	GLuint shaderProg;

	void loadAsset(const char * path, Obj3d &asset);

	public:

	Obj3d(bool HasTx);
	void start(GLuint shaderPrg, const char * path, Obj3d &obj);

	const struct aiScene * getScene(void){return scene;};
	GLuint getCubeHandle(void){return cubeHandle;};
	GLuint getShaderProgram(void){return shaderProg;};
	void setCubeHandle(GLuint handle){cubeHandle = handle;};
	void set_float4(float f[4], float a, float b, float c, float d);

	void color4_to_float4(const struct aiColor4D *c, float f[4]);

	void recursive_render (aiMatrix4x4 currentTransform, const struct aiNode* nd, Obj3d &asset);
	int LoadGLTextures(const aiScene* scene, Obj3d &asset);
	void draw(float * matMV, float * matP, GLuint matMVLoc, GLuint matPLoc);

};
