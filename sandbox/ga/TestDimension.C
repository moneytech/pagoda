#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif // HAVE_CONFIG_H

// C includes, std and otherwise
#include <ga.h>
#include <macdecls.h>
#include <mpi.h>
#include <pnetcdf.h>
#include <unistd.h>

// C++ includes, std and otherwise
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// C++ includes
#include "Dimension.H"
#include "Mask.H"
#include "PnetcdfDataset.H"
#include "PnetcdfDimension.H"
#include "Util.H"

using std::cout;
using std::endl;
using std::ostringstream;
using std::string;
using std::vector;


int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    GA_Initialize();

    if (argc < 2) return EXIT_FAILURE;

    for (int argi=0; argi<argc; ++argi) {
        cout << argv[argi] << endl;
    }

    PnetcdfDataset *dataset = new PnetcdfDataset(argv[1]);
    Mask *mask = NULL;

    vector<Dimension*> dims = dataset->get_dims();
    for (size_t dimid=0,ndim=dims.size(); dimid<ndim; ++dimid) {
        Dimension *dim = dims[dimid];
        cout << dim << endl;
        if (dim->get_name() == "cells") {
            mask = Mask::create(dim);
        }
    }

    if (mask) {
        mask->reindex();
        //GA_Print(mask->get_handle_index());
    }

    delete mask;
    delete dataset;

    // Must always call these to exit cleanly.
    GA_Terminate();
    MPI_Finalize();

    return EXIT_SUCCESS;
}

