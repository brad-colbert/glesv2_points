/*
 * Copyright (C) 2022  Brad Colbert   All Rights Reserved.
 * Adapted from es2gears.c:
 *      Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *      Ported to GLES2. Kristian Høgsberg <krh@bitplanet.net>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#define _GNU_SOURCE

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "eglut/eglut.h"
#include "linear.h"

#define POINT_VERTEX_STRIDE 6
#define NUM_POINTS 100000

typedef GLfloat PointVertex[POINT_VERTEX_STRIDE];  // xyz,rgb
struct pointset
{
   PointVertex* vertices;
   int nvertices;
   GLuint vbo;
};
static struct pointset* points1;

/** The view rotation [x, y, z] */
static GLfloat view_rot[3] = { 0.0, 0.0, 0.0 };
/** The current gear rotation angle */
static GLfloat angle = 10.0;
/** The location of the shader uniforms */
static GLuint ModelViewProjectionMatrix_location,
              NormalMatrix_location,
              LightSourcePosition_location,
              MaterialColor_location;
/** The projection matrix */
static GLfloat ProjectionMatrix[16];
/** The direction of the directional light for the scene */
static const GLfloat LightSourcePosition[4] = { 5.0, 5.0, 10.0, 1.0};

static struct pointset*
update_points(struct pointset* points, GLuint num)
{
   GLuint vidx = 0;
   PointVertex* v = NULL;

   //
   for(vidx = 0; vidx < num; vidx++)
   {
      v = &points->vertices[vidx];
      (*v)[0] = (float)rand() / (float)(RAND_MAX); // X
      (*v)[1] = (float)rand() / (float)(RAND_MAX); // Y
      (*v)[2] = (float)rand() / (float)(RAND_MAX); // Z
      (*v)[3] = (*v)[0];                              // R
      (*v)[4] = (*v)[1];                              // R
      (*v)[5] = (*v)[2];                              // R

      #define POINT_CLOUD_SCALE 50.0
      (*v)[0] = ((*v)[0]-0.5) * POINT_CLOUD_SCALE; // X
      (*v)[1] = ((*v)[1]-0.5) * -POINT_CLOUD_SCALE; // Y
      (*v)[2] = ((*v)[2]-0.5) * -POINT_CLOUD_SCALE; // Z
   }

   /* Store the vertices in a vertex buffer object (VBO) */
   glBindBuffer(GL_ARRAY_BUFFER, points->vbo);
   glBufferData(GL_ARRAY_BUFFER, points->nvertices * sizeof(PointVertex),
                points->vertices, GL_STATIC_DRAW);

   return points;
}

static struct pointset*
create_points(GLuint num)
{
   struct pointset* points = NULL;
   PointVertex* v = NULL;
   GLuint vidx = 0;

   /* Allocate memory for the gear */
   points = malloc(sizeof *points);
   if (points == NULL)
      return NULL;

   assert(num > 0);
   points->nvertices = num;

   /* Allocate memory for the vertices */
   points->vertices = calloc(points->nvertices, sizeof(*points->vertices));

   //
   glGenBuffers(1, &points->vbo);
   update_points(points, num);

   return points;
}

static void
draw_points(struct pointset* points, GLfloat *transform,
            GLfloat x, GLfloat y, GLfloat angle, const GLfloat color[4])
{
   GLfloat model_view[16];
   GLfloat normal_matrix[16];
   GLfloat model_view_projection[16];

   /* Translate and rotate the gear */
   memcpy(model_view, transform, sizeof (model_view));
   translate(model_view, x, y, -100);
   rotate(model_view, 2 * M_PI * angle / 360.0, 0, 0, 1);

   /* Create and set the ModelViewProjectionMatrix */
   memcpy(model_view_projection, ProjectionMatrix, sizeof(model_view_projection));
   multiply(model_view_projection, model_view);

   glUniformMatrix4fv(ModelViewProjectionMatrix_location, 1, GL_FALSE,
                      model_view_projection);

   /* 
    * Create and set the NormalMatrix. It's the inverse transpose of the
    * ModelView matrix.
    */
   memcpy(normal_matrix, model_view, sizeof (normal_matrix));
   invert(normal_matrix);
   transpose(normal_matrix);
   glUniformMatrix4fv(NormalMatrix_location, 1, GL_FALSE, normal_matrix);

   /* Set the gear color */
   //glUniform4fv(MaterialColor_location, 1, color);

   /* Set the vertex buffer object to use */
   glBindBuffer(GL_ARRAY_BUFFER, points->vbo);

   /* Set up the position of the attributes in the vertex buffer object */
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
         6 * sizeof(GLfloat), NULL);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
         6 * sizeof(GLfloat), (GLfloat *) 0 + 3);

   /* Enable the attributes */
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);

   /* Draw the triangle strips that comprise the gear */
   glDrawArrays(GL_POINTS, 0, points->nvertices);

   /* Disable the attributes */
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(0);
}

/** 
 * Draws the scene.
 */
