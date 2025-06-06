// mathlib.c -- math primitives

#include "../common/mathlib.h"
#include "../common/const.h"
#include <math.h>

// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2 

#pragma warning(disable : 4244)

vec3_t vec3_origin = {0,0,0};
int nanmask = 255<<23;

float	anglemod(float a)
{
	a = 360.0f/65536 * ((int)(a*(65536/360.0f)) & 65535);
	return a;
}

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float angle = angles[YAW] * (M_PI * 2 / 360);
	const float sy = sin(angle);
	const float cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	const float sp = sin(angle);
	const float cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	const float sr = sin(angle);
	const float cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = -1*sr*sp*cy+-1*cr*-sy;
		right[1] = -1*sr*sp*sy+-1*cr*cy;
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = cr*sp*cy+-sr*-sy;
		up[1] = cr*sp*sy+-sr*cy;
		up[2] = cr*cp;
	}
}

void AngleVectorsTranspose (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float angle = angles[YAW] * (M_PI * 2 / 360);
	const float sy = sin(angle);
	const float cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	const float sp = sin(angle);
	const float cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	const float sr = sin(angle);
	const float cr = cos(angle);

	if (forward)
	{
		forward[0]	= cp*cy;
		forward[1]	= sr*sp*cy+cr*-sy;
		forward[2]	= cr*sp*cy+-sr*-sy;
	}
	if (right)
	{
		right[0]	= cp*sy;
		right[1]	= sr*sp*sy+cr*cy;
		right[2]	= cr*sp*sy+-sr*cy;
	}
	if (up)
	{
		up[0]		= -sp;
		up[1]		= sr*cp;
		up[2]		= cr*cp;
	}
}


void AngleMatrix (const vec3_t angles, float (*matrix)[4] )
{
	float angle = angles[YAW] * (M_PI * 2 / 360);
	const float sy = sin(angle);
	const float cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	const float sp = sin(angle);
	const float cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	const float sr = sin(angle);
	const float cr = cos(angle);

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp*cy;
	matrix[1][0] = cp*sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[2][1] = sr*cp;
	matrix[0][2] = cr*sp*cy+-sr*-sy;
	matrix[1][2] = cr*sp*sy+-sr*cy;
	matrix[2][2] = cr*cp;
	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

void AngleIMatrix (const vec3_t angles, float matrix[3][4] )
{
	float angle = angles[YAW] * (M_PI * 2 / 360);
	const float sy = sin(angle);
	const float cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	const float sp = sin(angle);
	const float cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	const float sr = sin(angle);
	const float cr = cos(angle);

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp*cy;
	matrix[0][1] = cp*sy;
	matrix[0][2] = -sp;
	matrix[1][0] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[1][2] = sr*cp;
	matrix[2][0] = cr*sp*cy+-sr*-sy;
	matrix[2][1] = cr*sp*sy+-sr*cy;
	matrix[2][2] = cr*cp;
	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}


void VectorTransform (const vec3_t in1, float in2[3][4], vec3_t out)
{
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
}


int VectorCompare (const vec3_t v1, const vec3_t v2)
{
	for (int i = 0 ; i<3 ; i++)
		if (v1[i] != v2[i])
			return 0;
			
	return 1;
}

void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

double sqrt(double x);

float Length(const vec3_t v)
{
	float length = 0;
	for (int i = 0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

float VectorNormalize (vec3_t v)
{
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt (length);		// FIXME

	if (length)
	{
		const float ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;

}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (const vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}


int Q_log2(int val)
{
	int answer=0;
	while (val>>=1)
		answer++;
	return answer;
}

void VectorMatrix( vec3_t forward, vec3_t right, vec3_t up)
{
	vec3_t tmp;

	if (forward[0] == 0.0f && forward[1] == 0.0f)
	{
		right[0] = 1;	
		right[1] = 0; 
		right[2] = 0;
		up[0] = -forward[2]; 
		up[1] = 0; 
		up[2] = 0;
		return;
	}

	tmp[0] = 0; tmp[1] = 0; tmp[2] = 1.0f;
	CrossProduct( forward, tmp, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
	VectorNormalize( up );
}


void VectorAngles( const vec3_t forward, vec3_t angles )
{
	float yaw, pitch;
	
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = atan2(forward[1], forward[0]) * 180 / M_PI;
		if (yaw < 0)
			yaw += 360;

		const float tmp = sqrt(forward[0] * forward[0] + forward[1] * forward[1]);
		pitch = atan2(forward[2], tmp) * 180 / M_PI;
		if (pitch < 0)
			pitch += 360;
	}
	
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}