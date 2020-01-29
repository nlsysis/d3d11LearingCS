#include "FluidMain.h"

FluidMain::FluidMain()
{
	m_fluidSystem.m_unitScale = 0.004f;
	m_fluidSystem.m_viscosity = 1.0f;				
	m_fluidSystem.m_restDensity = 1000.f;			
	m_fluidSystem.m_pointMass = 0.0004f;			
	m_fluidSystem.m_gasConstantK = 1.0f;				
	m_fluidSystem.m_smoothRadius = 0.01f;		

	m_fluidSystem.m_boundartStiffness = 10000.f;
	m_fluidSystem.m_boundaryDampening = 256.f;
	m_fluidSystem.m_speedLimiting = 200.f;

	//Poly6 Kernel
	m_kernelPoly6 = 315.0f / (64.0f * 3.141592f * pow(m_fluidSystem.m_smoothRadius, 9));
	//Spiky Kernel
	m_kernelSpiky = -45.0f / (3.141592f * pow(m_fluidSystem.m_smoothRadius, 6));
	//Viscosity Kernel
	m_kernelViscosity = 45.0f / (3.141592f * pow(m_fluidSystem.m_smoothRadius, 6));
}

FluidMain::~FluidMain()
{
}

void FluidMain::Init(unsigned short maxPointCounts, 
	const XMFLOAT3 wallBox_min, 
	const XMFLOAT3 wallBox_max, 
	const XMFLOAT3 initFluidBox_min, 
	const XMFLOAT3 initFluidBox_max,
	const XMFLOAT3  gravity)
{
	m_pointBuffer.Reset(maxPointCounts);

	m_fluidSystem.m_sphWallBox[0] = wallBox_min;
	m_fluidSystem.m_sphWallBox[1] = wallBox_max;

	m_fluidSystem.m_gravityDir = gravity;

	// Create the particles
	float pointDistance = pow(m_fluidSystem.m_pointMass / m_fluidSystem.m_restDensity, 1.f / 3.f);  //the distance between point
	_addFluidVolume(initFluidBox_min, initFluidBox_max, pointDistance / m_fluidSystem.m_unitScale);

	// Setup grid Grid cell size (2r)	
	m_gridContainer.init(wallBox_min, wallBox_max, m_fluidSystem.m_unitScale, m_fluidSystem.m_smoothRadius*2.f, 1.0);
}

void FluidMain::tick(void)
{
	m_gridContainer.insertParticles(&m_pointBuffer);
	_computePressure();
	_computeForce();
	_advance();
}

void FluidMain::_computePressure(void)
{
	//h^2
	float h2 = m_fluidSystem.m_smoothRadius * m_fluidSystem.m_smoothRadius;

	//reset neightbor table
	m_neighborTable.reset(m_pointBuffer.size());

	for (unsigned int i = 0; i < m_pointBuffer.size(); i++)
	{
		Point* pi = m_pointBuffer.GetPoint(i);
		float sum = 0.f;
		m_neighborTable.point_prepare(i);

		int gridCell[8];
		m_gridContainer.findCells(pi->vPos, m_fluidSystem.m_smoothRadius / m_fluidSystem.m_unitScale, gridCell);

		for (int cell = 0; cell < 8; cell++)
		{
			if (gridCell[cell] == -1) continue;

			int pndx = m_gridContainer.getGridData(gridCell[cell]);
			while (pndx != -1)
			{
				Point* pj = m_pointBuffer.GetPoint(pndx);
				if (pj == pi)
				{
					sum += pow(h2, 3.f);  //self
				}
				else
				{
					XMFLOAT3 pi_pj = XMFloatSub(pi->vPos, pj->vPos);
					pi_pj.x *= m_fluidSystem.m_unitScale;
					pi_pj.y *= m_fluidSystem.m_unitScale;
					pi_pj.z *= m_fluidSystem.m_unitScale;
					float r2 = Len_sq(pi_pj);
					if (h2 > r2)
					{
						float h2_r2 = h2 - r2;
						sum += pow(h2_r2, 3.f);  //(h^2-r^2)^3

						if (!m_neighborTable.point_add_neighbor(pndx, sqrt(r2)))
						{
							m_neighborTable.point_commit();
						}
					}
				}
				pndx = pj->next;
			}
		}

		//m_kernelPoly6 = 315.0f/(64.0f * 3.141592f * h^9);
		pi->fDensity = m_kernelPoly6 * m_fluidSystem.m_pointMass * sum;
		pi->fPressure = (pi->fDensity - m_fluidSystem.m_restDensity) * m_fluidSystem.m_gasConstantK;
	}
}

