#include "PointBuffer.h"

const int ELEM_MAX = 4096;

PointBuffer::PointBuffer()
	:
	mFluidBuf(nullptr),
	mFluidCounts(0),
	mBufCapcity(0)
{
}

PointBuffer::~PointBuffer()
{
	SAFE_DELETE(mFluidBuf);
}

void PointBuffer::Reset(UINT capcity)
{
	mBufCapcity = capcity;
	SAFE_DELETE(mFluidBuf);

	if(mBufCapcity > 0)
	{
		mFluidBuf = (Point*)malloc(mBufCapcity * sizeof(Point));
		/*for (size_t i = 0; i < capcity; i++)
		{
			mFluidBuf[i].vPos = XMFLOAT3(0, 0, 0);
			mFluidBuf[i].next = 0;
			mFluidBuf[i].vVelocity = XMFLOAT3(0, 0, 0);
			mFluidBuf[i].vVelocity_eval = XMFLOAT3(0, 0, 0);
			mFluidBuf[i].vAccel = XMFLOAT3(0, 0, 0);

			mFluidBuf[i].fPressure = 0;
			mFluidBuf[i].fDensity = 0;
		}*/
	}
	mFluidCounts = 0;
}

Point* PointBuffer::AddPointReuse(void)
{
	if (mFluidCounts >= mBufCapcity)
	{
		if(mBufCapcity * 2 > ELEM_MAX)
		{
			//get a random point
			UINT index = rand() % mFluidCounts;
			return mFluidBuf + index;
		}
		//realloc point buffer
		mBufCapcity *= 2;
		Point* new_data = (Point*)malloc(mBufCapcity * sizeof(Point));
		memcpy(new_data, mFluidBuf, mFluidCounts * sizeof(Point));
		SAFE_DELETE(mFluidBuf);
		mFluidBuf = new_data;
	}

	//a new point

	Point*  point = mFluidBuf + (mFluidCounts++);
	point->vPos = XMFLOAT3(0, 0, 0);
	point->next = 0;
	point->vVelocity = XMFLOAT3(0, 0, 0);
	point->vVelocity_eval = XMFLOAT3(0, 0, 0);
	point->vAccel = XMFLOAT3(0, 0, 0);

	point->fPressure = 0;
	point->fDensity = 0;
	return point;
}
