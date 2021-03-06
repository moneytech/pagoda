#ifndef DIMENSION_H_
#define DIMENSION_H_

#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>

using std::ostream;
using std::string;
using std::vector;

class Array;
class Dataset;


/**
 * A Dimension is a named size.
 *
 * Although it is illogical for a Dimension's size to be less than or equal to
 * zero, this interface does allow it due to the way certain libraries
 * implement Dimensions (notably the netcdf library) as integers.
 *
 * An unlimited Dimension is also taken into account -- another throwback to
 * netcdf which this interface is modeled after.
 */
class Dimension
{
    public:
        /**
         * Dimension destructor.
         */
        virtual ~Dimension();

        /**
         * Returns the name of this Dimension.
         *
         * @return the name of this Dimension.
         */
        virtual string get_name() const = 0;

        /**
         * Returns the size of this Dimension.
         *
         * @return the size of this Dimension.
         */
        virtual int64_t get_size() const = 0;

        /**
         * Returns true if this Dimension is of unlimited size.
         *
         * @return true if this Dimension is of unlimited size.
         */
        virtual bool is_unlimited() const = 0;

        /**
         * Returns the Dataset this Dimension is associated with.
         *
         * @return the Dataset this Dimension is associated with.
         */
        virtual Dataset* get_dataset() const = 0;

        /**
         * Returns the mask associated with this Dimension.
         *
         * @return the mask associated with this Dimension.
         */
        virtual Array* get_mask() const;

        /**
         * Write this Dimension to the given stream.
         *
         * @param[in,out] os the stream
         * @return the stream
         */
        virtual ostream& print(ostream &os) const = 0;

        friend ostream& operator << (ostream &os, const Dimension *other);

        /**
         * Compares to another Dimension.
         *
         * Sizes and names are compared.
         */
        static bool equal(const Dimension *left, const Dimension *right);

        /**
         * Tests two vectors of Dimension instances for equality.
         */
        static bool equal(const vector<Dimension*> &left,
                          const vector<Dimension*> &right);

        /**
         * Tests two vectors of Dimension instances for conformity.
         *
         *  Two vectors of Dimension instances conform if the right-hand side
         *  has the same or fewer Dimension instances and the Dimension
         *  instances in the right-hand side occur in the same order as the
         *  left-hand side.
         */
        static bool conforms(const vector<Dimension*> &left,
                             const vector<Dimension*> &right);

    protected:
        /**
         * Protected constructor.
         */
        Dimension();

        /**
         * Protected copy constructor.
         *
         * @param[in] copy the Dimension to copy
         */
        Dimension(const Dimension &copy);

        /**
         * Protected assignment operator.
         *
         * @param[in] copy the Dimension to copy
         */
        Dimension& operator = (const Dimension &copy);
};

#endif // DIMENSION_H_
