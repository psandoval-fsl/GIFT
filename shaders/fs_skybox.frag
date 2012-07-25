uniform samplerCube Sampler;

varying mediump vec3 TexCoord;

 void main() {
     gl_FragColor = textureCube(Sampler, TexCoord);
 }