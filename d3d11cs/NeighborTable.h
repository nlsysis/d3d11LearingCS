#pragma once
#include "common/d3dUtil.h"
#include "SphHelper.h"


const int MAX_NEIGHTBOR_COUNTS = 80;
class NeighborTable
{
public:
	NeighborTable();
	~NeighborTable();

	void reset(unsigned short pointCounts);          //reset neighbor table
	void point_prepare(unsigned short ptIndex);      //prepare a point neighbor data
	bool point_add_neighbor(unsigned short ptIndex, float distance);    //add neighbor data to current point
	void point_commit(void);     //commit point neighbor data to data buf
	int getNeighborCounts(unsigned short ptIndex) { return m_pointExtraData[ptIndex].neighborCounts; }
	//get point neightbor information
	void getNeighborInfo(unsigned short ptIndex, int index, unsigned short& neighborIndex, float& neighborDistance);

private:
	void growDataBuf(UINT need_size);
private:
	union PointExtraData
	{
		struct
		{
			unsigned neighborDataOffset : 24;
			unsigned neighborCounts : 8;
		};
		UINT neighborData;
	};

	PointExtraData* m_pointExtraData;
	UINT  m_pointCounts;
	UINT  m_pointCapcity;

	unsigned char*  m_neighborDataBuf;      //neighbor data buf
	UINT  m_dataBufSize;                    //in bytes
	UINT  m_dataBufOffset;                  //current neighbor data buf offset

	///temp data for current point
	unsigned short m_currPoint;
	int m_currNeighborCounts;
	unsigned short  m_currNeightborIndex[MAX_NEIGHTBOR_COUNTS];
	float m_currNeighborDistance[MAX_NEIGHTBOR_COUNTS];

};