void FluidMain::_computeForce(void)
{
	float h2 = m_fluidSystem.m_smoothRadius * m_fluidSystem.m_smoothRadius;

	for (unsigned int i = 0; i < m_pointBuffer.size(); i++)
	{
		Point* pi = m_pointBuffer.GetPoint(i);

		XMFLOAT3 accel_sum = {0.0f,0.0f,0.0f};
		int neighborCounts = m_neighborTable.getNeighborCounts(i);

		for (int j = 0; j < neighborCounts; j++)
		{
			unsigned short neighborIndex;
			float r;
			m_neighborTable.getNeighborInfo(i, j, neighborIndex, r);

			Point* pj = m_pointBuffer.GetPoint(neighborIndex);
			//r(i)-r(j)
			XMFLOAT3 ri_rj = XMFloatSub(pi->vPos, pj->vPos);
			ri_rj.x *= m_fluidSystem.m_unitScale;
			ri_rj.y *= m_fluidSystem.m_unitScale;
			ri_rj.z *= m_fluidSystem.m_unitScale;
			//h-r
			float h_r = m_fluidSystem.m_smoothRadius - r;
			//h^2-r^2
			float h2_r2 = h2 - r * r;

			//F_Pressure
			//m_kernelSpiky = -45.0f/(3.141592f * h^6);			
			float pterm = -m_fluidSystem.m_pointMass * m_kernelSpiky*h_r*h_r*(pi->fPressure + pj->fPressure) / (2.f * pi->fDensity * pj->fDensity);
			XMFLOAT3 temp = multiplyF3(ri_rj, pterm / r);
			XMFloat3Add(accel_sum,temp);

			//F_Viscosity
			//m_kernelViscosity = 45.0f/(3.141592f * h^6);
			float vterm = m_kernelViscosity * m_fluidSystem.m_viscosity * h_r *  m_fluidSystem.m_pointMass / (pi->fDensity * pj->fDensity);
			temp = XMFloatSub(pj->vVelocity_eval, pi->vVelocity_eval);
			temp = multiplyF3(temp, vterm);
			XMFloat3Add(accel_sum, temp);
		}

		pi->vAccel = accel_sum;
	}
}

