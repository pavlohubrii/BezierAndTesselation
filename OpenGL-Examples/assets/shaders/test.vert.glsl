#version 450 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 texCoord;

out vec2 v_TexCoord;

uniform mat4 u_ViewProjection;

void main()
{
	v_TexCoord = texCoord;
	gl_Position = u_ViewProjection * vec4(a_Position, 0.0f, 1.0f);
}