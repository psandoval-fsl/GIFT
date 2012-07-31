uniform mat4 projMatrix;
uniform mat4 viewMatrix;

attribute vec3 positionSB;

varying vec3 TexCoord;

void main() 
{
	gl_Position = projMatrix * viewMatrix * vec4(positionSB, 1.0);
	TexCoord = normalize(positionSB);
}