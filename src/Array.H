#ifndef ARRAY_H_
#define ARRAY_H_

#include <stdint.h>

#include <vector>

using std::vector;

#include "DataType.H"
#include "Error.H"
#include "ProcessGroup.H"

class Dimension;
class IndexHyperslab;
class LatLonBox;
class Validator;

/**
 * N-dimensional distributed Array interface.
 *
 * Some of the methods of Array assume the data is distributed.  Each process
 * may "own" a portion of the Array's data.  Data is assumed to be distributed
 * in some form of cartesian distribution.  When accessed, the local data can
 * be manipulated as a 1D C array.  This is how new operations may be written.
 *
 * Unfortunately, to work around certain deficiencies in some IO libraries, we
 * had to introduce the notion of separate read, write, and storage types.
 * Each Array has a concrete type which represents the DataType of each
 * element in the Array.  However, when reading from disk using an IO library,
 * the IO library may not allow data conversion (i.e. the data is stored on
 * disk as a char array but we want to promote it to an int in memory.)  We
 * must then read into a temporary buffer of the correct type, then cast into
 * an int Array.  This is handled by setting the "read_type" to something
 * other than the "type".  Similarly, when writing back to disk, the
 * "write_type" can be different than "type".  Typically, read_type ==
 * write_type == type.  And in rare cases, read_type == write_type != type.
 * And in no case thus far are all types different.
 */
class Array
{
    public:

        /**
         * Factory Array creation taking a shape directly.
         *
         * @param[in] type of Array to create
         * @param[in] shape of the Array to create
         * @return the new Array
         */
        static Array* create(DataType type, vector<int64_t> shape);

        /**
         * Factory Array creation taking a shape directly.
         *
         * @param[in] type of Array to create
         * @param[in] shape of the Array to create
         * @param[in] group the process group
         * @return the new Array
         */
        static Array* create(DataType type, vector<int64_t> shape,
                const ProcessGroup &group);

        /**
         * Factory Array creation deriving shape from dims.
         *
         * @param[in] type of Array to create
         * @param[in] dims the shape is taken from the given Dimensions
         * @return the new Array
         */
        static Array* create(DataType type, vector<Dimension*> dims);

        /**
         * Factory Array creation deriving shape from dims.
         *
         * @param[in] type of Array to create
         * @param[in] dims the shape is taken from the given Dimensions
         * @param[in] group the process group
         * @return the new Array
         */
        static Array* create(DataType type, vector<Dimension*> dims,
                const ProcessGroup &group);

        /**
         * Destroys the Array.
         */
        virtual ~Array();

        /**
         * Returns the DataType of the Array.
         *
         * @return the DataType
         */
        virtual DataType get_type() const = 0;

        /**
         * Gets the DataType to use when writing the data.
         */
        virtual DataType get_write_type() const = 0;

        /**
         * Sets the DataType to use when writing the data.
         */
        virtual void set_write_type(DataType type) = 0;

        /**
         * Gets the DataType to use when reading the data.
         */
        virtual DataType get_read_type() const = 0;

        /**
         * Sets the DataType to use when reading the data.
         */
        virtual void set_read_type(DataType type) = 0;

        /**
         * Returns the shape of the Array.
         *
         * @return the shape
         */
        virtual vector<int64_t> get_shape() const = 0;

        /**
         * Returns the size of the Array.
         *
         * @return the global number of elements in the Array
         */
        virtual int64_t get_size() const = 0;

        /**
         * Returns the local shape of the Array portion owned by this process.
         *
         * @return the local shape
         */
        virtual vector<int64_t> get_local_shape() const = 0;

        /**
         * Returns the local size of the Array portion owned by this process.
         *
         * @return the local number of elements in the Array
         */
        virtual int64_t get_local_size() const = 0;

        /**
         * Returns the number of dimensions in this Array.
         *
         * This is also known as the rank of the Array.
         *
         * @return the number of dimensions in this Array
         */
        virtual int64_t get_ndim() const = 0;

        /**
         * Returns true if the number of dimensions is zero.
         *
         * @return true if the number of dimensions is zero.
         */
        virtual bool is_scalar() const = 0;

