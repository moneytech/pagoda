#ifndef REGULARGRID_H_
#define REGULARGRID_H_

#include "Grid.H"

class Dataset;
class Dimension;
class Variable;


/**
 * The implementation of the Grid interface for regular grids.
 *
 * @todo this is unfinished
 */
class RegularGrid : public Grid
{
    public:
        static vector<Grid*> get_grids(const Dataset *dataset);

        RegularGrid(const Dataset *dataset, const Variable *grid_variable=NULL);
        virtual ~RegularGrid();

        virtual GridType get_type() const;

        virtual Variable* get_cell_lat();
        virtual Variable* get_cell_lon();
        virtual Variable* get_edge_lat();
        virtual Variable* get_edge_lon();
        virtual Variable* get_corner_lat();
        virtual Variable* get_corner_lon();
        virtual bool is_radians();

        virtual Dimension* get_cell_dim();
        virtual Dimension* get_edge_dim();
        virtual Dimension* get_corner_dim();

        virtual Variable* get_cell_cells();
        virtual Variable* get_cell_edges();
        virtual Variable* get_cell_corners();
        virtual Variable* get_edge_cells();
        virtual Variable* get_edge_edges();
        virtual Variable* get_edge_corners();
        virtual Variable* get_corner_cells();
        virtual Variable* get_corner_edges();
        virtual Variable* get_corner_corners();

        virtual const Dataset* get_dataset() const;

        virtual Variable* get_lat();
        virtual Variable* get_lon();
        virtual Dimension* get_lat_dim();
        virtual Dimension* get_lon_dim();

    protected:
        RegularGrid();
        RegularGrid(const RegularGrid &that);

        const Dataset *dataset;
        const Variable *grid_var;
};

#endif /* REGULARGRID_H_ */
