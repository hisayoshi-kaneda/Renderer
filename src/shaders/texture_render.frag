#version 330
precision highp float;

in vec3 f_posTexture;

out vec4 out_color;

uniform sampler2D texImage;
 
void main(void){
    out_color = texelFetch(texImage, ivec2(gl_FragCoord.xy), 0);
}
