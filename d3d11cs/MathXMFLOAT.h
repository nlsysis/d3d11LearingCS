#pragma once
#include "common/d3dUtil.h"
void subtractF3(XMFLOAT3& in_flo3, float in_data);

void addtractF3(XMFLOAT3& in_flo3, float in_data);

void divisionF3(XMFLOAT3& in_flo3, float in_data);
XMFLOAT3 multiplyF3(XMFLOAT3 in_flo3, float in_data);

void XMFloat3Sub(XMFLOAT3& in_flo3, XMFLOAT3 in_data);

void XMFloat3Add(XMFLOAT3& in_flo3, XMFLOAT3 in_data);
XMFLOAT3 XMFlo3Add(XMFLOAT3 in_flo3, XMFLOAT3 in_data);

XMFLOAT3 XMFloatSub(XMFLOAT3 in_flo3, XMFLOAT3 in_data);
float Len_sq(XMFLOAT3 in_flo3);
float XMF3Dot(XMFLOAT3 in_flo3, XMFLOAT3 in_data);