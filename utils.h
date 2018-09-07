/* Author: Matthew MacDonald */
/* Year: 2018 */
#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using SDL2 for the base window and OpenGL context init */
#include <SDL2/SDL.h>

/* Read in file and return as string*/
char * read_file(char * file_name);

/* Create a shader. Returns 0 on fail */
GLuint create_shader(char * file, GLenum type );

#endif
