#include "obj3d.h"

Obj3d::Obj3d(bool hasTx){
	vertexLoc = 0;
	normalLoc = 0;
	texCoordLoc = 0;

	hasTx?hasTextures=1:hasTextures=0;

		// proj, modelview matrices are uniforms
	modelMatrixLoc= 0;

	matRot[0] = 1;
	matRot[1] = 0;
	matRot[2] = 0;
	matRot[3] = 0;

	matRot[4] = 0;
	matRot[5] = 1;
	matRot[6] = 0;
	matRot[7] = 0;

	matRot[8] = 0;
	matRot[9] = 0;
	matRot[10] = 1;
	matRot[11] = 0;

	matRot[12] = 0;
	matRot[13] = 0;
	matRot[14] = 0;
	matRot[15] = 1;

		//get uniform locations for frag shader material stuff
	diffuseLoc = 0;
	ambientLoc = 0;
	specularLoc = 0;
	emissiveLoc = 0;
	shininessLoc = 0;
	opacityLoc = 0;

	texUnitLoc = 0;

	shaderProg = 0;

	cubeHandle = 0;
	scene = 0;
	texCountLoc = 0;
}

void Obj3d::start(GLuint shaderPrg, const char * path, Obj3d &obj){
	// Grab location of shader attributes.
	vertexLoc = glGetAttribLocation(shaderPrg , "position");
	normalLoc = glGetAttribLocation(shaderPrg , "normal");
	texCoordLoc = glGetAttribLocation(shaderPrg , "texCoord");

	// proj, modelview matrices are uniforms
	modelMatrixLoc= glGetUniformLocation(shaderPrg, "modelMatrix");

	//get uniform locations for frag shader material stuff
	diffuseLoc = glGetUniformLocation(shaderPrg, "diffuse");
	ambientLoc = glGetUniformLocation(shaderPrg, "ambient");
	specularLoc = glGetUniformLocation(shaderPrg, "specular");
	emissiveLoc = glGetUniformLocation(shaderPrg, "emissive");
	shininessLoc = glGetUniformLocation(shaderPrg, "shininess");
	opacityLoc = glGetUniformLocation(shaderPrg, "opacity");
	texUnitLoc = glGetUniformLocation(shaderPrg, "texUnit");

	shaderProg = shaderPrg;

	cubeHandle = 0;
	scene = 0;
	texCountLoc = 0;

	loadAsset(path, obj);

	glEnable( GL_CULL_FACE );
	glEnable(GL_DEPTH_TEST);
	glEnable( GL_BLEND );
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

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
		//TODO Horrible hardcoded texture paths
		if (!filename.compare(0,2,"D:"))
		{
			//we have Mustang's hardcoded textures!
			filename.erase(0,27);
		}
		//end of hardcoding
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
			printf("Image loaded: %s, %i out of %i textures\n", filename.c_str(), i+1, numTextures);
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
		if(hasTextures){
			printf("habemus textureas!\n");
			LoadGLTextures(asset.scene, asset);
		}
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

			float Opacity = 0.0;
			aiGetMaterialFloat(mtl, AI_MATKEY_OPACITY, &Opacity);
			aMat.opacity = Opacity;

			aMesh.matInfo = aMat;
			asset.myMeshes.push_back(aMesh);
		}
	}else
	{
		printf ("could not load model. misstype?\n");
	}
}

void Obj3d::draw(float * matMV, float * matP, GLuint matMVLoc, GLuint matPLoc)
{
	glUseProgram(shaderProg);
	glUniformMatrix4fv( matMVLoc, 1, 0, matMV );
	glUniformMatrix4fv( matPLoc, 1, 0, matP );

	//Renders the car
	recursive_render(matRot, scene->mRootNode, *this);

}

/***************************************************************************************
* renders recusrively
***************************************************************************************/

void Obj3d::recursive_render (float * currentTransform, const struct aiNode* nd, Obj3d &asset)
{
	// Get node transformation matrix
	struct aiMatrix4x4 m = nd->mTransformation;
	
	m.Transpose();

	m*= aiMatrix4x4(currentTransform[0], currentTransform[1], currentTransform[2], currentTransform[3],
			currentTransform[4], currentTransform[5], currentTransform[6], currentTransform[7],
			currentTransform[8], currentTransform[9], currentTransform[10], currentTransform[11],
			currentTransform[12], currentTransform[13], currentTransform[14], currentTransform[15]);

//	fslPrintMatrix4x4(m[0]);
//	printf("\n");

	glUniformMatrix4fv( asset.modelMatrixLoc, 1, 0, m[0] );

	for (unsigned int n=0; n < nd->mNumMeshes; ++n){
		// bind material uniform
		glUniform4fv( asset.emissiveLoc, 1, asset.myMeshes[nd->mMeshes[n]].matInfo.emissive);
		glUniform4fv( asset.diffuseLoc, 1, asset.myMeshes[nd->mMeshes[n]].matInfo.diffuse);
		glUniform4fv( asset.ambientLoc, 1, asset.myMeshes[nd->mMeshes[n]].matInfo.ambient);
		glUniform4fv( asset.specularLoc, 1, asset.myMeshes[nd->mMeshes[n]].matInfo.specular);
		glUniform1f( asset.shininessLoc, asset.myMeshes[nd->mMeshes[n]].matInfo.shininess);
		glUniform1f( asset.opacityLoc, asset.myMeshes[nd->mMeshes[n]].matInfo.opacity);
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

		//if (asset.myMeshes[nd->mMeshes[n]].matInfo.opacity < 1){
		glDrawElements(GL_TRIANGLES, asset.myMeshes[nd->mMeshes[n]].numFaces*3, GL_UNSIGNED_SHORT, 0); 			
		//printf("n=%i, op=%f\n",n,asset.myMeshes[nd->mMeshes[n]].matInfo.opacity);
		//}

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
			//printf("stored\n");
			recursive_render(m[0], nd->mChildren[n], asset);
		} else
		{
			//printf("new\n");
			//float id[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
			recursive_render(m[0], nd->mChildren[n], asset);
		}
	}
}

