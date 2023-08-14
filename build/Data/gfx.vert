// Flipper GFX Engine vertex shader
#version 330 core

// Define the input attributes of the vertex passed through the VBO

in uint in_PosMatIdx;
in uint in_Tex0MatIdx;
in uint in_Tex1MatIdx;
in uint in_Tex2MatIdx;
in uint in_Tex3MatIdx;
in uint in_Tex4MatIdx;
in uint in_Tex5MatIdx;
in uint in_Tex6MatIdx;
in uint in_Tex7MatIdx;
in vec3 in_Position;
in vec3 in_Normal;
in vec3 in_Binormal;
in vec3 in_Tangent;
in uvec4 in_Color0;         // RGBA
in uvec4 in_Color1;
in vec2 in_TexCoord0;
in vec2 in_TexCoord1;
in vec2 in_TexCoord2;
in vec2 in_TexCoord3;
in vec2 in_TexCoord4;
in vec2 in_TexCoord5;
in vec2 in_TexCoord6;
in vec2 in_TexCoord7;

// Define uniforms to be updated when the XF registers of the block are updated

uniform struct Light
{
    uvec3 reserved;
    uvec4 rgba;
    vec3 a;
    vec3 k;
    vec3 lpx;
    vec3 dhx;
} ligth[8];

void main(void) 
{
    gl_Position = vec4(in_Position.x, in_Position.y, in_Position.z, 1.0);
}
