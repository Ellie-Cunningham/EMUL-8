#version 330 core

uniform float red;
uniform float green;
uniform float blue;

out vec4 color;

void main() {
  color = vec4(red, green, blue, 1.0);
}