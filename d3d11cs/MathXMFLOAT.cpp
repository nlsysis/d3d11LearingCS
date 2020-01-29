#include "MathXMFLOAT.h"

void subtractF3(XMFLOAT3& in_flo3, float in_data)
{
	in_flo3.x -= in_data;
	in_flo3.y -= in_data;
	in_flo3.z -= in_data;
}

void addtractF3(XMFLOAT3& in_flo3, float in_data)
{
	in_flo3.x += in_data;
	in_flo3.y += in_data;
	in_flo3.z += in_data;
}

void divisionF3(XMFLOAT3& in_flo3, float in_data)
{
	in_flo3.x /= in_data;
	in_flo3.y /= in_data;
	in_flo3.z /= in_data;
}

XMFLOAT3 multiplyF3(XMFLOAT3 in_flo3, float in_data)
{
	XMFLOAT3 temp_flo3 = in_flo3;
	temp_flo3.x *= in_data;
	temp_flo3.y *= in_data;
	temp_flo3.z *= in_data;

	return temp_flo3;
}

void XMFloat3Sub(XMFLOAT3& in_flo3, XMFLOAT3 in_data)
{
	in_flo3.x -= in_data.x;
	in_flo3.y -= in_data.y;
	in_flo3.z -= in_data.z;
}

void XMFloat3Add(XMFLOAT3& in_flo3, XMFLOAT3 in_data)
{
	in_flo3.x += in_data.x;
	in_flo3.y += in_data.y;
	in_flo3.z += in_data.z;
}

XMFLOAT3 XMFlo3Add(XMFLOAT3 in_flo3, XMFLOAT3 in_data)
{
	XMFLOAT3 temp_flo3 = in_flo3;
	temp_flo3.x += in_data.x;
	temp_flo3.y += in_data.y;
	temp_flo3.z += in_data.z;

	return temp_flo3;
}


XMFLOAT3 XMFloatSub(XMFLOAT3 in_flo3, XMFLOAT3 in_data)
{
	XMFLOAT3 temp_flo3 = in_flo3;
	temp_flo3.x -= in_data.x;
	temp_flo3.y -= in_data.y;
	temp_flo3.z -= in_data.z;

	return temp_flo3;
}

float Len_sq(XMFLOAT3 in_flo3)
{
	return in_flo3.x * in_flo3.x + in_flo3.y * in_flo3.y + in_flo3.z *in_flo3.z;
}

float XMF3Dot(XMFLOAT3 in_flo3, XMFLOAT3 in_data)
{
	return in_flo3.x * in_data.x + in_flo3.y * in_data.y + in_flo3.z * in_data.z;
}

void XMFloat3Div(XMFLOAT3& in_flo3, XMFLOAT3 in_data)
{
	in_flo3.x /= in_data.x;
	in_flo3.y /= in_data.y;
	in_flo3.z /= in_data.z;
}