void FluidMain::_advance(void)
{
	//fixed delta time per frame
	float deltaTime = 0.003f;

	float SL2 = m_fluidSystem.m_speedLimiting * m_fluidSystem.m_speedLimiting;

	Point* p = m_pointBuffer.GetPoint(0);

	for (unsigned int i = 0; i < m_pointBuffer.size(); i++)
	{
		

		// Compute Acceleration		
		XMFLOAT3 accel = p[i].vAccel;

		// Velocity limiting 
		float accel_2 = Len_sq(accel);
		if (accel_2 > SL2)
		{
			accel = multiplyF3(accel,m_fluidSystem.m_speedLimiting / sqrt(accel_2));
		}

		// Boundary Conditions

		// Z-axis walls
		float diff = 2 * m_fluidSystem.m_unitScale - (p[i].vPos.z - m_fluidSystem.m_sphWallBox[0].z)* m_fluidSystem.m_unitScale;
		if (diff > 0.f)
		{
			XMFLOAT3 norm(0, 0, 1);
			float adj = m_fluidSystem.m_boundartStiffness * diff - m_fluidSystem.m_boundaryDampening * XMF3Dot(norm,p[i].vVelocity_eval);
			accel.x += adj * norm.x;
			accel.y += adj * norm.y;
			accel.z += adj * norm.z;
		}

		diff = 2 * m_fluidSystem.m_unitScale - (m_fluidSystem.m_sphWallBox[1].z - p[i].vPos.z)*m_fluidSystem.m_unitScale;
		if (diff > 0.f)
		{
			XMFLOAT3 norm(0, 0, -1);
			float adj = m_fluidSystem.m_boundartStiffness * diff - m_fluidSystem.m_boundaryDampening * XMF3Dot(norm, p[i].vVelocity_eval);
			accel.x += adj * norm.x;
			accel.y += adj * norm.y;
			accel.z += adj * norm.z;
		}

		// X-axis walls
		diff = 2 * m_fluidSystem.m_unitScale - (p[i].vPos.x - m_fluidSystem.m_sphWallBox[0].x)*m_fluidSystem.m_unitScale;
		if (diff > 0.f)
		{
			XMFLOAT3 norm(1, 0, 0);
			float adj = m_fluidSystem.m_boundartStiffness * diff - m_fluidSystem.m_boundaryDampening * XMF3Dot(norm, p[i].vVelocity_eval);
			accel.x += adj * norm.x;
			accel.y += adj * norm.y;
			accel.z += adj * norm.z;
		}

		diff = 2 * m_fluidSystem.m_unitScale - (m_fluidSystem.m_sphWallBox[1].x - p[i].vPos.x)*m_fluidSystem.m_unitScale;
		if (diff > 0.f)
		{
			XMFLOAT3 norm(-1, 0, 0);
			float adj = m_fluidSystem.m_boundartStiffness * diff - m_fluidSystem.m_boundaryDampening * XMF3Dot(norm, p[i].vVelocity_eval);
			accel.x += adj * norm.x;
			accel.y += adj * norm.y;
			accel.z += adj * norm.z;
		}

		// Y-axis walls
		diff = 2 * m_fluidSystem.m_unitScale - (p[i].vPos.y - m_fluidSystem.m_sphWallBox[0].y)*m_fluidSystem.m_unitScale;
		if (diff > 0.f)
		{
			XMFLOAT3 norm(0, 1, 0);
			float adj = m_fluidSystem.m_boundartStiffness * diff - m_fluidSystem.m_boundaryDampening * XMF3Dot(norm, p[i].vVelocity_eval);
			accel.x += adj * norm.x;
			accel.y += adj * norm.y;
			accel.z += adj * norm.z;
		}
		diff = 2 * m_fluidSystem.m_unitScale - (m_fluidSystem.m_sphWallBox[1].y - p[i].vPos.y) * m_fluidSystem.m_unitScale;
		if (diff > 0.f)
		{
			XMFLOAT3 norm(0, -1, 0);
			float adj = m_fluidSystem.m_boundartStiffness * diff - m_fluidSystem.m_boundaryDampening * XMF3Dot(norm, p[i].vVelocity_eval);
			accel.x += adj * norm.x;
			accel.y += adj * norm.y;
			accel.z += adj * norm.z;
		}

		// Plane gravity
		XMFloat3Add(accel, m_fluidSystem.m_gravityDir);

		// Leapfrog Integration ----------------------------
		XMFLOAT3 vnext = XMFlo3Add(p[i].vVelocity , multiplyF3(accel , deltaTime));			// v(t+1/2) = v(t-1/2) + a(t) dt			
		p[i].vVelocity_eval = multiplyF3(XMFlo3Add(p[i].vVelocity ,vnext) , 0.5f);				// v(t+1) = [v(t-1/2) + v(t+1/2)] * 0.5		used to compute forces later
		p[i].vVelocity = vnext;
		XMFloat3Add(p[i].vPos , multiplyF3( vnext, deltaTime / m_fluidSystem.m_unitScale));		// p(t+1) = p(t) + v(t+1/2) dt
	}
}

void FluidMain::_addFluidVolume(const XMFLOAT3 initFluidBox_min,
	const XMFLOAT3 initFluidBox_max, float spacing)
{
	float cx = (initFluidBox_max.x + initFluidBox_min.x) / 2.f;
	float cy = (initFluidBox_max.y + initFluidBox_min.y) / 2.f;
	float cz = (initFluidBox_max.z + initFluidBox_min.z) / 2.f;

	for (float z = initFluidBox_max.z; z >= initFluidBox_min.z; z -= spacing)
	{
		for (float y = initFluidBox_min.y; y <= initFluidBox_max.y; y += spacing)
		{
			for (float x = initFluidBox_min.x; x <= initFluidBox_max.x; x += spacing)
			{
				Point* p = m_pointBuffer.AddPointReuse();

				p->vPos = XMFLOAT3(x, y, z);
			}
		}
	}
}
