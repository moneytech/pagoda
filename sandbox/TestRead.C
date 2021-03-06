/** Perform various block-sized pnetcdf reads. */
#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <cstdlib>
#include <iostream>

#include "Array.H"
#include "Bootstrap.H"
#include "Dataset.H"
#include "Debug.H"
#include "Variable.H"

#include "timer.h"

using std::cerr;
using std::cout;
using std::endl;


/**
 * 1) Open a dataset.
 * 2) Attempt to read a variable or all variables into Array(s).
 * 3) Compare the blocking and non-blocking values.
 */
int main(int argc, char **argv)
{
    Dataset *dataset;
    vector<Variable*> variables;
    vector<Array*> arrays;
    vector<Array*> nbarrays;
    utimer_t timer_blocking_local;
    utimer_t timer_blocking_total=0;
    utimer_t timer_nonblocking_local;
    utimer_t timer_nonblocking_total=0;

    pagoda::initialize(&argc, &argv);
#if TEST_PROMOTE_TO_FLOAT
    Variable::promote_to_float = true;
#endif

    if (argc < 2) {
        pagoda::println_zero("Usage: TestRead filename [variablename] ...");
        pagoda::finalize();
        return EXIT_FAILURE;
    }

    dataset = Dataset::open(argv[1]);
    if (2 == argc) {
        vector<Variable*> vars = dataset->get_vars();
        for (size_t i=0; i<vars.size(); ++i) {
            Variable *var = vars[i];
            DataType type = var->get_type();
            //if (!var->has_record())
                //if (var->get_ndim() >= 1)
                    //if (type == DataType::INT || type == DataType::FLOAT || type == DataType::DOUBLE)
                    {
                        variables.push_back(vars[i]);
                    }
        }
    } else {
        for (int i=2; i<argc; ++i) {
            variables.push_back(dataset->get_var(argv[i]));
        }
    }

    for (size_t i=0; i<variables.size(); ++i) {
        Variable *var = variables[i];
        string name = var->get_name();
        pagoda::print_zero("blocking read of " + name + " ... ");
        timer_blocking_local = timer_start();
        arrays.push_back(var->read());
        timer_blocking_total += timer_end(timer_blocking_local);
        pagoda::println_zero("done");
        timer_nonblocking_local = timer_start();
        pagoda::print_zero("non-blocking read of " + name + " ... ");
        timer_nonblocking_total += timer_end(timer_nonblocking_local);
        nbarrays.push_back(var->iread());
        pagoda::println_zero("done");
    }
    pagoda::print_zero("wait... ");
    timer_nonblocking_local = timer_start();
    dataset->wait();
    timer_nonblocking_total += timer_end(timer_nonblocking_local);
    pagoda::println_zero("done");

    // compare the arrays
    // it is assumed they have the same distributions
    for (size_t i=0; i<arrays.size(); ++i) {
        Variable *var = variables[i];
        Array *array = arrays[i];
        Array *nbarray = nbarrays[i];
        string name = var->get_name();
        DataType type = array->get_type();
        bool ok = true;
        pagoda::println_zero("name=" + name + " type=" + type.get_name());
        if (array->owns_data()) {
            int64_t size = array->get_local_size();
#define compare_helper(DT,T) \
            if (type == DT) { \
                T *b  = (T*)array->access(); \
                T *nb = (T*)nbarray->access(); \
                for (int64_t i=0; i<size; ++i) { \
                    ok &= (b[i] == nb[i]); \
                    if (!ok) { \
                        break; \
                    } \
                } \
            } else
            compare_helper(DataType::INT, int)
            compare_helper(DataType::FLOAT, float)
            compare_helper(DataType::DOUBLE, double)
            {
                pagoda::println_zero("unhandled data type");
                ok = false;
            }
#undef compare_helper
            array->release();
            nbarray->release();
        } else {
            if (!(type == DataType::INT
                    || type == DataType::FLOAT
                    || type == DataType::DOUBLE)
                    ) {
                pagoda::println_zero("unhandled data type");
                ok = false;
            }
        }
        if (ok) {
            pagoda::print_sync(name + " ok\n");
        } else {
            pagoda::print_sync(name + " failure\n");
        }
    }

    // print timing info
    pagoda::println_sync("   timer_blocking_total="
            + pagoda::to_string(timer_blocking_total));
    pagoda::println_sync("timer_nonblocking_total="
            + pagoda::to_string(timer_nonblocking_total));
    delete dataset;
    for (size_t i=0; i<arrays.size(); ++i) {
        delete arrays[i];
        delete nbarrays[i];
    }

    pagoda::finalize();

    return EXIT_SUCCESS;
}
