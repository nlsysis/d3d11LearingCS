#include "PointBuffer.h"
#include "GridContainer.h"

void XMFloat3Div(XMFLOAT3& in_flo3, XMFLOAT3 in_data)
{
	in_flo3.x /= in_data.x;
	in_flo3.y /= in_data.y;
	in_flo3.z /= in_data.z;
}

void GridContainer::init(const XMFLOAT3 box_min, const XMFLOAT3 box_max, float sim_scale, float cell_size, float border)
{
	// Ideal grid cell size (gs) = 2 * smoothing radius = 0.02*2 = 0.04
	// Ideal domain size = k* gs / d = k*0.02*2/0.005 = k*8 = {8, 16, 24, 32, 40, 48, ..}
	// k = number of cells, gs = cell size, d = simulation scale)
	float world_cellsize = cell_size / sim_scale;

	m_GridMin = box_min;	subtractF3(m_GridMin ,border);
	m_GridMax = box_max;	addtractF3(m_GridMax, border);
	m_GridSize = m_GridMax;
	XMFloat3Sub(m_GridSize, m_GridMin);
	m_GridCellsize = world_cellsize;
	// Determine grid resolution
	m_GridRes.x = (int)ceil(m_GridSize.x / world_cellsize);
	m_GridRes.y = (int)ceil(m_GridSize.y / world_cellsize);
	m_GridRes.z = (int)ceil(m_GridSize.z / world_cellsize);
	// Adjust grid size to multiple of cell size
	m_GridSize.x = m_GridRes.x * cell_size / sim_scale;
	m_GridSize.y = m_GridRes.y * cell_size / sim_scale;
	m_GridSize.z = m_GridRes.z * cell_size / sim_scale;
	// delta = translate from world space to cell #
	m_GridDelta = m_GridRes;
	XMFloat3Div(m_GridDelta, m_GridSize);


	m_gridTotal = (int)(m_GridRes.x * m_GridRes.y * m_GridRes.z);
	m_gridData = new int[m_gridTotal];
	//m_gridData.resize(m_gridTotal);
}

//create hash grid index  table
void GridContainer::insertParticles(PointBuffer * pointBuffer)
{
	for (size_t i = 0; i < m_gridTotal; i++)
	{
		m_gridData[i] = -1;
	}

	Point* p = pointBuffer->GetPoint(0);
	for (unsigned int n = 0; n < pointBuffer->size(); n++)
	{
		int gs = getGridCellIndex(p[n].vPos.x, p[n].vPos.y, p[n].vPos.z);
		if (gs >= 0 && gs < m_gridTotal)
		{
			p[n].next = m_gridData[gs];
			m_gridData[gs] = (int)n;
		}
		else p[n].next = -1;
	}
}

void GridContainer::findCells(const XMFLOAT3 p, float radius, int * gridCell)
{
	//initial the gridcell
	for (int i = 0; i < 8; i++) gridCell[i] = -1;

	// Compute sphere range
	int sph_min_x = (int)((-radius + p.x - m_GridMin.x) * m_GridDelta.x);
	int sph_min_y = (int)((-radius + p.y - m_GridMin.y) * m_GridDelta.y);
	int sph_min_z = (int)((-radius + p.z - m_GridMin.z) * m_GridDelta.z);
	max(0, sph_min_x);
	max(0, sph_min_y);
	max(0, sph_min_z);

	gridCell[0] = (sph_min_z * m_GridRes.y + sph_min_y) * m_GridRes.x + sph_min_x;
	gridCell[1] = gridCell[0] + 1;
	gridCell[2] = (int)(gridCell[0] + m_GridRes.x);
	gridCell[3] = gridCell[2] + 1;

	if (sph_min_z + 1 < m_GridRes.z) {
		gridCell[4] = (int)(gridCell[0] + m_GridRes.y*m_GridRes.x);
		gridCell[5] = gridCell[4] + 1;
		gridCell[6] = (int)(gridCell[4] + m_GridRes.x);
		gridCell[7] = gridCell[6] + 1;
	}
	if (sph_min_x + 1 >= m_GridRes.x) {
		gridCell[1] = -1;		gridCell[3] = -1;
		gridCell[5] = -1;		gridCell[7] = -1;
	}
	if (sph_min_y + 1 >= m_GridRes.y) {
		gridCell[2] = -1;		gridCell[3] = -1;
		gridCell[6] = -1;		gridCell[7] = -1;
	}
}

int GridContainer::findCell(const XMFLOAT3 & p)
{
	int gc = getGridCellIndex(p.x, p.y, p.z);

	if (gc < 0 || gc >= m_gridTotal) return -1;
	return gc;
}

int GridContainer::getGridData(int gridIndex)
{
	if (gridIndex < 0 || gridIndex >= m_gridTotal)
		return -1;

	return m_gridData[gridIndex];
}

int GridContainer::getGridCellIndex(float px, float py, float pz)
{
	int gx = (int)((px - m_GridMin.x) * m_GridDelta.x);
	int gy = (int)((py - m_GridMin.y) * m_GridDelta.y);
	int gz = (int)((pz - m_GridMin.z) * m_GridDelta.z);
	return (gz * m_GridRes.y + gy) * m_GridRes.x + gx;
}
