#pragma once
#include "PointBuffer.h"
#include "NeighborTable.h"
#include "GridContainer.h"
#include <xnamath.h>

class FluidMain
{
public:
	FluidMain();
	~FluidMain();

	void Init(unsigned short maxPointCounts,
		const XMFLOAT3 wallBox_min, const XMFLOAT3 wallBox_max,
		const XMFLOAT3 initFluidBox_min, const XMFLOAT3 initFluidBox_max,
		const XMFLOAT3 gravity);
	
		

	UINT getPointStride(void) const { return sizeof(Point); }
	UINT getPointCounts(void) const { return m_pointBuffer.size(); }
	XMFLOAT3* getPointBuf(void) const { return (XMFLOAT3*)m_pointBuffer.GetPoint(0); }
	void tick(void);

private:
	void _computePressure(void);
	void _computeForce(void);
	void _advance(void);
	void _addFluidVolume(const XMFLOAT3 initFluidBox_min,
		const XMFLOAT3 initFluidBox_max, float spacing);

private:
	PointBuffer		m_pointBuffer;
	GridContainer   m_gridContainer;
	NeighborTable   m_neighborTable;

	// SPH Kernel
	float m_kernelPoly6;
	float m_kernelSpiky;
	float m_kernelViscosity;

	FluidSystem m_fluidSystem;

};