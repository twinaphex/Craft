#include <math.h>
#include <float.h>
#include "config.h"
#include "matrix.h"
#include "util.h"

void normalize(float *x, float *y, float *z)
{
   float d = sqrtf((*x) * (*x) + (*y) * (*y) + (*z) * (*z));
   if (d < FLT_MIN) {
      *x = 0.0f;
      *y = 0.0f;
      *z = 0.0f;
   } else {
      *x /= d;
      *y /= d;
      *z /= d;
   }
}

void mat_identity(float *matrix)
{
   matrix[0] = 1;
   matrix[1] = 0;
   matrix[2] = 0;
   matrix[3] = 0;
   matrix[4] = 0;
   matrix[5] = 1;
   matrix[6] = 0;
   matrix[7] = 0;
   matrix[8] = 0;
   matrix[9] = 0;
   matrix[10] = 1;
   matrix[11] = 0;
   matrix[12] = 0;
   matrix[13] = 0;
   matrix[14] = 0;
   matrix[15] = 1;
}

void mat_translate(float *matrix, float dx, float dy, float dz)
{
   matrix[0] = 1;
   matrix[1] = 0;
   matrix[2] = 0;
   matrix[3] = 0;
   matrix[4] = 0;
   matrix[5] = 1;
   matrix[6] = 0;
   matrix[7] = 0;
   matrix[8] = 0;
   matrix[9] = 0;
   matrix[10] = 1;
   matrix[11] = 0;
   matrix[12] = dx;
   matrix[13] = dy;
   matrix[14] = dz;
   matrix[15] = 1;
}

void mat_rotate(float *matrix, float x, float y, float z, float angle)
{
   float s, c, m;
   normalize(&x, &y, &z);
   s = sinf(angle);
   c = cosf(angle);
   m = 1 - c;
   matrix[0] = m * x * x + c;
   matrix[1] = m * x * y - z * s;
   matrix[2] = m * z * x + y * s;
   matrix[3] = 0;
   matrix[4] = m * x * y + z * s;
   matrix[5] = m * y * y + c;
   matrix[6] = m * y * z - x * s;
   matrix[7] = 0;
   matrix[8] = m * z * x - y * s;
   matrix[9] = m * y * z + x * s;
   matrix[10] = m * z * z + c;
   matrix[11] = 0;
   matrix[12] = 0;
   matrix[13] = 0;
   matrix[14] = 0;
   matrix[15] = 1;
}

void mat_vec_multiply(float *vector, float *a, float *b)
{
   unsigned i;
   float result[4];
   for (i = 0; i < 4; i++)
   {
      int j;
      float total = 0;
      for (j = 0; j < 4; j++)
      {
         int p = j * 4 + i;
         int q = j;
         total += a[p] * b[q];
      }
      result[i] = total;
   }

   for (i = 0; i < 4; i++)
      vector[i] = result[i];
}

void mat_multiply(float *matrix, float *a, float *b)
{
   int c;
   float result[16];

   for (c = 0; c < 4; c++)
   {
      int r;
      for (r = 0; r < 4; r++)
      {
         int i;
         int index = c * 4 + r;
         float total = 0;
         for (i = 0; i < 4; i++)
         {
            int p = i * 4 + r;
            int q = c * 4 + i;
            total += a[p] * b[q];
         }
         result[index] = total;
      }
   }

   for (int i = 0; i < 16; i++)
      matrix[i] = result[i];
}

void mat_apply(float *data, float *matrix, int count, int offset, int stride)
{
   int i;
   float vec[4] = {0, 0, 0, 1};

   for (i = 0; i < count; i++)
   {
      float *d = data + offset + stride * i;
      vec[0] = *(d++); vec[1] = *(d++); vec[2] = *(d++);
      mat_vec_multiply(vec, matrix, vec);
      d = data + offset + stride * i;
      *(d++) = vec[0]; *(d++) = vec[1]; *(d++) = vec[2];
   }
}

void frustum_planes(float planes[6][4], int radius, float *matrix)
{
   float znear  = 0.125;
   float zfar   = radius * 32 + 64;
   float *m     = matrix;

   planes[0][0] = m[3] + m[0];
   planes[0][1] = m[7] + m[4];
   planes[0][2] = m[11] + m[8];
   planes[0][3] = m[15] + m[12];
   planes[1][0] = m[3] - m[0];
   planes[1][1] = m[7] - m[4];
   planes[1][2] = m[11] - m[8];
   planes[1][3] = m[15] - m[12];
   planes[2][0] = m[3] + m[1];
   planes[2][1] = m[7] + m[5];
   planes[2][2] = m[11] + m[9];
   planes[2][3] = m[15] + m[13];
   planes[3][0] = m[3] - m[1];
   planes[3][1] = m[7] - m[5];
   planes[3][2] = m[11] - m[9];
   planes[3][3] = m[15] - m[13];
   planes[4][0] = znear * m[3] + m[2];
   planes[4][1] = znear * m[7] + m[6];
   planes[4][2] = znear * m[11] + m[10];
   planes[4][3] = znear * m[15] + m[14];
   planes[5][0] = zfar * m[3] - m[2];
   planes[5][1] = zfar * m[7] - m[6];
   planes[5][2] = zfar * m[11] - m[10];
   planes[5][3] = zfar * m[15] - m[14];
}