static void
scene_draw(void)
{
   const static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   const static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   const static GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };
   GLfloat transform[16];
   identity(transform);

   glClearColor(0.0, 0.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* Translate and rotate the view */
   translate(transform, 0, 0, -40);
   rotate(transform, 2 * M_PI * view_rot[0] / 360.0, 1, 0, 0);
   rotate(transform, 2 * M_PI * view_rot[1] / 360.0, 0, 1, 0);
   rotate(transform, 2 * M_PI * view_rot[2] / 360.0, 0, 0, 1);

   draw_points(points1, transform, 0.0, 0.0, 0.0, green);
}

/** 
 * Handles a new window size or exposure.
 * 
 * @param width the window width
 * @param height the window height
 */
static void
window_reshape(int width, int height)
{
   /* Update the projection matrix */
   GLfloat h = (GLfloat)height / (GLfloat)width;
   frustum(ProjectionMatrix, -1.0, 1.0, -h, h, 5.0, 200.0);

   /* Set the viewport */
   glViewport(0, 0, (GLint) width, (GLint) height);
}

/** 
 * Handles special eglut events.
 * 
 * @param special the event to handle.
 */
static void
event_handling(int special)
{
   switch (special) {
      case EGLUT_KEY_LEFT:
         view_rot[1] += 5.0;
         break;
      case EGLUT_KEY_RIGHT:
         view_rot[1] -= 5.0;
         break;
      case EGLUT_KEY_UP:
         view_rot[0] += 5.0;
         break;
      case EGLUT_KEY_DOWN:
         view_rot[0] -= 5.0;
         break;
   }
}

// Update function between frame rendering
static void
scene_idle(void)
{
   static int point_update_count = 0;
   static int frames = 0;
   double dt, t = eglutGet(EGLUT_ELAPSED_TIME) / 1000.0;

   if(point_update_count > 60)
   {
      update_points(points1, 100000);
      point_update_count = 0;
   }
   point_update_count++;

   eglutPostRedisplay();
   frames++;
}

static const char vertex_shader_points[] =
"attribute vec3 position;\n"
"attribute vec3 color;\n"
"\n"
"uniform mat4 ModelViewProjectionMatrix;\n"
"uniform mat4 NormalMatrix;\n"
"uniform vec4 LightSourcePosition;\n"
"uniform vec4 MaterialColor;\n"
"\n"
"varying vec4 Color;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_PointSize = 4.0;\n"
"\n"
"    // Multiply the diffuse value by the vertex color (which is fixed in this case)\n"
"    // to get the actual color that we will use to draw this vertex with\n"
"    Color = vec4(color, 1.0); //vec4((ambient + diffuse) * MaterialColor.xyz, MaterialColor.a);\n"
"\n"
"    // Transform the position to clip coordinates\n"
"    gl_Position = ModelViewProjectionMatrix * vec4(position, 1.0);\n"
"}";

static const char fragment_shader[] =
"precision mediump float;\n"
"varying vec4 Color;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = Color;\n"
"}";

static void
points_init(void)
{
   GLuint v, f, program;
   const char *p;
   char msg[512];

   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);

   /* Compile the vertex shader */
   p = vertex_shader_points;
   v = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(v, 1, &p, NULL);
   glCompileShader(v);
   glGetShaderInfoLog(v, sizeof msg, NULL, msg);
   printf("vertex shader info: %s\n", msg);

   /* Compile the fragment shader */
   p = fragment_shader;
   f = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(f, 1, &p, NULL);
   glCompileShader(f);
   glGetShaderInfoLog(f, sizeof msg, NULL, msg);
   printf("fragment shader info: %s\n", msg);

   /* Create and link the shader program */
   program = glCreateProgram();
   glAttachShader(program, v);
   glAttachShader(program, f);
   glBindAttribLocation(program, 0, "position");
   glBindAttribLocation(program, 1, "color");

   glLinkProgram(program);
   glGetProgramInfoLog(program, sizeof msg, NULL, msg);
   printf("info: %s\n", msg);

   /* Enable the shaders */
   glUseProgram(program);

   /* Get the locations of the uniforms so we can access them */
   ModelViewProjectionMatrix_location = glGetUniformLocation(program, "ModelViewProjectionMatrix");
   NormalMatrix_location = glGetUniformLocation(program, "NormalMatrix");
   LightSourcePosition_location = glGetUniformLocation(program, "LightSourcePosition");
   MaterialColor_location = glGetUniformLocation(program, "MaterialColor");

   /* Set the LightSourcePosition uniform which is constant throught the program */
   glUniform4fv(LightSourcePosition_location, 1, LightSourcePosition);

   /* make the gears */
   points1 = create_points(NUM_POINTS);
}

int
main(int argc, char *argv[])
{
   /* Initialize the window */
   eglutInitWindowSize(1280, 720);
   eglutInitAPIMask(EGLUT_OPENGL_ES2_BIT);
   eglutInit(argc, argv);

   eglutCreateWindow("gles2");

   /* Set up eglut callback functions */
   eglutIdleFunc(scene_idle);
   eglutReshapeFunc(window_reshape);
   eglutDisplayFunc(scene_draw);
   eglutSpecialFunc(event_handling);

   /* Initialize the gears */
   points_init();

   eglutMainLoop();

   return 0;
}
