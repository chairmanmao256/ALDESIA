import pyALDESIA

# specify the number of design variables
nDV = 10000
DVList = [0.0] * 10000

# initialize ALDESIA
solver = pyALDESIA.ALDESIA(b'input.ini')

# set the design variable according to the 
# input specification
for i in range(nDV):
    if solver.isInBox(i):
        DVList[i] = 1.0
    solver.setDesignVariable(i,DVList[i])

# solve primal problem
solver.solvePrimal()

# get the objVal
objVal = solver.calcObj(b"averageTempreture")

# write the primal solution
solver.writePrimal()

# solve the Discrete-Adjoint
solver.solveDA(b"averageTempreture")