void mat_frustum(
    float *matrix, float left, float right, float bottom,
    float top, float znear, float zfar)
{
   float temp  = 2.0 * znear;
   float temp2 = right - left;
   float temp3 = top - bottom;
   float temp4 = zfar - znear;

   if (temp2 < FLT_MIN)
      temp2 = FLT_MIN;
   if (temp3 < FLT_MIN)
      temp3 = FLT_MIN;
   if (temp4 < FLT_MIN)
      temp4 = FLT_MIN;

   matrix[0] = temp / temp2;
   matrix[1] = 0.0;
   matrix[2] = 0.0;
   matrix[3] = 0.0;
   matrix[4] = 0.0;
   matrix[5] = temp / temp3;
   matrix[6] = 0.0;
   matrix[7] = 0.0;
   matrix[8] = (right + left) / temp2;
   matrix[9] = (top + bottom) / temp3;
   matrix[10] = (-zfar - znear) / temp4;
   matrix[11] = -1.0;
   matrix[12] = 0.0;
   matrix[13] = 0.0;
   matrix[14] = (-temp * zfar) / temp4;
   matrix[15] = 0.0;
}

void mat_perspective(
    float *matrix, float fov, float aspect,
    float znear, float zfar)
{
   float ymax = znear * tanf(fov * PI / 360.0);
   float xmax = ymax * aspect;
   mat_frustum(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);
}

void mat_ortho(
    float *matrix,
    float left, float right, float bottom, float top, float near, float far)
{
   float temp = right - left;
   float temp2 = top - bottom;
   float temp3 = far - near;

   if (temp < FLT_MIN)
      temp = FLT_MIN;
   if (temp2 < FLT_MIN)
      temp2 = FLT_MIN;
   if (temp3 < FLT_MIN)
      temp3 = FLT_MIN;

   matrix[0] = 2 / temp;
   matrix[1] = 0;
   matrix[2] = 0;
   matrix[3] = 0;
   matrix[4] = 0;
   matrix[5] = 2 / temp2;
   matrix[6] = 0;
   matrix[7] = 0;
   matrix[8] = 0;
   matrix[9] = 0;
   matrix[10] = -2 / temp3;
   matrix[11] = 0;
   matrix[12] = -(right + left) / temp;
   matrix[13] = -(top + bottom) / temp2;
   matrix[14] = -(far + near) / temp3;
   matrix[15] = 1;
}

void set_matrix_2d(float *matrix, int width, int height)
{
   mat_ortho(matrix, 0, width, 0, height, -1, 1);
}

void set_matrix_3d(
    float *matrix, int width, int height,
    float x, float y, float z, float rx, float ry,
    float fov, int ortho, int radius)
{
   float a[16];
   float b[16];
   float aspect = (height >= FLT_MIN) ? (float)width / height : 1.0f;
   float znear = 0.125;
   float zfar = radius * 32 + 64;
   mat_identity(a);
   mat_translate(b, -x, -y, -z);
   mat_multiply(a, b, a);
   mat_rotate(b, cosf(rx), 0, sinf(rx), ry);
   mat_multiply(a, b, a);
   mat_rotate(b, 0, 1, 0, -rx);
   mat_multiply(a, b, a);
   if (ortho) {
      int size = ortho;
      mat_ortho(b, -size * aspect, size * aspect, -size, size, -zfar, zfar);
   }
   else {
      mat_perspective(b, fov, aspect, znear, zfar);
   }
   mat_multiply(a, b, a);
   mat_identity(matrix);
   mat_multiply(matrix, a, matrix);
}

void set_matrix_item(float *matrix, int width, int height, int scale)
{
   float a[16];
   float b[16];
   float aspect = (height >= FLT_MIN) ? (float)width / height : 1.0f;
   float size = (scale >= FLT_MIN) ? 64 * scale : 64.0f;
   float box = height / size / 2;
   float xoffset = 1 - size / width * 2;
   float yoffset = 1 - size / height * 2;
   mat_identity(a);
   mat_rotate(b, 0, 1, 0, -PI / 4);
   mat_multiply(a, b, a);
   mat_rotate(b, 1, 0, 0, -PI / 10);
   mat_multiply(a, b, a);
   mat_ortho(b, -box * aspect, box * aspect, -box, box, -1, 1);
   mat_multiply(a, b, a);
   mat_translate(b, -xoffset, -yoffset, 0);
   mat_multiply(a, b, a);
   mat_identity(matrix);
   mat_multiply(matrix, a, matrix);
}

