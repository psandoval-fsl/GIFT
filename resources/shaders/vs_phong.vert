uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

attribute vec3 position;
attribute vec3 normal;

varying vec3 Normal;
varying vec3 ReflectDir;

mat3 GetLinearPart( mat4 m )
{
	mat3 result;
	
	result[0][0] = m[0][0]; 
	result[0][1] = m[0][1]; 
	result[0][2] = m[0][2]; 

	result[1][0] = m[1][0]; 
	result[1][1] = m[1][1]; 
	result[1][2] = m[1][2]; 
	
	result[2][0] = m[2][0]; 
	result[2][1] = m[2][1]; 
	result[2][2] = m[2][2]; 
	
	return result;
}	

void main()
{
	Normal = normalize(vec3(viewMatrix * modelMatrix * vec4(normal,0.0)));	
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(position,1.0);

	 // Compute eye direction in object space:
    mediump vec3 eyeDir = normalize(position - vec3(0.0,0.0,1.0));
	
	mat3 model = GetLinearPart(viewMatrix);
    // Reflect eye direction over normal and transform to world space:
    ReflectDir = model * eyeDir;//model * reflect(eyeDir, normal);
	ReflectDir.y = -ReflectDir.y;
}