        /**
         * Fills the array with the given value.
         *
         * @param[in] value to fill with
         */
        virtual void fill(void *value) = 0;

        /**
         * Fills the array with the given value.
         *
         * @param[in] value to fill with
         */
        template <class CT> void fill_value(CT value) {
            DataType type = get_type();
#define DATATYPE_EXPAND(DT,T) \
            if (type == DT) { \
                T the_value = static_cast<T>(value); \
                fill(&the_value); \
            } else
#include "DataType.def"
            {
                EXCEPT(DataTypeException, "DataType not handled", type);
            }
        }

        /**
         * Copies all elements from the given Array into this Array.
         *
         * The arrays must be the same type and shape.
         *
         * @param[in] src the Array to copy
         */
        virtual void copy(const Array *src) = 0;

        /**
         * Copies elements in a patch of one Array into another one.
         *
         * The patches of Arrays may be of different shapes but must have the
         * same number of elements.  Patches must be nonoverlapping
         * (if src == dst).
         *
         * @param[in] src the Array to copy
         * @param[in] src_lo lower bounds of src Array
         * @param[in] src_hi upper bounds of src Array
         * @param[in] dst_lo lower bounds of dst Array
         * @param[in] dst_hi upper bounds of dst Array
         */
        virtual void copy(const Array *src,
                          const vector<int64_t> &src_lo,
                          const vector<int64_t> &src_hi,
                          const vector<int64_t> &dst_lo,
                          const vector<int64_t> &dst_hi) = 0;

        /**
         * Duplicates this Array but as a new type.
         *
         * @param[in] new_type the type of the new Array
         * @return the new Array, cast to the given type
         */
        virtual Array* cast(DataType new_type) const = 0;

        /**
         * Duplicates this Array and tranposes the elements.
         *
         * Here, tranpose can mean an arbitrary transpose i.e.
         * 2,3,4,5 --> 5,4,3,2 --or--
         * 2,3,4,5 --> 2,5,3,4
         *
         * @param[in] axes how to reorder the Array dimensions
         *            for example 2,3,4 --> 4,2,3 needs 2,0,1
         *            for example 2,3,4 --> 4,3,2 needs 2,1,0
         * @param[in] dst_array a preallocated Array, or NULL to create one
         * @return the new Array, transposed appropriately
         */
        virtual Array* transpose(const vector<int64_t> &axes,
                                 Array *dst_array=NULL) const = 0;

        /**
         * Duplicates this Array and tranposes the elements, possibly reversing
         * some of the axes.
         *
         * Here, tranpose can mean an arbitrary transpose i.e.
         * 2,3,4,5 --> 5,4,3,2 --or--
         * 2,3,4,5 --> 2,5,3,4
         *
         * @param[in] axes how to reorder the Array dimensions
         *            for example 2,3,4 --> 4,2,3 needs 2,0,1
         *            for example 2,3,4 --> 4,3,2 needs 2,1,0
         * @param[in] reverse which axes to reverse (1 keep same, -1 reverse)
         * @param[in] dst_array a preallocated Array, or NULL to create one
         * @return the new Array, transposed appropriately
         */
        virtual Array* transpose(const vector<int64_t> &axes,
                                 const vector<int64_t> &reverse,
                                 Array *dst_array=NULL) const = 0;

        /**
         * Duplicates this Array i.e. same shape, type, and data.
         */
        virtual Array* clone() const = 0;

        /**
         * Returns true if the distribution of this Array matches the other.
         *
         * @param[in] other the other Array
         * @return true if the distribution of this Array matches the other.
         */
        virtual bool same_distribution(const Array *other) const = 0;

        /**
         * Returns true if this process owns a local portion of the Array.
         *
         * @return true if this process owns a local portion of the Array.
         */
        virtual bool owns_data() const = 0;

        /**
         * Gets the local distribution of the Array.
         *
         * lo and hi will be empty if this process does not own any data.
         *
         * @param[in,out] lo starting indices for Array section
         * @param[in,out] hi ending indices for Array section
         */
        virtual void get_distribution(vector<int64_t> &lo,
                                      vector<int64_t> &hi) const = 0;

