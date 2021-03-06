#include <iostream>
#include <string>
#include <algorithm>
#include <codi.hpp>
#include "../field/volScalarField.h"
#include "../field/volVectorField.h"
#include "../field/mesh.h"
#include "CDSolverResidual.h"

using namespace std;
using Real = codi::RealForward;

// brute-force forward AD
codi::Jacobian<double> calcdRdWBruteForce(volScalarField& T, volScalarField& nu, volScalarField& S,
volVectorField& U, mesh& Mesh)
{
    // get the dimension of the mesh
    int nx = Mesh.getNx(), ny = Mesh.getNy();
    double dx = Mesh.dx, dy = Mesh.dy, vol = Mesh.vol;
    int pIndex, nIndex, sIndex, eIndex, wIndex;
    double Fs, Fn, Fw, Fe, Ds, Dn, De, Dw, as, an, ae, aw, ap;

    // initialize the FORWARD Real class for derivative computation, and assign the value
    Real* y = new Real[(nx + 2) * (ny + 2)];
    Real* xW = new Real[(nx + 2) * (ny + 2)];
    Real* xX = new Real[(nx + 2) * (ny + 2)];
    
    for(size_t j = 0; j < ny + 2; j++){
        for(size_t i = 0; i < nx + 2; i++){
            xW[i + j * (nx + 2)] = T[i][j];
            xX[i + j * (nx + 2)] = S[i][j];
        }
    }

    // compute the full Jacobian
    codi::Jacobian<double> jacobian(nx*ny,nx*ny);

    // compute the full Jacobian.
    for (size_t j = 1; j < ny + 1; j++){
        for (size_t i = 1; i < nx + 1; i++){
            xW[i + j * (nx + 2)].gradient() = 1.0;   // compute the derivatives with respect to state variables

            CDResidual(xW, xX, y, nx, ny, dx, dy, vol, T, U, nu, S);

            for (size_t l = 1; l < ny + 1; l++){
                for (size_t m = 1; m < nx + 1; m++){
                    // compute the element of the Jacobian. only internal points are included
                    jacobian(m + (l-1) * nx - 1, i + (j-1) * nx - 1) 
                    = y[m + l * (nx + 2)].getGradient();

                    // std::cout<<"("<<m + (l - 1) * nx - 1<<", "
                    // << i + (j - 1) * nx - 1<<")" <<": "
                    // <<y[m + l * (nx + 2)].getGradient()<<"\n";
                }
            }

            xW[i + j * (nx + 2)].gradient() = 0.0;
        }
    }

    delete [] y;
    delete [] xW;
    delete [] xX;

    cout<<"dRdW-brute-force computation completed!\n";

    return jacobian;
}

