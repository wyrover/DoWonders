#include "stdafx.h"

void WsShowHelp(void) {
    std::cout <<
        "won32_sanitizer --- Won32 sanitizer" << std::endl <<
        "Usage: won32_sanitizer [--prefix ...] [--suffix ...]" << std::endl;
}

void WsShowVersion(void) {
    std::cout <<
#ifdef _WIN64
# ifdef __GNUC__
        "won32_sanitizer --- Won32 sanitizer for 64-bit gcc\n"
# else
        "won32_sanitizer --- Won32 sanitizer for 64-bit cl (VC++)\n"
# endif
#else
# ifdef __GNUC__
        "won32_sanitizer --- Won32 sanitizer for 32-bit gcc\n"
# else
        "won32_sanitizer --- Won32 sanitizer for 32-bit cl (VC++)\n"
# endif
#endif
        "Version 0.2" << std::endl;
}
bool WsJustDoIt(
    const std::string& prefix, const std::string& suffix)
{
    std::string suffix2 = suffix;
    if (suffix2.find(".dat") == suffix2.size() - 4) {
        suffix2.resize(suffix2.size() - 4);
    }
    std::string strOutput = prefix + "sanitize" + suffix2 + ".c";

    auto error_info = make_shared<CR_ErrorInfo>();
    #ifdef _WIN64
        CR_NameScope ns(error_info, true);
    #else
        CR_NameScope ns(error_info, false);
    #endif

    if (!ns.LoadFromFiles(prefix, suffix)) {
        std::cerr << "ERROR: cannot load data" << std::endl;
        return false;
    }

    std::fstream out(strOutput, std::ios::out | std::ios::trunc);
    if (!out) {
        std::cerr << "ERROR: cannot create file '" <<
                     strOutput << "'" << std::endl;
        return false;
    }
    out <<
        "#include \"win32.h\"\n" << 
        "#include <stdio.h>\n" << 
        "\n" << 
        "/* fixup */\n" <<
        "#undef RASCTRYINFO\n" <<
        "#undef RASIPADDR\n" <<
        "#undef PROCESSENTRY32\n" << 
        "#undef MODULEENTRY32\n" << 
        "\n" << 
        "#define check_size(name,size) do { \\\n" << 
        "\tif (sizeof(name) != (size)) { \\\n" << 
        "\t\tfprintf(stderr, \"%s: size mismatched, real size is %d\\n\", #name, (int)sizeof(name)); \\\n" << 
        "\t\treturn 1; \\\n" << 
        "\t} \\\n" << 
        "} while (0) \n" << 
        "\n" << 
        "#define check_value(name,value) do { \\\n" << 
        "\tif ((name) != (value)) { \\\n" << 
        "\t\tfprintf(stderr, \"%s: value mismatched, real value is %d\\n\", #name, (int)name); \\\n" << 
        "\t\treturn 2; \\\n" << 
        "\t} \\\n" << 
        "} while (0) \n" << 
        "\n" <<
        "#define check_align(name,align) do { \\\n" << 
        "\tif (__alignof(name) != (align)) { \\\n" << 
        "\t\tfprintf(stderr, \"%s: alignment mismatched, real value is %d\\n\", #name, (int)__alignof(name)); \\\n" << 
        "\t\treturn 3; \\\n" << 
        "\t} \\\n" << 
        "} while (0) \n" << 
        "\n" <<
        "int main(void) {" << std::endl;

    for (size_t i = 0; i < ns.LogTypes().size(); ++i) {
        auto type_id = i;
        auto name = ns.NameFromTypeID(type_id);
        auto& type = ns.LogType(type_id);
        auto flags = type.m_flags;
        auto size = type.m_size;
        auto align = type.m_align;
        if (size && name.size() && name.find("*") == std::string::npos) {
            const long excl_flags =
                (TF_ENUM | TF_FUNCTION | TF_INCOMPLETE | TF_INACCURATE);
            if (!(flags & excl_flags)) {
                out << "\tcheck_align(" << name << ", " << align << ");" <<
                       std::endl;
                out << "\tcheck_size(" << name << ", " << size << ");" <<
                       std::endl;
            }
        }
    }

    for (size_t i = 0; i < ns.LogTypes().size(); ++i) {
        auto type_id = i;
        auto name = ns.NameFromTypeID(type_id);
        auto& type = ns.LogType(type_id);
        auto flags = type.m_flags;
        if ((flags & (TF_ENUM | TF_ALIAS)) == TF_ENUM) {
            auto eid = type.m_sub_id;
            auto& le = ns.LogEnum(eid);

            for (auto& it : le.m_mNameToValue) {
                out << "\tcheck_value(" << it.first << ", " <<
                       it.second << ");" << std::endl; 
            }
        }
    }

    out <<
        "\tputs(\"success\");\n" <<
        "\treturn 0;\n" <<
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
