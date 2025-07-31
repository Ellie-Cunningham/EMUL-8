#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices=4) out;

uniform float cellWidth;
uniform float cellHeight;

in VS_OUT {
  int valid;
} gs_in[];

void main() {
  if(gs_in[0].valid != 0) {
    vec4 position = gl_in[0].gl_Position;

    gl_Position = position;
    EmitVertex();

    gl_Position = position + vec4(cellWidth, 0.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = position + vec4(0.0, cellHeight, 0.0, 0.0);
    EmitVertex();

    gl_Position = position + vec4(cellWidth, cellHeight, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
  }
}