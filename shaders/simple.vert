#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 rotate;
uniform mat4 camera;
uniform mat4 perspective;
uniform mat4 move;

uniform float layerHeight;
out float vertHeight;

void main()
{
    if (aPos.z != 0){
        gl_Position = move * perspective * camera * rotate* projection * vec4(aPos.x, aPos.y, -aPos.z, 1.0f);
    }
    else if (layerHeight != 0){
        gl_Position = move * perspective * camera * rotate* projection * vec4(aPos.x, aPos.y, -layerHeight, 1.0f);
    }
    else {
        gl_Position = move * perspective * camera * rotate* projection * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
    }

    gl_PointSize = 4.5;
    vertHeight = aPos.z;
}
