#pragma once

#include "common/d3dUtil.h"
#include "MathXMFLOAT.h"

class PointBuffer;

class GridContainer
{
public:
	GridContainer() {};
	// Spatial Subdivision
	void init(const XMFLOAT3 box_min, const XMFLOAT3 box_max, float sim_scale, float cell_size, float border);
	void insertParticles(PointBuffer* pointBuffer);
	void findCells(const XMFLOAT3 p, float radius, int* gridCell);
	int findCell(const XMFLOAT3& p);
	int getGridData(int gridIndex);

	const XMFLOAT3* getGridRes(void) const { return &m_GridRes; }
	const XMFLOAT3* getGridMin(void) const { return &m_GridMin; }
	const XMFLOAT3* getGridMax(void) const { return &m_GridMax; }
	const XMFLOAT3* getGridSize(void) const { return &m_GridSize; }

	int getGridCellIndex(float px, float py, float pz);
private:
	// Spatial Grid
	int*            	m_gridData;
	XMFLOAT3			m_GridMin;				// volume of grid (may not match domain volume exactly)
	XMFLOAT3			m_GridMax;
	XMFLOAT3  	    	m_GridRes;				// resolution in each axis
	XMFLOAT3			m_GridSize;				// physical size in each axis
	XMFLOAT3			m_GridDelta;
	float				m_GridCellsize;
	int                 m_gridTotal;
};