#if HAVE_CONFIG_H
#   include <config.h>
#endif

#include <stdint.h>

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using std::ostream;
using std::map;
using std::string;
using std::vector;

#include "Array.H"
#include "Common.H"
#include "Dataset.H"
#include "Dimension.H"
#include "Grid.H"
#include "LatLonBox.H"
#include "MaskMap.H"
#include "NotImplementedException.H"
#include "Print.H"
#include "Slice.H"
#include "Variable.H"


/**
 * Default constructor.
 */
MaskMap::MaskMap()
    :   masks()
    ,   sizes()
    ,   cleared()
{
}


/**
 * Create or seed Masks based on the Dimensions found in the given Dataset.
 *
 * @param[in] dataset the Dataset from which to get Dimensions
 * @param[in] seed whether to create (false) or seed (true) masks
 */
MaskMap::MaskMap(Dataset *dataset, bool seed)
    :   masks()
    ,   sizes()
    ,   cleared()
{
    if (seed) {
        seed_masks(dataset->get_dims());
    } else {
        create_masks(dataset->get_dims());
    }
}


/**
 * Destuctor.
 *
 * Deletes all Masks.
 */
MaskMap::~MaskMap()
{
    for (masks_t::iterator it=masks.begin(); it!= masks.end(); ++it) {
        delete it->second;
    }
}


/**
 * Create a mask based on the given Dimension.
 *
 * If a mask does not yet exist for a Dimension, it is initialized to 1.
 * Otherwise, if a mask already exists, it is left alone.
 *
 * @param[in] dim the Dimension on which to base the mask
 */
void MaskMap::create_mask(const Dimension *dim)
{
    assert(get_mask(dim) != NULL);
}


/**
 * Create a mask based on the given name and size.
 *
 * If a mask does not yet exist for a Dimension, it is initialized to 1.
 * Otherwise, if a mask already exists, it is left alone.
 *
 * @param[in] dim the Dimension on which to base the mask
 */
void MaskMap::create_mask(const string &name, int64_t size)
{
    seed_mask(name, size);
    assert(get_mask(name) != NULL);
}


/**
 * Create a default mask for each Dimension in the given Dataset.
 *
 * If a mask does not yet exist for a Dimension, it is initialized to 1.
 * Otherwise, if a mask already exists, it is left alone.
 *
 * @param[in] dataset the Dataset from which to get Dimensions
 */
void MaskMap::create_masks(const Dataset *dataset)
{
    create_masks(dataset->get_dims());
}


/**
 * Create a default mask for each given Dimension.
 *
 * If a mask does not yet exist for a Dimension, it is initialized to 1.
 * Otherwise, if a mask already exists, it is left alone.
 *
 * @param[in] dims the Dimension instances for which mask instances should be
 *                 created
 */
void MaskMap::create_masks(const vector<Dimension*> &dims)
{
    vector<Dimension*>::const_iterator dim_it;
    vector<Dimension*>::const_iterator dim_end;


    for (dim_it=dims.begin(),dim_end=dims.end(); dim_it!=dim_end; ++dim_it) {
        create_mask(*dim_it);
    }
}


/**
 * Store information required to create a mask at a later time.
 *
 * @param[in] dim gather mask info from the given Dimension
 */
void MaskMap::seed_mask(const Dimension *dim)
{
    if (sizes.find(dim->get_name()) == sizes.end()) {
        sizes[dim->get_name()] = dim->get_size();
    }
}


/**
 * Store information required to create a mask at a later time.
 *
 * @param[in] name the name of the mask
 * @param[in] size the size of the mask
 */
void MaskMap::seed_mask(const string &name, int64_t size)
{
    if (sizes.find(name) == sizes.end()) {
        sizes[name] = size;
    }
}


/**
 * Store information required to create mask instances at a later time.
 *
 * @param[in] dataset seed mask isntances for each Dimension in the dataset
 */
void MaskMap::seed_masks(const Dataset *dataset)
{
    seed_masks(dataset->get_dims());
}


