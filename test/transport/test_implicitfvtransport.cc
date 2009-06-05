// $Id$

#include "config.h"
#include <iostream>
#include <iomanip>
#include <dune/grid/sgrid.hh> // load sgrid definition
#include <dune/grid/io/file/vtk/vtkwriter.hh>
#include <dune/istl/io.hh>
#include "dumux/material/phaseproperties/phaseproperties2p.hh"
#include "dumux/transport/fv/implicitfvtransport.hh"
#include "dumux/transport/fv/computeupwind.hh"
#include "dumux/transport/fv/capillarydiffusion.hh"
#include "dumux/transport/problems/simpleproblem.hh"
#include "dumux/transport/problems/simplenonlinearproblem.hh"
#include "dumux/timedisc/timeloop.hh"
#include "dumux/fractionalflow/variableclass2p.hh"

int main(int argc, char** argv)
{
    try{
        // define the problem dimensions
        const int dim=2;

        // time loop parameters
        const double tStart = 0;
        const double tEnd = 1.5e9;
        double dt = 2.5e8;
	double firstDt = dt;
	double maxDt = dt;
        int modulo = 1;

        // create a grid object
        typedef double NumberType;
        typedef Dune::SGrid<dim,dim> GridType;
        typedef GridType::LevelGridView GridView;
        typedef Dune::FieldVector<GridType::ctype,dim> FieldVector;
        Dune::FieldVector<int,dim> N(1); N[0] = 200;
        FieldVector L(0);
        FieldVector H(300); H[0] = 600;
        GridType grid(N,L,H);

        grid.globalRefine(0);

        GridView gridView(grid.levelView(0));

        Dune::Uniform mat(0.2);
        //Dune::HomogeneousLinearSoil<GridType, NumberType> soil;
        Dune::HomogeneousNonlinearSoil<GridType, NumberType> soil;
        Dune::TwoPhaseRelations<GridType, NumberType> materialLaw(soil, mat, mat);

        typedef Dune::VariableClass<GridView, NumberType> VC;

        double initsat=0;
        Dune::FieldVector<double,dim> vel(0);
        vel[0] = 1.0/6.0*1e-6;

        VC variables(gridView,initsat,vel);

        //Dune::SimpleProblem<GridView, NumberType, VC> ProblemType;
        typedef Dune::SimpleNonlinearProblem<GridView, NumberType, VC> ProblemType;
	ProblemType problem(variables, mat, mat , soil, materialLaw,L,H);

        //Dune::DiffusivePart<GridView, NumberType> diffusivePart;
        Dune::CapillaryDiffusion<GridView, NumberType, VC, ProblemType> capillaryDiffusion(problem, soil, false);
        Dune::ComputeUpwind<GridView, NumberType, VC> computeNumFlux(problem);

        typedef Dune::ImplicitFVTransport<GridView, NumberType, VC> Transport;

        //Transport transport(gridView, problem, dt, diffusivePart, computeNumFlux);
        Transport transport(gridView, problem, capillaryDiffusion, computeNumFlux);

	Dune::RungeKuttaStep<GridType, Transport> timeStep(1);
	//Dune::ImplicitEulerStep<GridType, Transport> timeStep;
        Dune::TimeLoop<GridType, Transport > timeloop(tStart, tEnd, dt, "implicitfvtransport", modulo, maxDt, firstDt, timeStep);

        timeloop.execute(transport);

        printvector(std::cout, variables.saturation(), "saturation", "row", 200, 1);

        return 0;
    }
    catch (Dune::Exception &e){
        std::cerr << "Dune reported error: " << e << std::endl;
    }
    catch (...){
        std::cerr << "Unknown exception thrown!" << std::endl;
    }
}