// forward AD using graph coloring, the number of evaluation = nx + 2
codi::Jacobian<double> calcdRdWColored(volScalarField& T, volScalarField& nu, volScalarField& S,
volVectorField& U, mesh& Mesh)
{
    // get the dimension of the mesh
    int nx = Mesh.getNx(), ny = Mesh.getNy(), t = 0;
    double dx = Mesh.dx, dy = Mesh.dy, vol = Mesh.vol;

    // initialize the indexes and coefficients used in residual evaluation
    int pIndex, nIndex, sIndex, eIndex, wIndex;
    int ppIndex, nnIndex, ssIndex, eeIndex, wwIndex;
    double Fs, Fn, Fw, Fe, Ds, Dn, De, Dw, as, an, ae, aw, ap;

    // initialize the FORWARD Real class for derivative computation, and assign the value
    Real* y = new Real[(nx + 2) * (ny + 2)];
    Real* xW = new Real[(nx + 2) * (ny + 2)];
    Real* xX = new Real[(nx + 2) * (ny + 2)];
    
    for(size_t j = 0; j < ny + 2; j++){
        for(size_t i = 0; i < nx + 2; i++){
            xW[i + j * (nx + 2)] = T[i][j];
            xX[i + j * (nx + 2)] = S[i][j];
        }
    }

    // initialize the full Jacobian
    codi::Jacobian<double> jacobian(nx*ny,nx*ny);

    // compute the full Jacobian. Only nx + 2 colors are needed
    
    for (size_t i = 0; i < nx + 2; i++){
        // reset t to 0
        t = 0;

        // activate all the columns whose index equals to i when % by nx + 2
        while (i + t*(nx + 2) < nx*ny)
        {
            ppIndex = jacIndexToArrayIndex(i + t*(nx + 2), nx, ny);
            xW[ppIndex].gradient() = 1.0; t++;    // compute the derivatives with respect to state variables
        }

        // reset t to 0
        t = 0;

        CDResidual(xW, xX, y, nx, ny, dx, dy, vol, T, U, nu, S);

        // assign the derivatives
        while (i + t*(nx + 2) < nx*ny)
        {
            ppIndex = jacIndexToArrayIndex(i + t*(nx + 2), nx, ny);
            // diagnoal element
            jacobian(i + t*(nx + 2), i + t*(nx + 2)) = y[ppIndex].getGradient();

            // off diagnoal elements
            nIndex = i + t*(nx + 2) + nx;
            sIndex = i + t*(nx + 2) - nx;
            eIndex = i + t*(nx + 2) + 1;
            wIndex = i + t*(nx + 2) - 1;

            // convert to x,y array index
            nnIndex = jacIndexToArrayIndex(nIndex, nx, ny);
            ssIndex = jacIndexToArrayIndex(sIndex, nx, ny);
            eeIndex = jacIndexToArrayIndex(eIndex, nx, ny);
            wwIndex = jacIndexToArrayIndex(wIndex, nx, ny);

            if (nIndex < nx * ny && nIndex >= 0) {jacobian(nIndex, i + t*(nx + 2)) = y[nnIndex].getGradient();}
            if (sIndex < nx * ny && sIndex >= 0) {jacobian(sIndex, i + t*(nx + 2)) = y[ssIndex].getGradient();}
            if (eIndex < nx * ny && eIndex >= 0) {jacobian(eIndex, i + t*(nx + 2)) = y[eeIndex].getGradient();}
            if (wIndex < nx * ny && wIndex >= 0) {jacobian(wIndex, i + t*(nx + 2)) = y[wwIndex].getGradient();}

            t++;
        }

        // reset t to 0
        t = 0;

        // deactivate all the columns whose index equals to i when % by 3
        while (i + t*(nx + 2) < nx*ny)
        {
            ppIndex = jacIndexToArrayIndex(i + t*(nx + 2), nx, ny);
            xW[ppIndex].gradient() = 0.0; t++;
        }  

        // reset t to 0
        t = 0;     
    }

    delete [] y;
    delete [] xW;
    delete [] xX;

    cout<<"dRdW-colored computation completed!\n";

    return jacobian;
}