        /**
         * Returns the starting address of the local data
         *
         * This will return NULL if this process doesn't have a local portion
         * Because this method is not const, you MUST call release_update() to
         * indicate the memory is no longer in use (do NOT delete the
         * returned pointer, simply call release_update() when finished).
         *
         * @return the starting address of the local data
         */
        virtual void* access() = 0;

        /**
         * Returns the starting address of the local data
         *
         * This will return NULL if this process doesn't have a local portion
         * Because this method is const, you MUST call release() to
         * indicate the memory is no longer in use (do NOT delete the
         * returned pointer, simply call release() when finished).
         *
         * @return the starting address of the local data
         */
        virtual const void* access() const = 0;

        /**
         * Indicate the pointer returned by access() const is no longer in
         * use.
         */
        virtual void release() const = 0;

        /**
         * Indicate the pointer returned by access() is no longer in use.
         * This implies the data was modified (or potentially modified.)
         */
        virtual void release_update() = 0;

        /**
         * Copies all data from Array into local buffer and returns the buffer.
         *
         * Caller must cast the returned buffer to the appropriate type and
         * must also free the returned buffer when finished.
         *
         * The buffer argument is optional.  If not NULL, the buffer should
         * point to an appropriately sized and typed contiguous block of
         * memory.  If NULL, an appropriately sized and typed buffer will be
         * allocated via new.
         *
         * @param[in] buffer to copy into
         * @return the buffer; new if buffer was NULL
         */
        virtual void* get(void *buffer=NULL) const = 0;

        /**
         * Copies data from Array into local buffer and returns the buffer.
         *
         * Caller must cast the returned buffer to the appropriate type and
         * must also free the returned buffer when finished.  Shortcut for a
         * 1-Dimensional get.  It is an error to call this if the rank of the
         * Array is not 1.
         *
         * The buffer argument is optional.  If not NULL, the buffer should
         * point to an appropriately sized and typed contiguous block of
         * memory.  If NULL, an appropriately sized and typed buffer will be
         * allocated via new.
         *
         * @param[in] lo the lowest index in the range to retrieve
         * @param[in] hi the highest index in the range to retrieve
         * @param[in] buffer to copy into
         * @return the buffer; new if buffer was NULL
         */
        virtual void* get(int64_t lo, int64_t hi, void *buffer=NULL) const = 0;

        /**
         * Copies data from Array into local buffer and returns the buffer.
         *
         * Caller must cast the returned buffer to the appropriate type and
         * must also free the returned buffer when finished.
         *
         * The buffer argument is optional.  If not NULL, the buffer should
         * point to an appropriately sized and typed contiguous block of
         * memory.  If NULL, an appropriately sized and typed buffer will be
         * allocated via new.
         *
         * @param[in] lo the lowest indices to retrieve
         * @param[in] hi the highest indices to retrieve
         * @param[in] buffer to copy into
         * @return the buffer; new if buffer was NULL
         */
        virtual void* get(const vector<int64_t> &lo,
                          const vector<int64_t> &hi,
                          void *buffer=NULL) const = 0;

        /**
         * Copies data from the source buffer into into the Array.
         *
         * The buffer is assumed to both have the same number of elements as
         * the entire Array and also to be contiguous.
         *
         * @param[in] buffer the contiguous buffer
         */
        virtual void put(void *buffer) = 0;

        /**
         * Copies data from the source buffer into into the 1-D Array.
         *
         * The buffer is assumed both to have the same number of elements as
         * the indicated patch and also to be contiguous.
         *
         * It is an error to call this if the rank of the Array is not 1.
         *
         * @param[in] buffer the contiguous buffer
         * @param[in] lo
         * @param[in] hi
         */
        virtual void put(void *buffer, int64_t lo, int64_t hi) = 0;

        /**
         * Copies data from the source buffer into into the Array patch.
         *
         * The buffer is assumed to have the same number of elements as the
         * indicated patch.
         *
         * @param[in] buffer the contiguous buffer
         * @param[in] lo
         * @param[in] hi
         */
        virtual void put(void *buffer,
                         const vector<int64_t> &lo,
                         const vector<int64_t> &hi) = 0;

