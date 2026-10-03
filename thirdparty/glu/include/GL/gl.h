/*
 * Minimal OpenGL types and enums for the SGI GLU tessellator (libtess) on
 * platforms without desktop OpenGL headers (iOS). Only what glu.h, libtess
 * and libcitygml's Tesselator use is defined here.
 */
#ifndef __gl_h_
#define __gl_h_

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef double GLdouble;
typedef void GLvoid;

#define GL_FALSE 0
#define GL_TRUE 1

#define GL_LINE_LOOP 0x0002
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006

#endif /* __gl_h_ */
