#include <iostream>
#include "../field/volScalarField.h"
#include "../field/volVectorField.h"
#include "../field/mesh.h"
#include <codi.hpp>

using Real = codi::RealForward;

// cacluate the residual of the primal solver
void CDResidual(Real* xW, Real* xX, Real* y, size_t nx, size_t ny, double dx, double dy, double vol, 
volScalarField& T, volVectorField& U, volScalarField& nu, volScalarField& S);

// convert the Jacobian's index into the matching x,y array index
size_t jacIndexToArrayIndex(size_t jacIndex, size_t nx, size_t ny);

// brute-force forward AD
codi::Jacobian<double> calcdRdWBruteForce(volScalarField& T, volScalarField& nu, volScalarField& S,
volVectorField& U, mesh& Mesh);

// forward AD using graph coloring, the number of evaluation = nx + 2
codi::Jacobian<double> calcdRdWColored(volScalarField& T, volScalarField& nu, volScalarField& S,
volVectorField& U, mesh& Mesh);

// forward AD using graph coloring. the number of evaluation = 1 (the residual of cellI depends 
// solely on the design variable of cellI)
codi::Jacobian<double> calcdRdXColored(volScalarField& T, volScalarField& nu, volScalarField& S,
volVectorField& U, mesh& Mesh);