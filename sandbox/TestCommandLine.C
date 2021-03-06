/** Perform various block-sized pnetcdf reads. */
#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <cstdlib>
#include <iostream>

#include "Bootstrap.H"
#include "Debug.H"
#include "SubsetterCommands.H"

using std::cerr;
using std::cout;
using std::endl;


int main(int argc, char **argv)
{
    SubsetterCommands cmd;

    pagoda::initialize(&argc, &argv);
    cmd.parse(argc,argv);

    if (0 == pagoda::me) {
        cout << "boxes=" << pagoda::vec_to_string(cmd.get_boxes(),";") << endl;
        cout << "index_hyperslabs=" << pagoda::vec_to_string(cmd.get_index_hyperslabs(),";") << endl;
        cout << "coord_hyperslabs=" << pagoda::vec_to_string(cmd.get_coord_hyperslabs(),";") << endl;
        cout << "inputs=" << pagoda::vec_to_string(cmd.get_input_filenames(),";") << endl;
        cout << "output=" << cmd.get_output_filename() << endl;
        cout << "variables=" << pagoda::vec_to_string(cmd.get_variable_names(),";") << endl;
        cout << "exclude=" << cmd.is_excluding_variables() << endl;
        cout << "join=" << cmd.get_join_name() << endl;
    }

    pagoda::finalize();

    return EXIT_SUCCESS;
}
