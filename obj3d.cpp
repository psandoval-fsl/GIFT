#include "obj3d.h"

void Obj3d::set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

void Obj3d::color4_to_float4(const struct aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

int Obj3d::LoadGLTextures(const aiScene* scene, Obj3d &asset)
{
	ILboolean success;

	/* scan scene's materials for textures */
	for (unsigned int m=0; m<scene->mNumMaterials; ++m)
	{
		int texIndex = 0;
		aiString path;	// filename

		aiReturn texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
		while (texFound == AI_SUCCESS) {
			//fill map with textures, OpenGL image ids set to 0
			asset.textureIdMap[path.data] = 0; 
			// more textures?
			texIndex++;
			texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
		}
	}

	int numTextures = asset.textureIdMap.size();

	/* create and fill array with DevIL texture ids */
	ILuint* imageIds = new ILuint[numTextures];
	ilGenImages(numTextures, imageIds); 

	/* create and fill array with GL texture ids */
	GLuint* textureIds = new GLuint[numTextures];
	glGenTextures(numTextures, textureIds); /* Texture name generation */

	/* get iterator */
	std::map<std::string, GLuint>::iterator itr = asset.textureIdMap.begin();
	int i=0;
	for (; itr != asset.textureIdMap.end(); ++i, ++itr)
	{
		//save IL image ID
		std::string filename = (*itr).first;  // get filename
		(*itr).second = textureIds[i];	  // save texture id for filename in map

		ilBindImage(imageIds[i]); /* Binding of DevIL image name */
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_LOWER_LEFT); 
		success = ilLoadImage((ILstring)filename.c_str());

		if (success) {
			/* Convert image to RGBA */
			ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE); 

			/* Create and load textures to OpenGL */
			glBindTexture(GL_TEXTURE_2D, textureIds[i]); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH),
				ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE,
				ilGetData()); 
			printf("Image loaded: %s, %i out of %i textures\n", filename.c_str(), numTextures, i);
		}
		else 
			printf("Couldn't load Image: %s\n", filename.c_str());
	}
	/* Because we have already copied image data into texture data
	we can release memory used by image. */
	ilDeleteImages(numTextures, imageIds); 

	//Cleanup
	delete [] imageIds;
	delete [] textureIds;

	//return success;
	return true;
}

/***************************************************************************************
* loads asset
***************************************************************************************/
void Obj3d::loadAsset(const char * path, Obj3d &asset)
{
	asset.scene = aiImportFile(path,aiProcessPreset_TargetRealtime_Quality);
	if (asset.scene)
	{
		
		struct MyMesh aMesh;
		struct MyMaterial aMat; 
		unsigned short shortFaces[3];	
	
		// For each mesh
		for (unsigned int n = 0; n < asset.scene->mNumMeshes; ++n)
		{
			const struct aiMesh* mesh = asset.scene->mMeshes[n];

			// create array with faces
			// have to convert from Assimp format to array
			unsigned short *faceArray;
			faceArray = (unsigned short *)malloc(sizeof(unsigned short) * mesh->mNumFaces * 3);
			unsigned int faceIndex = 0;
		// we need unsigned_short for drawElements, cast uint to ushort for faces index array
			for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
				const struct aiFace* face = &mesh->mFaces[t];
				shortFaces[0] = (unsigned short)face->mIndices[0];
				shortFaces[1] = (unsigned short)face->mIndices[1];
				shortFaces[2] = (unsigned short)face->mIndices[2];
				memcpy(&faceArray[faceIndex], shortFaces, 3 * sizeof(unsigned short));
				faceIndex += 3;
			}
			aMesh.numFaces = asset.scene->mMeshes[n]->mNumFaces;

			// buffer for faces
			glGenBuffers(1, &aMesh.VBO[VBO_faces]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aMesh.VBO[VBO_faces]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * mesh->mNumFaces * 3, faceArray, GL_STATIC_DRAW);
			free(faceArray);

			// buffer for vertex positions
			if (mesh->HasPositions()) {
				glGenBuffers(1, &aMesh.VBO[VBO_vertices]);
				glBindBuffer(GL_ARRAY_BUFFER, aMesh.VBO[VBO_vertices]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
				
			}

			// buffer for vertex normals
			if (mesh->HasNormals()) {
				glGenBuffers(1, &aMesh.VBO[VBO_normals]);
				glBindBuffer(GL_ARRAY_BUFFER, aMesh.VBO[VBO_normals]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);
				
			}

			// buffer for vertex texture coordinates
			if (mesh->HasTextureCoords(0)) {
				float *texCoords = (float *)malloc(sizeof(float)*2*mesh->mNumVertices);
				for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {

					texCoords[k*2]   = mesh->mTextureCoords[0][k].x;
					texCoords[k*2+1] = mesh->mTextureCoords[0][k].y; 
				
				}
				glGenBuffers(1, &aMesh.VBO[VBO_texCoords]);
				glBindBuffer(GL_ARRAY_BUFFER, aMesh.VBO[VBO_texCoords]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
				
				free(texCoords);
				LoadGLTextures(asset.scene, asset);
			} else {
				glGenBuffers(1, &aMesh.VBO[VBO_texCoords]);
			}

			// unbind buffers
			//glBindVertexArray(0); 
			glBindBuffer(GL_ARRAY_BUFFER,0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	
			// fill aMesh.matInfo
			struct aiMaterial *mtl = asset.scene->mMaterials[mesh->mMaterialIndex];
			
			aiString texPath;	//contains filename of texture
			if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)){
					//bind texture
					unsigned int texId = asset.textureIdMap[texPath.data]; //DevIL!!
					aMesh.texIndex = texId;
				}
			else
				aMat.texCount = 0;

			float c[4];
			set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
			aiColor4D diffuse;
			if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) //(color * intensity) = diffuse
				color4_to_float4(&diffuse, c);
			memcpy(aMat.diffuse, c, sizeof(c));

			set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
			aiColor4D ambient;
			if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
				color4_to_float4(&ambient, c);
			memcpy(aMat.ambient, c, sizeof(c));

			set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
			aiColor4D specular;
			if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
				color4_to_float4(&specular, c);
			memcpy(aMat.specular, c, sizeof(c));

			set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
			aiColor4D emission;
			if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
				color4_to_float4(&emission, c);
			memcpy(aMat.emissive, c, sizeof(c));

			float shininess = 0.0;
			unsigned int max;
			aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
			aMat.shininess = shininess;
			aMesh.matInfo = aMat;
			asset.myMeshes.push_back(aMesh);
		}
	}else
	{
		printf ("could not load model. misstype?\n");
	}
}

