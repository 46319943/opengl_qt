#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
uniform mat4 camera;
uniform mat4 perspective;
uniform mat4 move;
void main()
{
   gl_Position = move * perspective * camera * projection * vec4(aPos.x, aPos.y, .0f, 1.0f);
   gl_PointSize = 5;
}