        /**
         * Accumulates data from the source buffer into into the Array.
         *
         * The buffer is assumed to both have the same number of elements as
         * the entire Array and also to be contiguous.
         *
         * @param[in] buffer the contiguous buffer
         */
        virtual void acc(void *buffer, void *alpha=NULL) = 0;

        /**
         * Accumulates data from the source buffer into into the 1-D Array.
         *
         * The buffer is assumed both to have the same number of elements as
         * the indicated patch and also to be contiguous.
         *
         * It is an error to call this if the rank of the Array is not 1.
         *
         * @param[in] buffer the contiguous buffer
         * @param[in] lo
         * @param[in] hi
         */
        virtual void acc(void *buffer, int64_t lo, int64_t hi,
                         void *alpha=NULL) = 0;

        /**
         * Accumulates data from the source buffer into into the Array patch.
         *
         * The buffer is assumed to have the same number of elements as the
         * indicated patch.
         *
         * @param[in] buffer the contiguous buffer
         * @param[in] lo
         * @param[in] hi
         */
        virtual void acc(void *buffer,
                         const vector<int64_t> &lo,
                         const vector<int64_t> &hi,
                         void *alpha=NULL) = 0;

        /**
         * Scatters values into this Array.
         *
         * The order of the values in the provided buffer should match the
         * order of the specified subscripts.
         *
         * @param[in] buffer the values to scatter
         * @param[in] subscripts the indices of where to scatter
         */
        virtual void scatter(void *buffer,
                             vector<int64_t> &subscripts) = 0;

        /**
         * Gathers values from this Array into a local buffer.
         *
         * The order of the values in the returned buffer match the order of
         * the specified subscripts.
         *
         * @param[in] subscripts the indices from where to gather
         * @param[in] buffer contiguous local array to copy values into
         * @return the gathered values
         */
        virtual void* gather(vector<int64_t> &subscripts,
                             void *buffer=NULL) const = 0;

        /**
         * Adds two Arrays.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to add
         * @return the result, as a new Array
         */
        virtual Array* add(const Array *rhs) const = 0;

        /**
         * Adds two Arrays, in-place.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to add
         * @return the result, as this Array
         */
        virtual Array* iadd(const Array *rhs) = 0;

        /**
         * Reduces this Array to a single value using addition.
         *
         * If the Array has an associated Validator it will be consulted.
         */
        virtual Array* reduce_add() const = 0;

        /**
         * Reduces this Array by summing over the indicated dimensions.
         *
         * The dimensions to sum over are indicated by <= 0.
         *
         * @param shape the shape of the array, with <= 0 to indicate a
         *              dimension to remove via summation
         * @return the reduced Array
         */
        virtual Array* reduce_add(const vector<int64_t> &shape) const = 0;

        /**
         * Reduces this Array to a single, maximum value.
         *
         * If the Array has an associated Validator it will be consulted.
         */
        virtual Array* reduce_max() const = 0;

        /**
         * Reduces this Array by maximizing over the indicated dimensions.
         */
        virtual Array* reduce_max(const vector<int64_t> &shape) const = 0;

        /**
         * Reduces this Array to a single, minimum value.
         *
         * If the Array has an associated Validator it will be consulted.
         */
        virtual Array* reduce_min() const = 0;

        /**
         * Reduces this Array by minimizing over the indicated dimensions.
         */
        virtual Array* reduce_min(const vector<int64_t> &shape) const = 0;

        /**
         * Subtracts two Arrays.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to subtract
         * @return the result, as a new Array
         */
        virtual Array* sub(const Array *rhs) const = 0;

        /**
         * Subtracts two Arrays, in-place.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to subtract
         * @return the result, as this Array
         */
        virtual Array* isub(const Array *rhs) = 0;

        /**
         * Multiplies two Arrays.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to multiply
         * @return the result, as a new Array
         */
        virtual Array* mul(const Array *rhs) const = 0;

