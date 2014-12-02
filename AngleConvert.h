#ifndef ANGLECONVERT_H_INCLUDED
#define ANGLECONVERT_H_INCLUDED
#define _USE_MATH_DEFINES
#include<math.h>
const float DEGPI=(float)M_PI/180.0f;
const float PIDEG=180.0f/(float)M_PI;
float DegToRad(float deg);
float RadToDeg(float rad);
#endif