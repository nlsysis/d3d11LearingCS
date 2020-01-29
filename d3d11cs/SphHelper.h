#pragma once
/**
	@brief the structures of different type of sph fluid simulation
**/
#include <Windows.h>
#include <xnamath.h>

/**this struct will be use compute shader to calculate SPH as constant buffer
	*@brief the size of the struct is 144byte.
**/
#pragma warning(push)
#pragma warning(disable:4324)   //structure was padded due to _declspec(align())
_DECLSPEC_ALIGN_16_ struct SimulationInitial
{
	UINT iNumParticles;            //the number of particles.
	float fTimeStep;               //for calculate the next position.
	float fSmoothlen;              //the h in SPH formula
	float fPressureStiffness;      //Implements this equation: Pressure = B * ((rho / rho_0)^y  - 1) as B
	float fRestDensity;            //the initial density of per particle for calculate the pressure. Implements this equation: Pressure = B * ((rho / rho_0)^y  - 1) as rho_0
	float fDensityCoef;            //fDensityCoef = m * 315 / ( 64 * pi * (h^9)) the coefficient for calculate dencity.
	float fGradPressureCoef;       //fGradPressureCoef = m * -45 / (pi * (h^6))  the coefficient for calculate acceleration caused by pressure.
	float fLapViscosityCoef;       //fLapViscosityCoef = m * u * 45 / (pi * h ^6) the cofficient for calculate acceleration caused by viscosity.
	float fWallStiffness;          //the rebound force of collision the wall

	XMFLOAT2A vGravity;           //the gracity direction and force
	XMFLOAT4  vGridDim;           //the size of cell

	XMFLOAT3A vPlanes[4];        //Map Wall Collision Planes the demo is in 2D so the plane in 4D scale.
};

/**
	*@brief this structure is for 3D fluid simulation calculated by CPU.
**/
struct FluidSystem
{
	float m_unitScale;            //the unit size.
	float m_viscosity;            //the u0 of viscosity.
	float m_restDensity;          //the initial density
	float m_pointMass;            //the mass of particle
	float m_smoothRadius;         //the h
	float m_gasConstantK;
	float m_boundartStiffness;
	float m_boundaryDampening;
	float m_speedLimiting;
	XMFLOAT3 m_gravityDir;

	XMFLOAT3 m_sphWallBox[2];    //two vector3 min/max
};

/**
	*@brief this struct the basic attributes of particles.
**/
struct Point
{
	XMFLOAT3   vPos;
	XMFLOAT3   vAccel;         //acceleration
	XMFLOAT3   vVelocity;
	XMFLOAT3   vVelocity_eval;

	float      fDensity;
	float      fPressure;

	int        next;
};

struct GridCell
{
	XMFLOAT3 range[2];    //min-0 max-2
	UINT counts;
};