        /**
         * Multiplies two Arrays.
         *
         * The given Array is assumed to have fewer dimensions than
         * the current Array. An explicit broadcasting shape must also be
         * supplied. Missing dimensions are indicated by <= 0 for a given
         * dimension index.
         *
         * @param[in] rhs the Array to multiply
         * @param[in] shape the shape of the Array to multiply
         * @return the result, as a new Array
         */
        virtual Array* mul(const Array *rhs, const vector<int64_t> &shape) const = 0;

        /**
         * Multiplies two Arrays, in-place.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to multiply
         * @return the result, as this Array
         */
        virtual Array* imul(const Array *rhs) = 0;

        /**
         * Multiplies two Arrays, in-place.
         *
         * The given Array is assumed to have fewer dimensions than
         * the current Array. An explicit broadcasting shape must also be
         * supplied. Missing dimensions are indicated by <= 0 for a given
         * dimension index.
         *
         * @param[in] rhs the Array to multiply
         * @param[in] shape the shape of the Array to multiply
         * @return the result, as a new Array
         */
        virtual Array* imul(const Array *rhs, const vector<int64_t> &shape) = 0;

        /**
         * Divides two Arrays.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to divide
         * @return the result, as a new Array
         */
        virtual Array* div(const Array *rhs) const = 0;

        /**
         * Divides two Arrays, in-place.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to divide
         * @return the result, as this Array
         */
        virtual Array* idiv(const Array *rhs) = 0;

        /**
         * Takes the element-wise maximum of two Arrays.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to consider
         * @return the result, as a new Array
         */
        virtual Array* max(const Array *rhs) const = 0;

        /**
         * Takes the element-wise maximum of two Arrays, in-place.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to consider
         * @return the result, as this Array
         */
        virtual Array* imax(const Array *rhs) = 0;

        /**
         * Takes the element-wise minimum of two Arrays.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to consider
         * @return the result, as a new Array
         */
        virtual Array* min(const Array *rhs) const = 0;

        /**
         * Takes the element-wise minimum of two Arrays, in-place.
         *
         * If the given Array has fewer Dimensions, but the shape is otherwise
         * compatible, the given Array is "broadcast" to fit.
         *
         * @param[in] rhs the Array to consider
         * @return the result, as this Array
         */
        virtual Array* imin(const Array *rhs) = 0;

        /**
         * Computes the element-wise power of the Array.
         *
         * @param[in] exponent to raise each value to
         * @return the result, as a new Array
         */
        virtual Array* pow(double exponent) const = 0;

        /**
         * Computes the element-wise power of the Array, in-place.
         *
         * @param[in] exponent to raise each value to
         * @return the result, as this Array
         */
        virtual Array* ipow(double exponent) = 0;

        /**
         * Sets the data Validator for this Array.
         *
         * All operations adhere to this Validator.
         *
         * @param[in] validator the validator
         */
        virtual void set_validator(Validator *validator) = 0;

        /**
         * Returns true if a Validator is set for this Array.
         */
        virtual bool has_validator() const = 0;

        /**
         * Returns the data Validator for this Array.
         */
        virtual Validator* get_validator() const = 0;

        /**
         * Sets a counter for arithmetic operations (used with validators).
         *
         * @param[in] counter
         */
        virtual void set_counter(Array *counter) = 0;

        /**
         * Returns a bitmask Array with the same shape as this Array using the
         * current validator.
         *
         * @return the result as an Array
         */
        virtual Array* get_mask() const = 0;

        /* ################################################################
         * THE FOLLOWING FUNCTIONS ARE SPECIFIC TO "MASK"
         * ################################################################ */

        /**
         * Factory mask creation for 1-dimensional mask instances.
         *
         * @param[in] name the name of the mask
         * @param[in] size the size of the mask
         */
        static Array* mask_create(const string &name, int64_t size);

        /**
         * Factory mask creation convenience function for 1-dimensional mask
         * instances corresponding to a specific Dimension instance.
         *
         * The returned mask will have the same number of elements as the
         * given Dimension.
         *
         * @param[in] dim the Dimension to mask
         */
        static Array* mask_create(const Dimension *dim);

