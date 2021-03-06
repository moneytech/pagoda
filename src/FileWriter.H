#ifndef FILEWRITER_H_
#define FILEWRITER_H_

#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>

#include "FileFormat.H"

class Array;
class Attribute;
class DataType;
class Dataset;
class Dimension;
class Values;
class Variable;

using std::ostream;
using std::string;
using std::vector;


/**
 * Interface for writing a Dataset to a file.
 *
 * Following the constraints of the netcdf format, the file must be completely
 * defined (all Dimensions, Variables, Variable Attributes, and global
 * Attributes) before writing of data can take place.
 *
 * As an optimization, it is recommended that for record Variables, the data
 * is written one complete record at a time (versus one entire Variable at a
 * time) in order to write large, contiguous chunks of data.  This can be
 * accomplished using the non-blocking interface.
 */
class FileWriter
{
    public:
        typedef FileWriter*(*writer_t)(const string &, FileFormat);

        static FileWriter* open(const string &filename, FileFormat format);

        static void register_writer(writer_t writer);

        FileWriter();
        virtual ~FileWriter();
        virtual void close() = 0;

        // named parameter idiom, but using virtuals
        virtual FileWriter* create() = 0; // finalizes parameters
        virtual FileWriter* fixed_record_dimension(int value) = 0;
        virtual FileWriter* header_pad(int value) = 0;
        virtual FileWriter* file_format(FileFormat value) = 0;
        virtual FileWriter* append(bool value) = 0;
        virtual FileWriter* overwrite(bool value) = 0;
        virtual FileWriter* compress(bool value) = 0;

        virtual void def_dataset(Dataset *dataset) = 0;

        virtual void def_dim(const string &name, int64_t size) = 0;
        virtual void def_dim(Array *mask) = 0;
        virtual void def_dim(Dimension *dim) = 0;
        virtual void def_dims(const vector<Array*> &masks) = 0;
        virtual void def_dims(const vector<Dimension*> &dims) = 0;

        virtual void def_var(const string &name,
                             const vector<string> &dims,
                             const DataType &type,
                             const vector<Attribute*> &atts=vector<Attribute*>()) = 0;
        virtual void def_var(const string &name,
                             const vector<Dimension*> &dims,
                             const DataType &type,
                             const vector<Attribute*> &atts=vector<Attribute*>()) = 0;
        virtual void def_var(const string &name,
                             const vector<Array*> &dims,
                             const DataType &type,
                             const vector<Attribute*> &atts=vector<Attribute*>()) = 0;
        virtual void def_var(Variable *var) = 0;
        virtual void def_vars(const vector<Variable*> &vars) = 0;

        virtual void write_att(const string &name, Values *values,
                               DataType type,
                               const string &var_name=string("")) = 0;
        virtual void write_att(Attribute *att,
                               const string &name=string("")) = 0;
        virtual void write_atts(const vector<Attribute*> &atts,
                                const string &name=string("")) = 0;

        virtual void write(Array *array, const string &name) = 0;
        virtual void write(Array *array, const string &name,
                           int64_t record) = 0;
        virtual void write(Array *array, const string &name,
                           int64_t ensemble, int64_t record) = 0;
        virtual void write(Array *array, const string &name,
                           const vector<int64_t> &start) = 0;

        virtual void iwrite(Array *array, const string &name) = 0;
        virtual void iwrite(Array *array, const string &name,
                            int64_t record) = 0;
        virtual void iwrite(Array *array, const string &name,
                            int64_t ensemble, int64_t record) = 0;
        virtual void iwrite(Array *array, const string &name,
                            const vector<int64_t> &start) = 0;

        virtual void copy_vars(const vector<Variable*> &vars) = 0;
        virtual void icopy_vars(const vector<Variable*> &vars) = 0;

        virtual void wait() = 0;

        virtual ostream& print(ostream &os) const = 0;
        friend ostream& operator << (ostream &os, const FileWriter *writer);

    private:
        static vector<writer_t> writers;
};

#endif // FILEWRITER_H_
