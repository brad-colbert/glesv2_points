/*
 *      Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *      Ported to GLES2. Kristian Høgsberg <krh@bitplanet.net>
 */
#ifndef __LINEAR_H__
#define __LINEAR_H__

#include "types.h"

#include <GLES2/gl2.h>

#include <stdlib.h>
#include <string.h>

/** 
 * Multiplies two 4x4 matrices.
 * 
 * The result is stored in matrix m.
 * 
 * @param m the first matrix to multiply
 * @param n the second matrix to multiply
 */
void
multiply(GLfloat *m, const GLfloat *n);

/** 
 * Rotates a 4x4 matrix.
 * 
 * @param[in,out] m the matrix to rotate
 * @param angle the angle to rotate
 * @param x the x component of the direction to rotate to
 * @param y the y component of the direction to rotate to
 * @param z the z component of the direction to rotate to
 */
void
rotate(GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);


/** 
 * Translates a 4x4 matrix.
 * 
 * @param[in,out] m the matrix to translate
 * @param x the x component of the direction to translate to
 * @param y the y component of the direction to translate to
 * @param z the z component of the direction to translate to
 */
void
translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z);

/** 
 * Creates an identity 4x4 matrix.
 * 
 * @param m the matrix make an identity matrix
 */
void
identity(GLfloat *m);

/** 
 * Transposes a 4x4 matrix.
 *
 * @param m the matrix to transpose
 */
void 
transpose(GLfloat *m);

/**
 * Inverts a 4x4 matrix.
 *
 * This function can currently handle only pure translation-rotation matrices.
 * Read http://www.gamedev.net/community/forums/topic.asp?topic_id=425118
 * for an explanation.
 */
void
invert(GLfloat *m);

/** 
 * Calculate a frustum projection transformation.
 * 
 * @param m the matrix to save the transformation in
 * @param l the left plane distance
 * @param r the right plane distance
 * @param b the bottom plane distance
 * @param t the top plane distance
 * @param n the near plane distance
 * @param f the far plane distance
 */
void
frustum(GLfloat *m, GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);

#endif // __LINEAR_H__