/**
 * Store information required to create mask instances at a later time.
 *
 * @param[in] dims seed mask isntances for each Dimension
 */
void MaskMap::seed_masks(const vector<Dimension*> &dims)
{
    vector<Dimension*>::const_iterator dim_it;
    vector<Dimension*>::const_iterator dim_end;

    for (dim_it=dims.begin(),dim_end=dims.end(); dim_it!=dim_end; ++dim_it) {
        seed_mask(*dim_it);
    }
}


/**
 * Calls Array::modify(IndexHyperslab) for the given hyperslab if associated
 * mask is found.
 *
 * If an associated mask is not found for the indicated Dimension, it is
 * reported to stderr but otherwise ignored.
 *
 * @param[in] hyperslab the IndexHyperslab to apply to associatd Masks
 */
void MaskMap::modify(const IndexHyperslab &hyperslab)
{
    string hyperslab_name = hyperslab.get_name();
    Array *mask = get_mask(hyperslab_name);

    if (mask == NULL) {
        pagoda::print_zero("Sliced dimension '%s' does not exist\n",
                           hyperslab_name.c_str());
    }
    else {
        // clear the mask the first time only
        if (cleared.count(hyperslab_name) == 0) {
            cleared.insert(hyperslab_name);
            mask->clear();
        }
        // modify the mask based on the current Slice
        mask->modify(hyperslab);
    }
}


/**
 * Calls Array::modify(IndexHyperslab) for each given hyperslab if associated
 * mask is found.
 *
 * If an associated mask is not found for the indicated Dimension, it is
 * reported to stderr but otherwise ignored.
 *
 * @param[in] hyperslabs the DimSlices to apply to associatd Masks
 */
void MaskMap::modify(const vector<IndexHyperslab> &hyperslabs)
{
    vector<IndexHyperslab>::const_iterator hyperslab_it;

    // we're iterating over the command-line specified hyperslabs to create masks
    for (hyperslab_it=hyperslabs.begin(); hyperslab_it!=hyperslabs.end(); ++hyperslab_it) {
        modify(*hyperslab_it);
    }
}


/**
 * Calls mask::modify(CoordHyperslab) for the given hyperslab if associated
 * mask is found.
 *
 * If an associated mask is not found for the indicated Dimension, it is
 * reported to stderr but otherwise ignored.
 *
 * @param[in] hyperslab the CoordHyperslab to apply to associatd Masks
 *
 * @todo TODO handle auxiliary coordinate hyperslabs?
 */
void MaskMap::modify(const CoordHyperslab &hyperslab, Grid *grid)
{
    string hyperslab_name = hyperslab.get_name();
    const Dataset *dataset = grid->get_dataset();
    Variable *variable = dataset->get_var(hyperslab_name);

    if (variable == NULL) {
        pagoda::print_zero("coordinate variable '%s' does not exist\n",
                hyperslab_name.c_str());
    } else {
        Array *data = variable->read();
        Array *mask = get_mask(hyperslab_name);

        if (mask == NULL) {
            pagoda::print_zero("Sliced dimension '%s' does not exist\n",
                    hyperslab_name.c_str());
        }

        ASSERT(variable->get_ndim() == 1);
        ASSERT(variable->get_dims()[0]->get_name() == hyperslab_name);

        // clear the mask the first time only
        if (cleared.count(hyperslab_name) == 0) {
            cleared.insert(hyperslab_name);
            mask->clear();
        }

        // modify the mask based on the current Slice
        ASSERT(hyperslab.has_min() || hyperslab.has_max());
        if (hyperslab.has_min() && hyperslab.has_max()) {
            mask->modify(
                    hyperslab.get_min(), hyperslab.get_max(), data);
        } else if (hyperslab.has_min()) {
            mask->modify_gt(hyperslab.get_min(), data);
        } else if (hyperslab.has_max()) {
            mask->modify_lt(hyperslab.get_max(), data);
        }

        delete data;
    }
}


/**
 * Calls Array::modify(CoordHyperslab) for each given hyperslab if associated
 * mask is found.
 *
 * If an associated mask is not found for the indicated Dimension, it is
 * reported to stderr but otherwise ignored.
 *
 * @param[in] hyperslabs the DimSlices to apply to associatd Masks
 */
