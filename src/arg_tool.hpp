// authors: Sebastian Reiter
// s.b.reiter@gmail.com

#ifndef __H__PROMESH_arg_tool
#define __H__PROMESH_arg_tool

#include <sstream>

class ArgTool {
public:
	ArgTool (int argc, const char** argv) :
		_argc (argc),
		_argv (argv)
	{}

	void new_section(const std::string& grpName)
	{
		_help << std::endl << "  " << grpName << std::endl;
	}

	bool has_param (const std::string& name, const std::string& desc)
	{
		add_help_entry (name, desc);
		return param_index(name) >= 0;
	}

	double get_double (const std::string& name, double defVal,
	                   const std::string& desc)
	{
		add_help_entry (name, desc);
		int pi = param_index (name);
		if(pi >= 0 && (pi + 1 < _argc)){
			return std::atof(_argv[pi+1]);
		}
		return defVal;
	}

	std::string get_string (const std::string& name, const std::string& defVal,
	                   		const std::string& desc)
	{
		add_help_entry (name, desc);
		int pi = param_index (name);
		if(pi >= 0 && (pi + 1 < _argc))
			return _argv[pi+1];
		return defVal;
	}

	const std::string get_help () const
	{
		return _help.str();
	}

private:
	int param_index (const std::string& name) const
	{
		for(int i = 0; i < _argc; ++i){
			if(name == _argv[i])
				return i;
		}
		return -1;
	}

	void add_help_entry (const std::string& name, const std::string& desc)
	{
		using namespace std;
		_help << "\n  " << name;
		const size_t len = name.size () + 2;
		if (len < _left_col_width)
			_help << string(_left_col_width - len, ' ');

		size_t cur = 0;
		size_t next = 0;
		bool multiline = false;
		while (cur < desc.size()) {
			next = desc.find('\n', cur);
			if(next != string::npos){
				size_t tmpNext = next;
				if((tmpNext > 0) && (desc[tmpNext-1] == '\r'))
					--tmpNext;
				if(multiline)
					_help << string(_left_col_width, ' ');
				_help << desc.substr(cur, tmpNext - cur + 1);
				cur = next + 1;
				if(cur >= desc.size())
					break;
			}
			else{
				if(multiline)
					_help << string(_left_col_width, ' ');
				_help << desc.substr(cur, desc.size() - cur) << endl;
				break;
			}
			multiline = true;
		}
		if(multiline)
			_help << endl;
	}

private:
	int _argc;
	const char** _argv;
	std::stringstream	_help;
	static constexpr size_t _left_col_width = 12;
};

#endif