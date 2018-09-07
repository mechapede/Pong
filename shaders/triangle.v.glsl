#version 120
attribute vec2 coord2d;
void main(void) {
  gl_Position = vec4(coord2d, 0.5, 1.0);
}