void MaskMap::modify(const vector<CoordHyperslab> &hyperslabs, Grid *grid)
{
    vector<CoordHyperslab>::const_iterator hyperslab_it;

    // we're iterating over the command-line specified hyperslabs to create
    // masks
    for (hyperslab_it=hyperslabs.begin(); hyperslab_it!=hyperslabs.end(); ++hyperslab_it) {
        modify(*hyperslab_it, grid);
    }
}


void MaskMap::modify(const LatLonBox &box, Grid *grid)
{
    if (grid->get_type() == GridType::GEODESIC) {
        Variable *cell_lat = grid->get_cell_lat();
        Variable *cell_lon = grid->get_cell_lon();
        Variable *corner_lat = grid->get_corner_lat();
        Variable *corner_lon = grid->get_corner_lon();
        Variable *edge_lat = grid->get_edge_lat();
        Variable *edge_lon = grid->get_edge_lon();
        Variable *cell_corners = grid->get_cell_corners();
        Variable *cell_edges = grid->get_cell_edges();
        Dimension *cell_dim = NULL;
        Dimension *corner_dim = NULL;
        Dimension *edge_dim = NULL;

        if (cell_lat) {
            cell_dim = cell_lat->get_dims().at(0);
        }
        else if (cell_lon) {
            cell_dim = cell_lon->get_dims().at(0);
        }
        if (corner_lat) {
            corner_dim = corner_lat->get_dims().at(0);
        }
        else if (corner_lon) {
            corner_dim = corner_lon->get_dims().at(0);
        }
        if (edge_lat) {
            edge_dim = edge_lat->get_dims().at(0);
        }
        else if (edge_lon) {
            edge_dim = edge_lon->get_dims().at(0);
        }

        if (cell_lat && cell_lon && cell_dim) {
            if (grid->is_radians()) {
                modify(box*RAD_PER_DEG, cell_lat, cell_lon, cell_dim);
            }
            else {
                modify(box, cell_lat, cell_lon, cell_dim);
            }
            if (corner_dim && cell_corners) {
                modify(cell_dim, corner_dim, cell_corners);
            }
            else {
                if (!corner_lat) {
                    pagoda::print_zero("grid corner lat missing\n");
                }
                if (!corner_lon) {
                    pagoda::print_zero("grid corner lon missing\n");
                }
                if (!corner_dim) {
                    pagoda::print_zero("grid corner dim missing\n");
                }
            }
            if (edge_dim && cell_edges) {
                modify(cell_dim, edge_dim, cell_edges);
            }
            else {
                if (!edge_lat) {
                    pagoda::print_zero("grid edge lat missing\n");
                }
                if (!edge_lon) {
                    pagoda::print_zero("grid edge lon missing\n");
                }
                if (!edge_dim) {
                    pagoda::print_zero("grid edge dim missing\n");
                }
            }
        }
        else {
            if (!cell_lat) {
                pagoda::print_zero("grid cell lat missing\n");
            }
            if (!cell_lon) {
                pagoda::print_zero("grid cell lon missing\n");
            }
            if (!cell_dim) {
                pagoda::print_zero("grid cell dim missing\n");
            }
        }
    } else {
        /* for any other grid type, look for generic lat/lon variables */
        Variable *lat = grid->get_lat();
        Variable *lon = grid->get_lon();
        Dimension *lat_dim = grid->get_lat_dim();
        Dimension *lon_dim = grid->get_lon_dim();

        if (lat && lon && lat_dim && lon_dim) {
            if (grid->is_radians()) {
                modify(box*RAD_PER_DEG, lat, lon, lat_dim, lon_dim);
            } else {
                modify(box, lat, lon, lat_dim, lon_dim);
            }
        } else {
            if (!lat) {
                pagoda::println_zero("generic grid lat missing");
            }
            if (!lon) {
                pagoda::println_zero("generic grid lon missing");
            }
            if (!lat_dim) {
                pagoda::println_zero("generic grid lat dim missing");
            }
            if (!lon_dim) {
                pagoda::println_zero("generic grid lon dim missing");
            }
        }
    }
}


