#ifndef COMMANDLINEOPTION_H_
#define COMMANDLINEOPTION_H_

#include <iostream>
#include <string>
#include <vector>

using std::endl;
using std::ostream;
using std::string;
using std::vector;


static string OP_AVG("avg");
static string OP_SQRAVG("sqravg");
static string OP_AVGSQR("avgsqr");
static string OP_MAX("max");
static string OP_MIN("min");
static string OP_RMS("rms");
static string OP_RMSSDN("rmssdn");
static string OP_SQRT("sqrt");
static string OP_TTL("ttl");
static string OP_ADD("+");
static string OP_DIV("/");
static string OP_MUL("*");
static string OP_SUB("-");
static string OP_EQ("eq");
static string OP_NE("ne");
static string OP_GE("ge");
static string OP_LE("le");
static string OP_GT("gt");
static string OP_LT("lt");
static string OP_EQ_SYM("=");
static string OP_NE_SYM("!=");
static string OP_GE_SYM(">=");
static string OP_LE_SYM("<=");
static string OP_GT_SYM(">");
static string OP_LT_SYM("<");

/** The types of averages supported. */
enum AverageOperator {
    AO_AVG,
    AO_SQRAVG,
    AO_AVGSQR,
    AO_MAX,
    AO_MIN,
    AO_RMS,
    AO_RMSSDN,
    AO_SQRT,
    AO_TTL,
};

/** The types of binary operations supported. */
enum BinaryOperator {
    BO_ADD,
    BO_DIV,
    BO_MUL,
    BO_SUB,
};


/**
 * A CommandLineOption mimics the GNU getopt struct long_option.
 *
 * This class allows for the collection of multiple long options into a single
 * logical entity.  This allows for aliases, shorthands, etc.
 */
class CommandLineOption
{
    public:
        static unsigned int WIDTH;

        CommandLineOption(const int &value, const string &name,
                          const bool &argument, const string &description,
                          const string &extra_name1="",
                          const string &extra_name2="",
                          const string &extra_name3="");
        ~CommandLineOption();

        void set_value(const int &value);
        void set_name(const string &name);
        void set_names(const vector<string> &names);
        void has_argument(const bool &argument);
        void set_description(const string &description);

        int get_value() const;
        string get_name() const;
        vector<string> get_names() const;
        bool has_argument() const;
        string get_description() const;

        bool handle(int value, const string &arg, const string &name="");
        string get_usage() const;
        string get_option() const;
        struct option get_long_option() const;
        vector<struct option> get_long_options() const;
        int get_count() const;
        string get_argument() const;
        vector<string> get_arguments() const;

        static struct option get_long_opt_null();

        friend ostream& operator<<(ostream &os, const CommandLineOption &op);

        static CommandLineOption ALPHABETIZE;
        static CommandLineOption APPEND;
        static CommandLineOption ARRANGE;
        static CommandLineOption AUXILIARY;
        static CommandLineOption AVERAGE_OPERATION;
        static CommandLineOption AVERAGE_DIMENSIONS;
        static CommandLineOption BINARY_OPERATION;
        static CommandLineOption CB_BUFFER_SIZE;
        static CommandLineOption CDF1;
        static CommandLineOption CDF2;
        static CommandLineOption CDF5;
        static CommandLineOption COORDS;
        static CommandLineOption DIMENSION;
        static CommandLineOption DIMENSION_MASK;
        static CommandLineOption EXCLUDE;
        static CommandLineOption FILE_FORMAT;
        static CommandLineOption FIX_RECORD_DIMENSION;
        static CommandLineOption GROUPS;
        static CommandLineOption HEADER_PAD;
        static CommandLineOption HELP;
        static CommandLineOption HISTORY;
        static CommandLineOption INPUT_PATH;
        static CommandLineOption INTERPOLATE;
        static CommandLineOption JOIN;
        static CommandLineOption LATLONBOX;
        static CommandLineOption MASK_COMPARATOR;
        static CommandLineOption MASK_CONDITION;
        static CommandLineOption MASK_NAME;
        static CommandLineOption MASK_VALUE;
        static CommandLineOption NC4;
        static CommandLineOption NC4_CLASSIC;
        static CommandLineOption NO_COORDS;
        static CommandLineOption NO_NORMALIZATION;
        static CommandLineOption NONBLOCKING_IO;
        static CommandLineOption OUTPUT;
        static CommandLineOption OVERWRITE;
        static CommandLineOption READ_ALL_RECORDS;
        static CommandLineOption READ_ALL_VARIABLES;
        static CommandLineOption RETAIN_DEGENERATE_DIMENSIONS;
        static CommandLineOption ROMIO_CB_READ;
        static CommandLineOption ROMIO_DS_READ;
        static CommandLineOption ROMIO_NO_INDEP_RW;
        static CommandLineOption STRIPING_UNIT;
        static CommandLineOption TOPOLOGY;
        static CommandLineOption UNION;
        static CommandLineOption UNLIMITED_DIMENSION_NAME;
        static CommandLineOption VARIABLE;
        static CommandLineOption VERBOSE;
        static CommandLineOption VERS;
        static CommandLineOption WEIGHTS_OF_FILES;
        static CommandLineOption WEIGHTING_VARIABLE_NAME;
        static CommandLineOption WEIGHT_MASK_COORDINATE_VARIABLES;
        static CommandLineOption COMPRESS;

    protected:
        int _value;
        vector<string> _names;
        bool _has_argument;
        string _description;
        int _count;
        vector<string> _arguments;
};

ostream& operator<<(ostream &os, const struct option &op);

#endif // COMMANDLINEOPTION_H_
