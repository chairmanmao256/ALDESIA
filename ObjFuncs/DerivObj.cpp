#include <iostream>
#include <string>
#include <codi.hpp>
#include "../field/volScalarField.h"
#include "../field/volVectorField.h"
#include "../field/mesh.h"
#include "DerivObj.h"

// since we want to get the gradient of the objective function, we use reverse mode AD
using Tape = typename RealR::Tape;
using namespace std;

// the Jacobian of objective function with respect to state variable
codi::Jacobian<double> calcdFdW(volScalarField& T, volVectorField& U, volScalarField& nu, 
volScalarField& S, mesh& Mesh, objFuncs& obj)
{
    // get the dimension info
    int nx = Mesh.getNx(), ny = Mesh.getNy();

    // initialize the AD variables
    RealR* xW = new RealR[nx * ny];
    RealR* xX = new RealR[nx * ny];
    RealR* y = new RealR[1];
    codi::Jacobian<double> jacobian(1, nx * ny);
    for (size_t j = 1; j <= ny; j++){
        for (size_t i = 1; i <= nx; i++){
            xW[i - 1 + (j - 1) * nx] = T[i][j];
            xX[i - 1 + (j - 1) * nx] = S[i][j];
        }
    }

    // initialize and activate tape recording
    Tape& tape = RealR::getTape();
    tape.setActive();

    // register input
    for (size_t i = 0; i < nx * ny; i++){
        tape.registerInput(xW[i]);
    }

    // evaluate the obj
    obj.evalObjForAD(xW, xX, y);

    // register output
    tape.registerOutput(y[0]);

    // set the tape passive
    tape.setPassive();

    // set the weight of output and evaluate the gradient
    y[0].gradient() = 1.0;
    tape.evaluate();
    for (size_t i = 0; i < nx * ny; i++){
        jacobian(0, i) = xW[i].getGradient();
    }

    // deallocate the space
    delete [] xW;
    delete [] xX;
    delete [] y;

    return jacobian;
}

// the Jacobian of objective function with respect to design variable
codi::Jacobian<double> calcdFdX(volScalarField& T, volVectorField& U, volScalarField& nu, 
volScalarField& S, mesh& Mesh, objFuncs& obj)
{
    // get the dimension info
    int nx = Mesh.getNx(), ny = Mesh.getNy();

    // initialize the AD variables
    RealR* xW = new RealR[nx * ny];
    RealR* xX = new RealR[nx * ny];
    RealR* y = new RealR[1];
    codi::Jacobian<double> jacobian(1, nx * ny);
    for (size_t j = 1; j <= ny; j++){
        for (size_t i = 1; i <= nx; i++){
            xW[i - 1 + (j - 1) * nx] = T[i][j];
            xX[i - 1 + (j - 1) * nx] = S[i][j];
        }
    }

    // initialize and activate tape recording
    Tape& tape = RealR::getTape();
    tape.setActive();

    // register input
    for (size_t i = 0; i < nx * ny; i++){
        tape.registerInput(xX[i]);
    }

    // evaluate the obj
    obj.evalObjForAD(xW, xX, y);

    // register output
    tape.registerOutput(y[0]);

    // set the tape passive
    tape.setPassive();

    // set the weight of output and evaluate the gradient
    y[0].gradient() = 1.0;
    tape.evaluate();
    for (size_t i = 0; i < nx * ny; i++){
        jacobian(0, i) = xX[i].getGradient();
    }

    // deallocate the space
    delete [] xW;
    delete [] xX;
    delete [] y;

    return jacobian;
}