void MaskMap::modify(const vector<LatLonBox> &boxes, Grid *grid)
{
    vector<LatLonBox>::const_iterator it;
    vector<LatLonBox>::const_iterator end;

    for (it=boxes.begin(),end=boxes.end(); it!=end; ++it) {
        modify(*it, grid);
    }
}


/**
 * Modify the mask by assigning the given values directly.
 */
void MaskMap::modify(const string &name, Array *values)
{
    Array *mask = get_mask(name);
    mask->copy(values);
}


/**
 * Modify the shared mask of the given lat/lon variables.
 *
 * For instance the lat/lon variables for corners, edges, and cells would have
 * their masks modified based on each pair of variables.
 *
 * @param[in] box the lat/lon specification
 * @param[in] lat the latitude coordinate data
 * @param[in] lon the longitude coordinate data
 * @param[in] dim the dimension we are masking
 */
void MaskMap::modify(const LatLonBox &box,
                     const Variable *lat, const Variable *lon, Dimension *dim)
{
    Array *mask = get_mask(dim);
    Array *lat_array;
    Array *lon_array;


    lat->get_dataset()->push_masks(NULL);
    lat_array = lat->read();
    lon_array = lon->read();
    lat->get_dataset()->pop_masks();

    // clear the mask the first time only
    if (cleared.count(dim->get_name()) == 0) {
        cleared.insert(dim->get_name());
        mask->clear();
    }
    mask->modify(box, lat_array, lon_array);
    delete lat_array;
    delete lon_array;
}


/**
 * Modifiy the masks of the given lat/lon variables.
 *
 * This is useful when the lat/lon dimensions are unique.  In other words,
 * assumes lat_dim and lon_dim are two different dimensions.
 *
 * @param[in] box the lat/lon specification
 * @param[in] lat the latitude coordinate data
 * @param[in] lon the longitude coordinate data
 * @param[in] lat_dim the latitude dimension we are masking
 * @param[in] lon_dim the longitude dimension we are masking
 */
void MaskMap::modify(
    const LatLonBox &box, const Variable *lat, const Variable *lon,
    Dimension *lat_dim, Dimension *lon_dim)
{
    Array *lat_mask = get_mask(lat_dim);
    Array *lon_mask = get_mask(lon_dim);
    Array *lat_array;
    Array *lon_array;

    lat->get_dataset()->push_masks(NULL);
    lat_array = lat->read();
    lon_array = lon->read();
    lat->get_dataset()->pop_masks();

    // clear the mask the first time only
    if (cleared.count(lat_dim->get_name()) == 0) {
        cleared.insert(lat_dim->get_name());
        lat_mask->clear();
    }
    if (cleared.count(lon_dim->get_name()) == 0) {
        cleared.insert(lon_dim->get_name());
        lon_mask->clear();
    }
    lat_mask->modify(box.s, box.n, lat_array);
    lon_mask->modify(box.w, box.e, lon_array);
    delete lat_array;
    delete lon_array;
}


/**
 * Modify the mask of a connected Dimension.
 *
 * This is a very specific function designed to help preserve whole cells
 * during a subset.  Let's assume we have cells/faces, edges, and
 * vertices/corners within a dataset and we also have variables indicating
 * how they are connected.  For a variable from cells to edges might look like
 * cell_edges(cells, celledges) where celledges indicates how many edges the
 * particular cell has.  We want to mask the related edges dimension (don't
 * confuse with the small celledges Dimension).  That's where this function
 * is useful.  Read it as: modify the mask for the "to_mask" Dimension based
 * on the given "masked" Dimension and the given "topology" relation.
 *
 * @param[in] dim_masked the Dimension which already has an associated mask
 * @param[in] dim_to_mask the Dimension which should be masked based on masked
 * @param[in] topology relation between the two given Dimensions
 */
