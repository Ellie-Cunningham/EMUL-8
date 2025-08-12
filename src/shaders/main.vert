#version 330 core

// take current cell as int
layout (location = 0) in int val;

uniform int width;
uniform float cellWidth;
uniform float cellHeight;

out VS_OUT {
  int valid;
} vs_out;

void main() {
  if(val != 0) {
    float row = float(gl_VertexID / width);
    float col = float(gl_VertexID % width);

    gl_Position = vec4(col * cellWidth - 1.0, 1.0 - (row + 1.0) * cellHeight, 0.0, 1.0);
    vs_out.valid = 1;
  }
  else {
    vs_out.valid = 0;
  }
}