#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "AggregationDimension.H"
#include "AggregationJoinExisting.H"
#include "AggregationVariable.H"
#include "Attribute.H"
#include "Dimension.H"
#include "Error.H"
#include "Util.H"
#include "Variable.H"

using std::ostream;
using std::string;
using std::transform;
using std::vector;


AggregationJoinExisting::AggregationJoinExisting(const string& name)
    :   Aggregation()
    ,   agg_dim_name(name)
    ,   agg_dim(NULL)
    ,   agg_vars()
{
}


AggregationJoinExisting::~AggregationJoinExisting()
{
    map<string,AggregationVariable*>::iterator agg_var_it = agg_vars.begin();
    map<string,AggregationVariable*>::iterator agg_var_end = agg_vars.end();


    for (; agg_var_it!=agg_var_end; ++agg_var_it) {
        AggregationVariable *agg_var = agg_var_it->second;
        delete agg_var;
        agg_var = NULL;
    }

    delete agg_dim;
}


void AggregationJoinExisting::add(Dataset *dataset)
{
    datasets.push_back(dataset);

    /* combine global attributes */
    vector<Attribute*> other_atts = dataset->get_atts();
    vector<Attribute*>::const_iterator other_atts_it = other_atts.begin();
    vector<Attribute*>::const_iterator other_atts_end = other_atts.end();
    for (; other_atts_it!=other_atts_end; ++other_atts_it) {
        Attribute *other_att = *other_atts_it;
        Attribute *att = get_att(other_att->get_name());
        if (! att) {
            atts.push_back(other_att);
        }
    }

    /* combine dimensions */
    vector<Dimension*> other_dims = dataset->get_dims();
    vector<Dimension*>::const_iterator other_dims_it = other_dims.begin();
    vector<Dimension*>::const_iterator other_dims_end = other_dims.end();
    for (; other_dims_it!=other_dims_end; ++other_dims_it) {
        Dimension *other_dim = *other_dims_it;
        Dimension *dim = get_dim(other_dim->get_name());
        if (! dim) {
            if (other_dim->get_name() == agg_dim_name) {
                agg_dim = new AggregationDimension(this, other_dim);
                dims.push_back(agg_dim);
            }
            else {
                dims.push_back(other_dim);
            }
        }
        else if (other_dim->get_name() == agg_dim_name) {
            agg_dim->add(other_dim);
        }
    }

    /* combine variables */
    vector<Variable*> other_vars = dataset->get_vars();
    vector<Variable*>::const_iterator other_vars_it = other_vars.begin();
    vector<Variable*>::const_iterator other_vars_end = other_vars.end();
    for (; other_vars_it!=other_vars_end; ++other_vars_it) {
        Variable *other_var = *other_vars_it;
        Variable *var = get_var(other_var->get_name());
        if (! var) {
            // other_var is not part of the aggregation yet
            if (other_var->get_ndim() > 0
                    && other_var->get_dims()[0]->get_name() == agg_dim_name) {
                // other_var has at least one dimension and its outer
                // dimension name matches the aggregated dimension name
                AggregationVariable *agg_var;
                agg_var = new AggregationVariable(this, agg_dim, other_var);
                agg_vars[other_var->get_name()] = agg_var;
                vars.push_back(agg_var);
            }
            else {
                // either other_var is dimensionless or its outer dimension
                // name did not match the aggregated dimension name
                vars.push_back(other_var);
            }
        }
        else {
            // other_var was found in the aggregation
            if (other_var->get_ndim() > 0
                    && other_var->get_dims()[0]->get_name() == agg_dim_name) {
                // other_var has at least one dimension and its outer
                // dimension name matches the aggregated dimension name
                AggregationVariable *agg_var;
                agg_var = dynamic_cast<AggregationVariable*>(var);
                // assumption here is that the agg_var was created previously
                if (! agg_var) {
                    ERR("dynamic_cast of AggregationVariable failed");
                }
                agg_var->add(other_var);
            }
            else {
                // no-op. other_var was found in the aggregation, but its
                // outer dimension does not match the aggregated dimension.
                // therefore this acts like a union aggregation (we toss out
                // conflicting variables)
            }
        }
    }
}


void AggregationJoinExisting::add(const vector<Dataset*> &dataset)
{
    Aggregation::add(dataset);
}


void AggregationJoinExisting::wait()
{
    Aggregation::wait();

    for (map<string,AggregationVariable*>::iterator it=agg_vars.begin(),
            end=agg_vars.end(); it!=end; ++it) {
        it->second->after_wait();
    }
}


ostream& AggregationJoinExisting::print(ostream &os) const
{
    return os << "AggregationJoinExisting()";
}
