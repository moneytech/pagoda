/**
 * @file pgcmp.C
 *
 * Compares two files.  Attribute, Dimension and Variable order is allowed to
 * differ.  In other words, this is a comparison between the set of
 * Attributes, Dimensions and Variables in each file.
 *
 * Variables are converted to double and then compared within a given
 * tolerance.
 */
#if HAVE_CONFIG_H
#   include <config.h>
#endif

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

using std::cerr;
using std::endl;
using std::map;
using std::mismatch;
using std::pair;
using std::string;
using std::vector;

#include "Array.H"
#include "Attribute.H"
#include "Bootstrap.H"
#include "Dataset.H"
#include "Debug.H"
#include "Dimension.H"
#include "Values.H"
#include "Variable.H"


template <class T>
static void init_map(const vector<T> &v, map<string,T> &m)
{
    for (size_t i=0; i<v.size(); ++i) {
        m[v[i]->get_name()] = v[i];
    }
}


bool cmp(double x, double y)
{
    return std::abs(x-y)/std::max(1.0,std::abs(x)) <= 1e-12;
}


int main(int argc, char **argv)
{
    Dataset *lhs;
    vector<Attribute*> lhs_atts;
    vector<Dimension*> lhs_dims;
    vector<Variable*>  lhs_vars;
    map<string,Attribute*> lhs_atts_m;
    map<string,Dimension*> lhs_dims_m;
    map<string,Variable*>  lhs_vars_m;

    Dataset *rhs;
    vector<Attribute*> rhs_atts;
    vector<Dimension*> rhs_dims;
    vector<Variable*>  rhs_vars;
    map<string,Attribute*> rhs_atts_m;
    map<string,Dimension*> rhs_dims_m;
    map<string,Variable*>  rhs_vars_m;

    pagoda::initialize(&argc,&argv);

    try {
        if (argc != 3) {
            throw "Usage: pgcmp <filename> <filename>";
        }

        lhs = Dataset::open(argv[1]);
        lhs_atts = lhs->get_atts();
        lhs_dims = lhs->get_dims();
        lhs_vars = lhs->get_vars();

        rhs = Dataset::open(argv[2]);
        rhs_atts = rhs->get_atts();
        rhs_dims = rhs->get_dims();
        rhs_vars = rhs->get_vars();

        // first check set sizes
        //pagoda::println_zero("first check set sizes");
        if (lhs_atts.size() != rhs_atts.size()) {
            // note that the number of global attributes is allowed to differ
            // since NCO might add "nco_openmp_thread_number".
            ostringstream str;
            str << "number of global attributes mismatch: ";
            str << lhs_atts.size() << "!=" << rhs_atts.size();
            pagoda::println_zero("WARNING: " + str.str());
            //throw str.str();
        }
        if (lhs_dims.size() != rhs_dims.size()) {
            ostringstream str;
            str << "number of dimensions mismatch: ";
            str << lhs_dims.size() << "!=" << rhs_dims.size();
            throw str.str();
        }
        if (lhs_vars.size() != rhs_vars.size()) {
            ostringstream str;
            str << "number of variables mismatch: ";
            str << lhs_vars.size() << "!=" << rhs_vars.size();
            throw str.str();
        }

        // the code is a bit simpler if we use maps; initialize them
        //pagoda::println_zero("the code is a bit simpler if we use maps; initialize them");
        init_map(lhs_atts, lhs_atts_m);
        init_map(lhs_dims, lhs_dims_m);
        init_map(lhs_vars, lhs_vars_m);
        init_map(rhs_atts, rhs_atts_m);
        init_map(rhs_dims, rhs_dims_m);
        init_map(rhs_vars, rhs_vars_m);

        // attributes are allowed to differ in number, not content
        for (size_t i=0; i<lhs_atts.size(); ++i) {
            Attribute *lhs_att = lhs_atts[i];
            Attribute *rhs_att;
            string name = lhs_att->get_name();
            vector<double> lhs_values;
            vector<double> rhs_values;
            pair<vector<double>::iterator,vector<double>::iterator> result;

            //pagoda::println_zero("comparing global attribute " + name);

            // always skip history attribute
            if (name == "history") {
                continue;
            }

            if (rhs_atts_m.count(name)) {
                rhs_att = rhs_atts_m[name];
            }
            else {
                ostringstream str;
                str << "global attribute '" << name << "' not found";
                //throw str.str();
                pagoda::println_zero("WARNING: " + str.str());
                continue;
            }

            lhs_att->get_values()->as(lhs_values);
            rhs_att->get_values()->as(rhs_values);
            //pagoda::println_zero(pagoda::vec_to_string(lhs_values));
            //pagoda::println_zero(pagoda::vec_to_string(rhs_values));
            if (lhs_values.size() != rhs_values.size()) {
                ostringstream str;
                str << "global attribute '" << name << "' length mismatch: ";
                str << lhs_values.size() << "!=" << rhs_values.size();
                //throw str.str();
                pagoda::println_zero("WARNING: " + str.str());
                continue;
            }
            result = mismatch(lhs_values.begin(), lhs_values.end(),
                              rhs_values.begin(), cmp);
            if (result.first != lhs_values.end()) {
                ostringstream str;
                str << "global attribute '" << name << "' mismatch:" << endl;
                str << "'" << lhs_att->get_string() << "'" << endl;
                str << "'" << rhs_att->get_string() << "'";
                throw str.str();
            }
        }

        // compare Dimensions
        // we duplicate the work of Dimension::equal(vector) in order to
        // pinpoint the problem and report it to stderr
        for (size_t i=0; i<lhs_dims.size(); ++i) {
            Dimension *lhs_dim = lhs_dims[i];
            Dimension *rhs_dim;
            string name = lhs_dim->get_name();

            if (rhs_dims_m.count(name)) {
                rhs_dim = rhs_dims_m[name];
            }
            else {
                ostringstream str;
                str << "dimension '" << name << "' not found";
                throw str.str();
            }

            if (lhs_dim->get_size() != rhs_dim->get_size()) {
                ostringstream str;
                str << "dimension '" << name << "' size mismatch: ";
                str << lhs_dim->get_size() << "!=" << rhs_dim->get_size();
                throw str.str();
            }
        }

        // compare Variables
        // this is a deep comparison as opposed to Variable::equal().
        for (size_t i=0; i<lhs_vars.size(); ++i) {
            Variable *lhs_var = lhs_vars[i];
            Variable *rhs_var;
            string name = lhs_var->get_name();
            Array *lhs_array;
            Array *rhs_array;

            if (rhs_vars_m.count(name)) {
                rhs_var = rhs_vars_m[name];
            }
            else {
                ostringstream str;
                str << "variable '" << name << "' not found";
                throw str.str();
            }

            if (lhs_var->get_shape() != rhs_var->get_shape()) {
                ostringstream str;
                str << "variable '" << name << "' shape mismatch: ";
                str << pagoda::vec_to_string(lhs_var->get_shape());
                str << "!=";
                str << pagoda::vec_to_string(rhs_var->get_shape());
                throw str.str();
            }

            if (lhs_var->get_type() != rhs_var->get_type()) {
                ostringstream str;
                str << "variable '" << name << "' type mismatch: ";
                str << lhs_var->get_type();
                str << "!=";
                str << rhs_var->get_type();
                throw str.str();
            }

            // read in entire variables
            // TODO optimize for memory by reading one record at a time
            lhs_array = lhs_var->read();
            rhs_array = rhs_var->read();
            if (!lhs_array->same_distribution(rhs_array)) {
                throw "distribution mismatch -- should not happen";
            }

            if (lhs_array->owns_data()) {
                int64_t size = lhs_array->get_local_size();
                DataType type = lhs_array->get_type();
                void *lhs_data;
                void *rhs_data;

                lhs_data = lhs_array->access();
                rhs_data = rhs_array->access();
#define DATATYPE_EXPAND(DT,T) \
                if (DT == type) { \
                    T *lhs = static_cast<T*>(lhs_data); \
                    T *rhs = static_cast<T*>(rhs_data); \
                    for (int64_t j=0; j<size; ++j) { \
                        double lhs_d = static_cast<double>(lhs[j]); \
                        double rhs_d = static_cast<double>(rhs[j]); \
                        if (!cmp(lhs_d,rhs_d)) { \
                            ostringstream str; \
                            str << "variable '" << name; \
                            str << "' value mismatch "; \
                            str << "at local index " << j << ": "; \
                            str << lhs[j] << "!=" << rhs[j]; \
                            lhs_array->release(); \
                            rhs_array->release(); \
                            ERR(str.str()); \
                        } \
                    } \
                } else
#include "DataType.def"
                lhs_array->release();
                rhs_array->release();
            }

            delete lhs_array;
            delete rhs_array;
        }

        delete lhs;
        lhs=NULL;
        delete rhs;
        rhs=NULL;

    }
    catch (string &ex) {
        if (lhs) {
            delete lhs;
            lhs=NULL;
        }
        if (rhs) {
            delete rhs;
            rhs=NULL;
        }
        pagoda::println_zero(stderr, ex);
        pagoda::finalize();
        return EXIT_FAILURE;
    }
    catch (PagodaException &ex) {
        pagoda::abort(ex.what());
    }

    pagoda::finalize();
    return EXIT_SUCCESS;
}