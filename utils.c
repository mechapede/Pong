/* Author: Matthew MacDonald */
/* Year: 2018 */
#include <stdio.h>
#include <stdbool.h>
#include "utils.h"

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using SDL2 for the base window and OpenGL context init */
#include <SDL2/SDL.h>

/*
 * Read in file and return it as a string (Null terminated)
 */
char * read_file(char * file_name){
	SDL_RWops * file =  SDL_RWFromFile(file_name, "r");
	if( file == NULL){
		fprintf(stderr, "Failed to open file %s\n", file_name);
	}
	
	Sint64 file_size = SDL_RWsize(file);
	char * file_buffer = malloc(file_size + 1);
	if( file_buffer == NULL){
		fprintf(stderr,"Fatal error, malloc allocation failed");
		return NULL;
	}
	Sint64 num_read = 1;
	Sint64 num_read_total = 0;
	char * buff = file_buffer;
	while( num_read_total < file_size && num_read != 0){
		num_read = SDL_RWread(file, buff,1,file_size - num_read_total);
		num_read_total += num_read;
		buff += num_read;
	}
	
	SDL_RWclose(file);
	if( num_read_total != file_size){
		fprintf(stderr,"Failed to read in file fully \n %s\n", file_name);
		free(file_buffer);
		return NULL;
	}
	
	file_buffer[num_read_total] = '\0';
	
	return file_buffer;
}

/*
 * Display compilation errors from an OpenGL shader compiler
 */
void print_log(GLuint object) {
	GLint log_length = 0;
	if (glIsShader(object)) {
		glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
	} else if (glIsProgram(object)) {
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
	} else {
		fprintf(stderr, "Printlog: Not a shader or a program\n");
		return;
	}

	char* log = malloc(log_length);
	
	if (glIsShader(object))
		glGetShaderInfoLog(object, log_length, NULL, log);
	else if (glIsProgram(object))
		glGetProgramInfoLog(object, log_length, NULL, log);
	
	fprintf(stderr,"%s",log);
	free(log);
}

/*
 * Create a shader from a null terminated string. Returns the id
 */
GLuint create_shader(char * file, GLenum type ){
	const GLchar * src = read_file(file);
	if( src == NULL){
		fprintf(stderr, "Failed to create shader, reading %s is NULL, Errors %s", file,  SDL_GetError());
		return 0;
	}
	
	GLuint res = glCreateShader(type);
	glShaderSource(res, 1, &src, NULL);
	free((void *) src);
	glCompileShader(res);
	GLint compile_ok = GL_FALSE;
	glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
	if (compile_ok == GL_FALSE) {
		fprintf(stderr,"%s:",file);
		print_log(res);
		glDeleteShader(res);
		return 0;
	}
	
	return res;
	
}
