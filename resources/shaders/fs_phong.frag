#ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
#else
    precision mediump float;
#endif

uniform vec4 diffuse;
uniform vec4 ambient;
uniform vec4 specular;
uniform vec4 emissive;
uniform float shininess;
uniform float opacity;

uniform samplerCube Sampler;

varying vec3 Normal;
varying mediump vec3 ReflectDir;

void main()
{
	vec4 color;
	vec4 amb;

        vec3 lightDir = normalize(vec3(0.0,.5,1.0));
        vec3 eyePos = normalize(vec3(0.0,0.5,1.0));
	vec3 n = normalize(Normal);	
	float intensity = max(dot(lightDir,n),0.0);

	float d = dot(lightDir, n);
	vec3 new = vec3(d+d) * n - lightDir; //weird, I cant do float * float so we use this fix.
        vec3 dotProduct = vec3(dot(normalize(new),eyePos));
	vec4 spec = specular * vec4(max(pow(dotProduct,vec3(3)), vec3(0)),0); //shininess instead of 5.0f

	color = diffuse; 
	amb = ambient;

	vec4 env = textureCube(Sampler, ReflectDir);

	//gl_FragColor = (color * intensity) + spec + amb + env; 
	vec4 fragColor = (color * intensity) + env*spec;
	fragColor.a = opacity;
	gl_FragColor = fragColor;
}