// forward AD using graph coloring. the number of evaluation = 1 (the residual of cellI depends 
// solely on the design variable of cellI)
codi::Jacobian<double> calcdRdXColored(volScalarField& T, volScalarField& nu, volScalarField& S,
volVectorField& U, mesh& Mesh)
{
    // get the dimension of the mesh
    int nx = Mesh.getNx(), ny = Mesh.getNy(), t = 0;
    double dx = Mesh.dx, dy = Mesh.dy, vol = Mesh.vol;

    // initialize the indexes and coefficients used in residual evaluation
    int pIndex, nIndex, sIndex, eIndex, wIndex;
    int ppIndex, nnIndex, ssIndex, eeIndex, wwIndex;
    double Fs, Fn, Fw, Fe, Ds, Dn, De, Dw, as, an, ae, aw, ap;

    // initialize the FORWARD Real class for derivative computation, and assign the value
    Real* y = new Real[(nx + 2) * (ny + 2)];
    Real* xW = new Real[(nx + 2) * (ny + 2)];
    Real* xX = new Real[(nx + 2) * (ny + 2)];
    
    for(size_t j = 0; j < ny + 2; j++){
        for(size_t i = 0; i < nx + 2; i++){
            xW[i + j * (nx + 2)] = T[i][j];
            xX[i + j * (nx + 2)] = S[i][j];  // here the heat source is the design variable
        }
    }

    // initialize the full Jacobian
    codi::Jacobian<double> jacobian(nx*ny,nx*ny);

    // use only one evaluation to compute the full Jacobian
    // set the seeds
    for (size_t i = 0; i < nx * ny; i++){
        xX[jacIndexToArrayIndex(i, nx, ny)].gradient() = 1.0;
    }

    // evaluate the residual
    CDResidual(xW, xX, y, nx, ny, dx, dy, vol, T, U, nu, S);

    // assign the derivatives
    for (size_t i = 0; i < nx * ny; i++){
        jacobian(i, i) = y[jacIndexToArrayIndex(i,nx,ny)].getGradient();
    }

    // reset the seeds
    for (size_t i = 0; i < nx * ny; i++){
        xX[jacIndexToArrayIndex(i, nx, ny)].gradient() = 0.0;
    }

    // deallocate the Real array
    delete [] y;
    delete [] xW;
    delete [] xX;

    cout<<"dRdX-colored computation completed!\n";

    return jacobian;
}

// cacluate the residual of the primal solver
void CDResidual(Real* xW, Real* xX, Real* y, size_t nx, size_t ny, double dx, double dy, double vol, 
volScalarField& T, volVectorField& U, volScalarField& nu, volScalarField& S){

    // the index of N, S, E, W and P points in the 1D x array and 1D y array
    size_t nIndex = 0, sIndex = 0, eIndex = 0, wIndex = 0, pIndex = 0;
    double Fs, Fn, Fw, Fe, Ds, Dn, De, Dw, as, an, aw, ae, ap;

    for(size_t j = 1; j <= ny; j++){
        for(size_t i = 1; i <= nx; i++){
            // get the 1D index of N, S, W ,E and P points
            pIndex = i + j * (nx + 2);
            nIndex = T.getNeighbor(i, j, "north");
            sIndex = T.getNeighbor(i, j, "south");
            wIndex = T.getNeighbor(i, j, "west");
            eIndex = T.getNeighbor(i, j, "east");

            // compute the convection-diffusion coefficients (Scaled)
            Fs = (U[1][i][j-1] + U[1][i][j]) / 2.0 * dx;
            Fn = (U[1][i][j+1] + U[1][i][j]) / 2.0 * dx;
            Fe = (U[0][i+1][j] + U[0][i][j]) / 2.0 * dy;
            Fw = (U[0][i-1][j] + U[0][i][j]) / 2.0 * dy;
            Ds = (nu[i][j] + nu[i][j-1]) / 2.0 / dy * dx;
            Dn = (nu[i][j] + nu[i][j+1]) / 2.0 / dy * dx;
            De = (nu[i][j] + nu[i+1][j]) / 2.0 / dx * dy;
            Dw = (nu[i][j] + nu[i-1][j]) / 2.0 / dx * dy;
            as = std::max(std::max(Fs, Ds + Fs / 2.0), 0.0);
            an = std::max(std::max(-Fn, Dn - Fn / 2.0), 0.0);
            ae = std::max(std::max(-Fe, De - Fe / 2.0), 0.0);
            aw = std::max(std::max(Fw, Dw + Fw / 2.0), 0.0);
            ap = Fe - Fw + Fn - Fs + as + an + ae + aw;

            // compute the residual
            y[pIndex] = vol * xX[pIndex] + as * xW[sIndex] + an * xW[nIndex] + ae * xW[eIndex] + aw * xW[wIndex] - ap * xW[pIndex];
    }
  }
}

// convert the Jacobian's index into the matching x,y array index
size_t jacIndexToArrayIndex(size_t jacIndex, size_t nx, size_t ny){
    size_t ii, tt;
    ii = jacIndex % nx;
    tt = (jacIndex - ii) / nx;
    ii++;
    tt++;
    return ii + tt*(nx + 2);
}