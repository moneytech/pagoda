#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <getopt.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>

#include "CommandException.H"
#include "CommandLineOption.H"
#include "CommandLineParser.H"
//#include "Error.H"


CommandLineParser::CommandLineParser()
    :   options()
    ,   options_map()
    ,   positional_arguments()
{
}


CommandLineParser::~CommandLineParser()
{
}


void CommandLineParser::push_back(CommandLineOption &option)
{
    options.push_back(&option);
    vector<string> names = option.get_names();
    for (vector<string>::iterator name_it=names.begin();
            name_it!=names.end(); ++name_it) {
        options_map[*name_it] = &option;
    }
}


void CommandLineParser::erase(CommandLineOption &option)
{
    optvec_t::iterator it;
    vector<string> names;

    it = std::find(options.begin(), options.end(), &option);
    assert(it != options.end());
    options.erase(it);

    names = option.get_names();
    for (vector<string>::iterator name_it=names.begin();
            name_it!=names.end(); ++name_it) {
        options_map.erase(*name_it);
    }
}


void CommandLineParser::parse(int argc, char **argv)
{
    int opt;
    string opt_string;
    vector<struct option> long_opts;
    int long_index = -1;
    optvec_t::const_iterator it;

    // build the opt_string and long_opts
    opt_string.reserve(options.size()*2);
    long_opts.reserve(options.size()*2);
    for (it=options.begin(); it!=options.end(); ++it) {
        vector<struct option> current_long_opts = (*it)->get_long_options();
        opt_string += (*it)->get_option();
        //long_opts.push_back((*it)->get_option_long());
        long_opts.insert(long_opts.end(),
                         current_long_opts.begin(), current_long_opts.end());
    }

    // do the parsing
    while ((opt = getopt_long(argc, argv, opt_string.c_str(),
                              &long_opts[0], &long_index)) != -1) {
        string name = long_index >= 0 ? long_opts[long_index].name : "";
        string arg = optarg ? optarg : "";
        if (opt == '?') {
            // getopt prints a warning
            throw CommandException("see --help for details");
        }
        for (it=options.begin(); it!=options.end(); ++it) {
            if ((*it)->handle(opt, arg, name)) {
                break;
            }
        }
        // reset long_index since getopt_long won't necessarily
        long_index = -1;
    }

    // the positional arguments are left, put them in a vector<string>
    while (optind < argc) {
        positional_arguments.push_back(argv[optind++]);
    }
}


int CommandLineParser::count(const string &name) const
{
    optmap_t::const_iterator it = options_map.find(name);
    if (it != options_map.end()) {
        return it->second->get_count();
    }
    return 0;
}


int CommandLineParser::count(const CommandLineOption &option) const
{
    return count(option.get_name());
}


string CommandLineParser::get_argument(const string &name) const
{
    optmap_t::const_iterator it = options_map.find(name);
    if (it != options_map.end()) {
        return it->second->get_argument();
    }
    return "";
}


string CommandLineParser::get_argument(const CommandLineOption &option) const
{
    return get_argument(option.get_name());
}


vector<string> CommandLineParser::get_arguments(const string &name) const
{
    optmap_t::const_iterator it = options_map.find(name);
    if (it != options_map.end()) {
        return it->second->get_arguments();
    }
    return vector<string>();
}


vector<string> CommandLineParser::get_arguments(const CommandLineOption &option) const
{
    return get_arguments(option.get_name());
}


vector<string> CommandLineParser::get_positional_arguments() const
{
    return positional_arguments;
}


string CommandLineParser::get_usage() const
{
    string ret;
    for (optvec_t::const_iterator it=options.begin(); it!=options.end(); ++it) {
        ret += (*it)->get_usage() + "\n";
    }
    return ret;
}