void MaskMap::modify(
    Dimension *dim_masked, Dimension *dim_to_mask, const Variable *topology)
{
    Array *mask = get_mask(dim_masked);
    Array *to_mask = get_mask(dim_to_mask);
    int64_t connections = topology->get_shape().at(1);
    int64_t mask_local_size = mask->get_local_size();
    vector<int64_t> masked_lo(1,0);
    vector<int64_t> masked_hi(1,0);
    vector<int64_t> topology_lo(2,0);
    vector<int64_t> topology_hi(2,0);
    int *mask_data;
    Array *topology_array;
    int *topology_data;
    vector<int> topology_buffer;
    vector<int64_t> topology_subscripts;

    // clear the mask the first time only
    if (cleared.count(dim_to_mask->get_name()) == 0) {
        cleared.insert(dim_to_mask->get_name());
        to_mask->clear();
    }

    // regardless of whether the mask owns the data, the topology variable
    // must be read in, and in its entirety (no subsetting)
    dim_masked->get_dataset()->push_masks(NULL);
    topology_array = topology->read();
    dim_masked->get_dataset()->pop_masks();

    if (!mask->owns_data()) {
        // bail if this process doesn't own any of the mask
        delete topology_array;
        return;
    }

    mask->get_distribution(masked_lo, masked_hi);
    topology_lo[0] = masked_lo.at(0);
    topology_lo[1] = 0;
    topology_hi[0] = masked_lo.at(0) + mask_local_size-1;
    topology_hi[1] = connections-1;

    // get portion of the topology local to the mask
    topology_data = static_cast<int*>(
            topology_array->get(topology_lo, topology_hi));

    // iterate over the topology indices and place into set
    mask_data = static_cast<int*>(mask->access());
    for (int64_t idx=0; idx<mask_local_size; ++idx) {
        if (mask_data[idx] != 0) {
            int64_t local_offset = idx * connections;
            for (int64_t connection=0; connection<connections; ++connection) {
                topology_subscripts.push_back(
                    topology_data[local_offset+connection]);
            }
        }
    }
    mask->release();

    topology_buffer.assign(topology_subscripts.size(), 1);
    to_mask->scatter(&topology_buffer[0], topology_subscripts);

    // clean up
    delete topology_array;
    delete [] topology_data;
}


void MaskMap::clear_mask(const string &name)
{
    Array *mask = get_mask(name);

    if (mask == NULL) {
        pagoda::print_zero("Sliced dimension '%s' does not exist\n",
                           name.c_str());
    }
    else {
        cleared.insert(name);
        mask->clear();
    }
}


bool MaskMap::has_mask(const string &name) const
{
    return masks.find(name) != masks.end();
}


Array* MaskMap::get_mask(const string &name)
{
    masks_t::iterator masks_it = masks.find(name);
    if (masks_it == masks.end()) {
        sizes_t::iterator sizes_it = sizes.find(name);
        if (sizes_it == sizes.end()) {
            ERR("cannot create mask; '" + name + "' not found");
        } else {
            return masks.insert(
                    make_pair(name, Array::mask_create(name,sizes_it->second)))
                .first->second;
        }
    }
    else {
        return masks_it->second;
    }
}


Array* MaskMap::operator [](const string &name)
{
    return get_mask(name);
}


void MaskMap::clear_mask(const Dimension *dim)
{
    seed_mask(dim);
    clear_mask(dim->get_name());
}


bool MaskMap::has_mask(const Dimension *dim) const
{
    return has_mask(dim->get_name());
}


Array* MaskMap::get_mask(const Dimension *dim)
{
    seed_mask(dim);
    return get_mask(dim->get_name());
}


Array* MaskMap::operator [](const Dimension *dim)
{
    seed_mask(dim);
    return get_mask(dim);
}


ostream& MaskMap::print(ostream &os) const
{
    os << "MaskMap " << this;
    return os;
}


ostream& operator << (ostream &os, const MaskMap &maskmap)
{
    maskmap.print(os);
    return os;
}


ostream& operator << (ostream &os, const MaskMap *maskmap)
{
    maskmap->print(os);
    return os;
}
