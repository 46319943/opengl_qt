#version 330 core
uniform vec4 ourColor;


in float vertHeight;

out vec4 FragColor;
void main()
{
    if (vertHeight == 0){
        FragColor = vec4(ourColor.x, ourColor.y, ourColor.z, ourColor.w);
    }
    else {
        FragColor = vec4(.8f + vertHeight, .9f - vertHeight * 0.7, .9f - vertHeight * 0.5, .5f);
    }

}
