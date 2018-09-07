/* Author: Matthew MacDonald */
/* Year: 2018 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
/* Use glew.h get all the GL prototypes declared */
#include <GL/glew.h>
/* Using SDL2 for window and OpenGL context init */
#include <SDL2/SDL.h>

#include "utils.h"

/* Global engine related varriables */

#define SHADER_PATH "shaders/" //directory for glsl shaders

#define GRID_HEIGHT 768 //parameters for grid size
#define GRID_WIDTH 1280 

typedef struct { //split into opengl draw stuff
    float_t x; 
    float_t y;
    float_t speed;
    float_t direction; //in degrees
    GLuint vbo; //storage space in gpu for coordinates
    GLfloat * verticies; //for rendering, DO NOT MODIFY DIRETLY
    int32_t num_verticies; 
    int32_t height; //for the collision detection mask 
    int32_t width; //TODO: generate automaticly
} PGInstance;

PGInstance * * master_list; //list of all instances in the game 
size_t master_list_size; //size allocated
size_t master_list_index; //index of next ellement to be added

GLint attribute_coord2d; //these are shared bettwen all objects currently
GLuint program;


/* Global varriables related to the game logic/instances */

/* Put any object references here */
PGInstance * right_paddle;
PGInstance * left_paddle;
PGInstance * ball;

float_t triangle_vertices[] = { //simple triangle coordinates in game spec (not opengl)
    0.0,0.0,
    48.0,0.0,
    48.0,128.0,
    0.0,0.0,
    0.0,128.0,
    48.0,128.0
};

float_t ball_vertices[] = { //simple square coordinates in game spec (not opengl)
    0.0,0.0,
    32.0,0.0,
    32.0,32.0,
    0.0,0.0,
    0.0,32.0,
    32.0,32.0
};


/* Converts points to GL coord system, assumes pairs of x,y ( 2 GLfloat per point)*/
void convert_to_gl_space(GLfloat * points, int32_t num_points) {
    for( int32_t i = 0; i < num_points * 2; i += 2) {
        points[i] = (points[i] - GRID_WIDTH/2) / (GRID_WIDTH/2);
        points[i+1] = (points[i+1] - GRID_HEIGHT/2) / (GRID_HEIGHT/2);
    }
}


/* Adds a instance to the master_list, allowing automatic drawing and movement 
 * Must be called after init
 */
void add_to_master_list(PGInstance * obj){
    if( master_list_index == master_list_size ){
        master_list_size *= 2;
        master_list = realloc(master_list,master_list_size);
        assert(master_list);
    }
    master_list[master_list_index++] = obj;
}

/* Create and intialize an instance struct, also sets up opengl buffers*/
PGInstance * create_instance(Sint32 x, Sint32 y, Sint32 width, Sint32 height, GLfloat * verticies, Sint32 num_verticies){
    PGInstance * obj = ( PGInstance * ) malloc(sizeof(PGInstance));
    assert(obj); 
    
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * num_verticies * 2, NULL, GL_DYNAMIC_DRAW); //empty buffer for storage of coordinates
    obj->vbo = vbo;
    
    assert(glGetError() == GL_NO_ERROR); //check for opengl errors
    
    obj->x = x;
    obj->y = y;
    
    obj->speed = 0;
    obj->direction = 0;
    
    obj->verticies = verticies;
    obj->num_verticies = num_verticies;
    
    obj->height = height;
    obj->width = width;
    
    add_to_master_list(obj); //add to list for drawing physics calculations
    
    return obj;
}

/* Generate the angel offset when ball hits a paddle*/
int ball_offset(PGInstance * ball, PGInstance * paddle){
    GLfloat paddle_middle = paddle->y + ( paddle->height / 2.0 );
    GLfloat degree_offset = 30; //60 in each direction
    
    return ((ball->y - paddle_middle)/( paddle->height / 2.0 ) )* degree_offset;
}


/* Transfers a point from model coordinates to game grid coordinates */
void transform_points(GLfloat * points,Sint32 num_points, Sint32 x, Sint32 y) { //transfer model for the grid
    for( Sint32 i = 0; i <num_points * 2; i+= 2) {
        points[i] += x;
        points[i+1] += y;
    }
}

