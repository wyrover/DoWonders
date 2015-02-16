#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>

#include <string>
#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>
using namespace std;

void WsShowHelp(void) {
    std::cout <<
        "won32_sanitizer --- Won32 sanitizer" << endl <<
        "Usage: won32_sanitizer [--prefix ...] [--suffix ...]" << std::endl;
}

void WsShowVersion(void) {
    std::cout <<
        "won32_sanitizer --- Won32 sanitizer" << std::endl <<
        "Version 0.0" << std::endl;
}

void WsSplit(std::vector<std::string>& v, const std::string& s, char separator) {
    std::size_t i = 0, j = s.find_first_of(separator);

    v.clear();
    while (j != std::string::npos) {
        v.push_back(s.substr(i, j - i));
        i = j + 1;
        j = s.find_first_of(separator, i);
    }
    v.push_back(s.substr(i, -1));
}

void WsSplitByTabs(std::vector<std::string>& fields, const std::string& buf) {
    WsSplit(fields, buf, '\t');
}

bool WsJustDoIt(
    const std::string& prefix, const std::string& suffix)
{
    std::string strOutput = prefix + "sanitize" + suffix + ".cpp";

    std::fstream out(strOutput, std::ios::out | std::ios::trunc);
    if (!out) {
        std::cerr << "ERROR: cannot create file '" <<
                     strOutput << "'" << std::endl;
        return false;
    }

    out << "#include <iostream>\n" << 
        "#include \"win32.h\"\n" << 
        "\n" << 
        "#define check_size(name,size) do { \\\n" << 
        "\tif (sizeof(name) != (size)) { \\\n" << 
        "\t\tstd::cerr << #name << \": size mismatch\" << std::endl; \\\n" << 
        "\t\treturn 1; \\\n" << 
        "\t} \\\n" << 
        "} while (0) \n" << 
        "\n" << 
        "#define check_value(name,value) do { \\\n" << 
        "\tif ((name) != (value)) { \\\n" << 
        "\t\tstd::cerr << #name << \": value mismatch\" << std::endl; \\\n" << 
        "\t\treturn 1; \\\n" << 
        "\t} \\\n" << 
        "} while (0) \n" << 
        "\n" << 
        "int main(void) {" << std::endl;

    std::vector<std::string> fields;

    std::fstream in1((prefix + "types" + suffix), std::ios::in);
    if (in1) {
        std::string line;

        // skip title line
        std::getline(in1, line);

        while (std::getline(in1, line)) {
            WsSplitByTabs(fields, line);

            //auto type_id = atoi(fields[0].data());
            auto name = fields[1];
            //auto flags = strtol(fields[2].data(), NULL, 16);
            //auto sub_id = atoi(fields[3].data());
            //auto count = atoi(fields[4].data());
            auto size = atoi(fields[5].data());
            if (size && name.size() && name.find("*") == std::string::npos) {
                out << "\tcheck_size(" << name << ", " << size << ");" <<
                       std::endl; 
            }
        }
    } else {
        out.close();
        //remove(strOutput.data());
        return false;
    }

    std::fstream in2((prefix + "structures" + suffix), std::ios::in);
    if (in2) {
        std::string line;

        // skip title line
        std::getline(in2, line);

        while (std::getline(in2, line)) {
            WsSplitByTabs(fields, line);

            //auto type_id = atoi(fields[0].data());
            auto name = fields[1];
            //auto struct_id = atoi(fields[2].data());
            //auto struct_or_union = atoi(fields[3].data());
            auto size = atoi(fields[4].data());
            //auto count = atoi(fields[5].data());
            if (size && name.size()) {
                out << "\tcheck_size(" << name << ", " << size << ");" <<
                       std::endl; 
            }
        }
    } else {
        out.close();
        //remove(strOutput.data());
        return false;
    }

    std::fstream in3((prefix + "enumitems" + suffix), std::ios::in);
    if (in3) {
        std::string line;

        // skip title line
        std::getline(in3, line);

        while (std::getline(in3, line)) {
            WsSplitByTabs(fields, line);

            auto name = fields[0];
            //auto enum_type_id = atoi(fields[1].data());
            auto value = atoi(fields[2].data());
            //auto enum_name = fields[3];
            if (name.size()) {
                out << "\tcheck_value(" << name << ", " << value << ");" <<
                       std::endl; 
            }
        }
    } else {
        out.close();
        //remove(strOutput.data());
        return false;
    }

    out << "\treturn 0;\n" <<
            "}" << std::endl;
    out.close();
    return true;
}

int main(int argc, char **argv) {
    bool show_help = false;
    bool show_version = false;
    const char *prefix = "";
    const char *suffix = ".dat";

    for (int i = 1; i < argc; ++i) {
        if (_stricmp(argv[i], "/?") == 0 ||
            _stricmp(argv[i], "--help") == 0)
        {
            show_help = true;
        } else if (_stricmp(argv[i], "--version") == 0 ||
                   _stricmp(argv[i], "/version") == 0)
        {
            show_version = true;
        } else if (_stricmp(argv[i], "--prefix") == 0 ||
                   _stricmp(argv[i], "-p") == 0) {
            ++i;
            prefix = argv[i];
        } else if (_stricmp(argv[i], "--suffix") == 0 ||
                   _stricmp(argv[i], "-s") == 0)
        {
            ++i;
            suffix = argv[i];
        } else {
            std::cerr << "ERROR: Invalid option '" << argv[i] <<
                         "'." << std::endl;
            return 1;
        }
    }

    if (show_help) {
        WsShowHelp();
        return 0;
    }

    if (show_version) {
        WsShowVersion();
        return 0;
    }

    WsJustDoIt(prefix, suffix);

    return 0;
}
