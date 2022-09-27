#version 450 core

out vec4 FragColor;

uniform vec4 u_Color;
uniform bool textured;
in vec2 v_TexCoord;

uniform sampler2D ourTexture;

void main()
{
	if(textured)
		FragColor = texture(ourTexture, v_TexCoord);
	else
		FragColor = u_Color * vec4(1.0f);

//	FragColor = vec4(vec3(v_TexCoord.y), 1.0f);
}