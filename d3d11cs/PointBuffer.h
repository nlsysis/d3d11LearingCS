#pragma once
#include "common/d3dUtil.h"
#include "SphHelper.h"


class PointBuffer
{
public:
	PointBuffer() ;
	~PointBuffer();
	void Reset(UINT capcity);
	UINT size(void)  const { return mFluidCounts; }
	Point* GetPoint(UINT index) const { return mFluidBuf + index;}
	Point* AddPointReuse(void);

private:
	Point*   mFluidBuf;
	UINT   mFluidCounts;
	UINT   mBufCapcity;
};