/* Renders a instance, must be called from render */
void draw_instance(PGInstance * obj, GLint attr, GLuint prog){
    GLfloat * coords = ( GLfloat * ) malloc(sizeof(GLfloat) * obj->num_verticies * 2);
    for( Sint32 i = 0; i < obj->num_verticies * 2; i++){
        coords[i] = (obj->verticies)[i];
    }
    
    transform_points(coords, obj->num_verticies, obj->x, obj->y);
    convert_to_gl_space(coords,obj->num_verticies);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(GLfloat) * obj->num_verticies * 2,coords);
    
    glEnableVertexAttribArray(attr);

    /* Describe our vertices array to OpenGL (it can't guess its format automatically) */
    glVertexAttribPointer(
        attr, // attribute
        2,                 // number of elements per vertex, here (x,y)
        GL_FLOAT,          // the type of each element
        GL_FALSE,          // take our values as-is
        0,                 // no extra data between each position
        0  // offset for buffer bined to GL_ARRAY_BUFFER
    );

    /* Push each element in buffer_vertices to the vertex shader */
    glDrawArrays(GL_TRIANGLES, 0, obj->num_verticies);
    
    glDisableVertexAttribArray(attr);
    
    assert(glGetError() == GL_NO_ERROR); //check for opengl errors
    free(coords);
    
}

/* Setup engine values, instances and opengl shader*/
int init_resources() {
    
    master_list_size = 10;
    master_list = malloc(sizeof(PGInstance *) * master_list_size);
    assert(master_list);
    master_list_index = 0;
    
    right_paddle = create_instance(GRID_WIDTH - 256, 320, 48, 128, triangle_vertices, (sizeof(triangle_vertices) / sizeof(GLfloat)) / 2);
    
    left_paddle = create_instance(128, 320, 48, 128, triangle_vertices, (sizeof(ball_vertices) / sizeof(GLfloat)) / 2);
    
    ball = create_instance(512, 368, 32, 32, ball_vertices, (sizeof(ball_vertices) / sizeof(GLfloat)) / 2);
    ball->speed = 16;

    GLint link_ok = GL_FALSE;
    GLuint vs, fs;
    if ((vs = create_shader( SHADER_PATH "triangle.v.glsl", GL_VERTEX_SHADER))   == 0) return false;
    if ((fs = create_shader( SHADER_PATH "triangle.f.glsl", GL_FRAGMENT_SHADER)) == 0) return false;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
        fprintf(stderr,"Error in glLinkProgram");
        return false;
    }

    const char* attribute_name = "coord2d";
    attribute_coord2d = glGetAttribLocation(program, attribute_name);
    if (attribute_coord2d == -1) {
        fprintf( stderr, "Could not bind attribute %s",attribute_name);
        return false;
    }
    
    assert(glGetError() == GL_NO_ERROR); //check for other errors
    
    return true;
}

/* Render the scene */
void render(SDL_Window* window) {
    /* Clear the background as white */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    //draw all the instances
    for(int i =0; i < master_list_index; i++){
        PGInstance * obj = master_list[i];
        draw_instance(obj,attribute_coord2d,program);
    }

    /* Display the result */
    SDL_GL_SwapWindow(window);
    
    assert(glGetError() == GL_NO_ERROR); //check for opengl errors
}

/*Fine level collision detection, 1 is touching, 0 otherwise */
int check_collision(PGInstance * obj1, PGInstance * obj2){
    if(obj1->x < obj2->x + obj2->width && obj1->x > obj2->x  && obj1->y + obj1->height > obj2->y && obj1->y < obj2->y + obj2->height){
        return 1;
    }
    return 0;
}

/* Update the instance based on speed and direction*/
void move_object(PGInstance * obj){
    GLfloat angel = obj->direction;
    
    angel = (M_PI * angel) / 180; //convert from degrees to radiansh
    GLfloat speed = obj->speed;
    
    obj->x += speed * cos(angel); 
    obj->y += speed * sin(angel);
    
}