        /**
         * Factory mask creation for N-dimensional masks.
         *
         * @param[in] shape of the mask
         */
        static Array* mask_create(const vector<int64_t> &shape);

        /**
         * Sets the name of the associated Dimension.
         */
        virtual void set_name(const string &name) = 0;

        /**
         * Returns the name of the associated Dimension, if any.
         *
         * @return the name of the associated Dimension, if any
         */
        virtual string get_name() const = 0;

        /**
         * Returns the number of set bits in this mask.
         *
         * A bit is set if it is any non-zero value.
         *
         * @return the number of set bits in this mask
         */
        virtual int64_t get_count() = 0;

        /**
         * Set all mask bits to zero.
         */
        virtual void clear() = 0;

        /**
         * Set all mask bits to one.
         */
        virtual void reset() = 0;

        /**
         * Set bits to one if they fall within the given hyperslab.
         *
         * This method is intended for 1-dimensional Array instances only.
         *
         * @param[in] hyperslab the specified hyperslab to mask
         */
        virtual void modify(const IndexHyperslab &hyperslab) = 0;

        /**
         * Set bits to one if they fall within the given LatLonBox.
         *
         * This version of modify assumes the lat and lon Arrays share a
         * single Dimension.
         *
         * @param[in] box the latitude/longitude box
         * @param[in] lat the latitude coordinate Array
         * @param[in] lon the longitude coordinate Array
         */
        virtual void modify(const LatLonBox &box,
                            const Array *lat, const Array *lon) = 0;

        /**
         * Set bits to one if they fall within the given low and hi range.
         *
         * Assumes the given Array has the same size as this mask.
         * In other words, the given Array is a coordinate Array.
         *
         * @param[in] low the lower end of the range
         * @param[in] hi the upper end of the range
         * @param[in] var the values to compare against low and hi
         */
        virtual void modify(double low, double hi, const Array *var) = 0;

        /**
         * Set bits to one if they fall within the half-open range [value,..].
         *
         * Assumes the given Array has the same size as this mask.
         * In other words, the given Array is a coordinate Array.
         *
         * @param[in] value values must be greater than this value
         * @param[in] var the values to compare against low and hi
         */
        virtual void modify_gt(double value, const Array *var) = 0;

        /**
         * Set bits to one if they fall within the half-open range [..,value].
         *
         * Assumes the given Array has the same size as this mask.
         * In other words, the given Array is a coordinate Array.
         *
         * @param[in] value values must be less than this value
         * @param[in] var the values to compare against low and hi
         */
        virtual void modify_lt(double value, const Array *var) = 0;

        /**
         * Return a new Array containing the enumeration of this mask.
         *
         * The enumeration takes into account the masked bits.  The first set
         * bit is 0, the next is 1, and so on.  Any unset bit is set to -1.
         *
         * The Array should NOT be deleted -- it is owned by this mask.  The
         * enumeration will be recomputed only as needed, as well.
         *
         * For example:
         * mask:   0  0  0  0  1  1  1  0  1  2 -1  0  1
         * Rtrn:  -1 -1 -1 -1  0  1  2 -1  3  4  5 -1  6
         *
         * @return the enumerated, reindexed Array
         */
        virtual Array* reindex() = 0;

        /**
         * Return a new Array containing the partial sum of this mask.
         *
         * The Array should NOT be deleted -- it is owned by this mask.  The
         * partial_sum will be recomputed only as needed, as well.
         *
         * @param[in] exclude whether the first value is always 0
         * @return the partially summed Array
         */
        virtual Array* partial_sum(bool exclude) = 0;

        /**
         * Change any non-zero value to 1 within this Array.
         */
        virtual void normalize() = 0;

        /**
         * Virtual friend implementation of stream insertion operator.
         *
         * @param[in] os the stream
         */
        virtual ostream& print(ostream &os) const = 0;

        friend ostream& operator << (ostream &os, const Array *array);

        /**
         * Dump the contents of this Array to the screen.
         */
        virtual void dump() const = 0;

    protected:
        /**
         * Constructs the Array interface.
         */
        Array();
};

#endif /* ARRAY_H_ */