/***************************************************************************************
* inits render
***************************************************************************************/

void Obj3d::shaderInit(GLuint &shaderProgram, Obj3d &asset)
{
	// Grab location of shader attributes.
	asset.vertexLoc = glGetAttribLocation(shaderProgram , "position");
	asset.normalLoc = glGetAttribLocation(shaderProgram , "normal"); 
	asset.texCoordLoc = glGetAttribLocation(shaderProgram , "texCoord"); 
	
	// proj, modelview matrices are uniforms
	asset.modelMatrixLoc= glGetUniformLocation(shaderProgram, "modelMatrix");
	
	//get uniform locations for frag shader material stuff
	asset.diffuseLoc = glGetUniformLocation(shaderProgram, "diffuse");
	asset.ambientLoc = glGetUniformLocation(shaderProgram, "ambient");
	asset.specularLoc = glGetUniformLocation(shaderProgram, "specular");
	asset.emissiveLoc = glGetUniformLocation(shaderProgram, "emissive");
	asset.shininessLoc = glGetUniformLocation(shaderProgram, "shininess");
	asset.texUnitLoc = glGetUniformLocation(shaderProgram, "texUnit");

	glEnable( GL_CULL_FACE );
	glEnable(GL_DEPTH_TEST);
}

/***************************************************************************************
* renders recusrively
***************************************************************************************/

void Obj3d::recursive_render (aiMatrix4x4 currentTransform, const struct aiNode* nd, Obj3d &asset)
{
	// Get node transformation matrix
	struct aiMatrix4x4 m = nd->mTransformation;
	
	m.Transpose();
	m*=currentTransform;

	glUniformMatrix4fv( asset.modelMatrixLoc, 1, 0, m[0] );

	for (unsigned int n=0; n < nd->mNumMeshes; ++n){
		// bind material uniform
		glUniform4fv( asset.emissiveLoc, 1, asset.myMeshes[nd->mMeshes[n]].matInfo.emissive);
		glUniform4fv( asset.diffuseLoc, 1, asset.myMeshes[nd->mMeshes[n]].matInfo.diffuse);
		glUniform4fv( asset.ambientLoc, 1, asset.myMeshes[nd->mMeshes[n]].matInfo.ambient);
		glUniform4fv( asset.specularLoc, 1, asset.myMeshes[nd->mMeshes[n]].matInfo.specular);
		glUniform1f( asset.shininessLoc, asset.myMeshes[nd->mMeshes[n]].matInfo.shininess);
		glUniform1i( asset.texUnitLoc,0);

		// bind texture
		glBindTexture(GL_TEXTURE_2D, asset.myMeshes[nd->mMeshes[n]].texIndex);

		// bind VBOs
		glBindBuffer(GL_ARRAY_BUFFER, asset.myMeshes[nd->mMeshes[n]].VBO[VBO_vertices]);
		glVertexAttribPointer(asset.vertexLoc, 3, GL_FLOAT, 0, 0, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asset.myMeshes[nd->mMeshes[n]].VBO[VBO_faces]);
		glEnableVertexAttribArray(asset.vertexLoc);

		glBindBuffer(GL_ARRAY_BUFFER, asset.myMeshes[nd->mMeshes[n]].VBO[VBO_normals]);
		glVertexAttribPointer(asset.normalLoc, 3, GL_FLOAT, 0, 0, 0);
		glEnableVertexAttribArray(asset.normalLoc);

		glBindBuffer(GL_ARRAY_BUFFER, asset.myMeshes[nd->mMeshes[n]].VBO[VBO_texCoords]);
		glVertexAttribPointer(asset.texCoordLoc, 2, GL_FLOAT, 0, 0, 0);
		glEnableVertexAttribArray(asset.texCoordLoc);

		glDrawElements(GL_TRIANGLES, asset.myMeshes[nd->mMeshes[n]].numFaces*3, GL_UNSIGNED_SHORT, 0); 			

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glDisableVertexAttribArray(asset.vertexLoc);
		glDisableVertexAttribArray(asset.normalLoc);
		glDisableVertexAttribArray(asset.texCoordLoc);
	}

	// draw all children
	for (unsigned int n=0; n < nd->mNumChildren; ++n){
		if(nd->mNumMeshes>0)
		{
			recursive_render(m, nd->mChildren[n], asset);
		} else
		{
			aiMatrix4x4 id(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
			id.Transpose();
			recursive_render(id, nd->mChildren[n], asset);
		}
	}
}