/* Update the movement and handle collisions/out of bound */
void update_movement() {
    
    //update movement of all objects
    for(int i =0; i < master_list_index; i++){
        PGInstance * obj = master_list[i];
        move_object(obj);
    }
    
    if(check_collision(ball,right_paddle)){
        ball->direction = 180 - ball_offset(ball,right_paddle);
        
        
    }
    if (check_collision(ball,left_paddle)){
        ball->direction = 0 + ball_offset(ball,left_paddle);
    }
    
    if( ball->y < 0){
        ball->direction += 90;
    }else if (ball->y > GRID_HEIGHT){
        ball->direction -= 90;
    }
    
    if( ball->x > GRID_WIDTH || ball->x < 0){
        ball->x = 512;
        ball->y = 512;
        ball->direction = 0;
    }

}

/* cleanup resources */
void free_resources() {
    glDeleteProgram(program);
    glDeleteBuffers(1, &(right_paddle->vbo));
    glDeleteBuffers(1, &(left_paddle->vbo));
    glDeleteBuffers(1,&(ball->vbo));
    
    for(int i =0; i < master_list_index; i++){
        PGInstance * obj = master_list[i];
        free(obj);
    }
    
    free(master_list);
    
}

/* Game loop with logic */
void mainLoop(SDL_Window* window) {
    while (true) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) { //main input loop
            switch(ev.type) {
                case SDL_QUIT:
                    return;
                case SDL_KEYDOWN:
                    if(!ev.key.repeat) {
                        switch(ev.key.keysym.scancode) {
                            case SDL_SCANCODE_UP:
                                right_paddle->speed = 8;
                                right_paddle->direction = 90;
                                break;
                            case SDL_SCANCODE_DOWN:
                                right_paddle->speed = 8;
                                right_paddle->direction = 270;
                                break;
                            case SDL_SCANCODE_W:
                                left_paddle->speed = 8;
                                left_paddle->direction = 90;
                                break;
                            case SDL_SCANCODE_S:
                                left_paddle->speed = 8;
                                left_paddle->direction = 270;
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case SDL_KEYUP:
                    if(!ev.key.repeat) {
                        switch(ev.key.keysym.scancode) {
                            case SDL_SCANCODE_UP:
                                if(right_paddle->direction == 90){
                                    right_paddle->speed = 0;
                                }
                                break;
                            case SDL_SCANCODE_DOWN:
                                if(right_paddle->direction == 270){
                                    right_paddle->speed = 0;
                                }
                                break;
                            case SDL_SCANCODE_W:
                                if(left_paddle->direction == 90){
                                    left_paddle->speed = 0;
                                }
                                break;
                            case SDL_SCANCODE_S:
                                if(left_paddle->direction == 270){
                                    left_paddle->speed = 0;
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        update_movement();
        render(window);
    }
}

/* Contains SDL initiaztion and mainloop start */
int main(int argc, char* argv[]) {

    /* SDL-related initialization */
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("My First Triangle",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          GRID_WIDTH, GRID_HEIGHT,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if( window == NULL) {
        fprintf(stderr, "Error: cannot create the window");
        return EXIT_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
    if (SDL_GL_CreateContext(window) == NULL) {
        fprintf(stderr,"Error: SDL_GL_CreateContext: %s \n", SDL_GetError());
        return EXIT_FAILURE;
    }


    SDL_GL_CreateContext(window);

    /* Extension wrangler initialising */
    GLenum glew_status = glewInit();
    if (glew_status != GLEW_OK) {
        fprintf(stderr,"Error: glewInit: %s", glewGetErrorString(glew_status));
        return EXIT_FAILURE;
    }

    /* check opengl version */
    if (!GLEW_VERSION_2_0) {
        fprintf(stderr,"Error: your graphic card does not support OpenGL 2.0\n");
        return EXIT_FAILURE;
    }

    /* check that init passes*/
    if (!init_resources()) {
        fprintf(stderr,"Failed to initialize resources");
        return EXIT_FAILURE;
    }
    
    mainLoop(window);

    /* If the program exits in the usual way,
       free resources and exit with a success */
    free_resources();
    return EXIT_SUCCESS;
}
