#include "stdafx.h"

static
void CrReplaceString(
    std::string& str, const std::string& from, const std::string& to)
{
    size_t i = 0;
    for (;;)
    {
        i = str.find(from, i);
        if (i == std::string::npos)
            break;

        str.replace(i, from.size(), to);
        i += to.size();
    }
}

void CrTrimString(std::string& str) {
    static const char *spaces = " \t";
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if (i != std::string::npos) {
        if (j != std::string::npos) {
            str = str.substr(i, j - i + 1);
        } else {
            str = str.substr(i);
        }
    } else {
        if (j != std::string::npos) {
            str = str.substr(0, j + 1);
        }
    }
}

void IwShowHelp(void) {
    std::cout <<
        "iwon --- Won32 interactive" << std::endl <<
        "Usage: iwon [--prefix ...] [--suffix ...] [type or constant]" << std::endl;
}

void IwShowVersion(void) {
    std::cout <<
#ifdef _WIN64
# ifdef __GNUC__
        "iwon --- Won32 interactive for 64-bit gcc\n"
# else
        "iwon --- Won32 interactive for 64-bit cl (VC++)\n"
# endif
#else
# ifdef __GNUC__
        "iwon --- Won32 interactive for 32-bit gcc\n"
# else
        "iwon --- Won32 interactive for 32-bit cl (VC++)\n"
# endif
#endif
        "Version 0.2" << std::endl;
}

bool IwJustDoIt(CR_NameScope& ns, const std::string& target) {
    bool ret = false;
    // type
    {
        auto it = ns.MapNameToTypeID().find(target);
        if (it != ns.MapNameToTypeID().end()) {
            auto tid = it->second;
            auto rtid = ns.ResolveAlias(tid);
            auto& type = ns.LogType(tid);
            auto& rtype = ns.LogType(rtid);

            if (type.m_location.m_file == "(predefined)") {
                std::cout << target << " is a predefined type." << std::endl;
            } else {
                // type
                if (rtype.m_flags & TF_STRUCT) {
                    // struct type
                    std::cout << target << " is a struct type, defined in " <<
                                 type.m_location.str() << "." << std::endl;
                    if (tid != rtid) {
                        std::cout << "typedef " << ns.StringOfType(tid, target, true) << ";" << std::endl;
                    } else {
                        std::cout << ns.StringOfType(tid, "", true) << ";" << std::endl;
                    }
                } else if (rtype.m_flags & TF_UNION) {
                    // union type
                    std::cout << target << " is a union type, defined in " <<
                                 type.m_location.str() << "." << std::endl;
                    if (tid != rtid) {
                        std::cout << "typedef " << ns.StringOfType(tid, target, true) << ";" << std::endl;
                    } else {
                        std::cout << ns.StringOfType(tid, "", true) << ";" << std::endl;
                    }
                } else if (rtype.m_flags & TF_POINTER) {
                    // pointer type
                    std::cout << target << " is a pointer type, defined in " <<
                                 type.m_location.str() << "." << std::endl;
                    std::cout << "typedef " << ns.StringOfType(tid, target, true) << ";" << std::endl;
                } else if (rtype.m_flags & TF_FUNCTION) {
                    // function type
                    std::cout << target << " is a function type, defined in " <<
                                 type.m_location.str() << "." << std::endl;
                    std::cout << "typedef " << ns.StringOfType(tid, target, true) << ";" << std::endl;
                } else {
                    if (ns.IsIntegralType(rtid)) {
                        // integral type
                        if (ns.IsUnsignedType(rtid)) {
                            std::cout << target << " is an unsigned integral type, defined in " <<
                                         type.m_location.str() << "." << std::endl;
                            std::cout << "typedef " << ns.StringOfType(tid, target, true) << ";" << std::endl;
                        } else {
                            std::cout << target << " is a signed integral type, defined in " <<
                                         type.m_location.str() << "." << std::endl;
                            std::cout << "typedef " << ns.StringOfType(tid, target, true) << ";" << std::endl;
                        }
                    } else if (ns.IsFloatingType(rtid)) {
                        std::cout << target << " is a floating-point type, defined in " <<
                                     type.m_location.str() << "." << std::endl;
                        std::cout << "typedef " << ns.StringOfType(tid, target, true) << ";" << std::endl;
                    } else {
                        // other type
                        std::cout << target << " is a type, defined in " <<
                                     type.m_location.str() << "." << std::endl;
                        std::cout << "typedef " << ns.StringOfType(rtid, target, true) << ";" << std::endl;
                    }
                }
            }
            std::cout << "size: " << type.m_size << std::endl;
            ret = true;
        }
    }
    // variable or function
    {
        auto it = ns.MapNameToVarID().find(target);
        if (it != ns.MapNameToVarID().end()) {
            auto vid = it->second;
            auto& var = ns.LogVar(vid);
            auto& value = var.m_typed_value;
            auto tid = value.m_type_id;
            auto rtid = tid;
            if (tid != cr_invalid_id) {
                rtid = ns.ResolveAlias(tid);
            }
            if (ns.IsFuncType(rtid)) {
                // function
                std::cout << target << " is a function, defined in " <<
                    var.m_location.str() << "." << std::endl;
                std::cout << ns.StringOfType(rtid, target, true) << ";" << std::endl;
            }
            else {
                if (var.m_is_macro) {
                    assert(rtid != cr_invalid_id);
                    if (var.m_location.m_file == "(predefined)") {
                        // predefined macro constant
                        std::cout << target << " is a predefined macro constant." << std::endl;
                        std::cout << ns.StringOfType(tid, target, true) << " = " <<
                            value.m_text << value.m_extra << ";" << std::endl;
                    }
                    else {
                        // macro constant
                        std::cout << target << " is a macro constant, defined in " <<
                            var.m_location.str() << "." << std::endl;
                        std::cout << ns.StringOfType(tid, target, true) << " = " <<
                            value.m_text << value.m_extra << ";" << std::endl;
                    }
                    auto& type = ns.LogType(rtid);
                    std::cout << "size: " << type.m_size << std::endl;
                }
                else {
                    if (rtid != cr_invalid_id) {
                        // variable with value
                        std::cout << target << " is a variable, defined in " <<
                            var.m_location.str() << "." << std::endl;
                        std::cout << ns.StringOfType(tid, target, true) <<
                            " = " << value.m_text << value.m_extra <<
                            ";" << std::endl;
                        auto& type = ns.LogType(rtid);
                        std::cout << "size: " << type.m_size << std::endl;
                    }
                    else {
                        // variable without value
                        std::cout << target << " is a variable without value, defined in " <<
                            var.m_location.str() << "." << std::endl;
                        std::cout << ns.StringOfType(tid, target, true) << ";" << std::endl;
                        auto& type = ns.LogType(rtid);
                        std::cout << "size: " << type.m_size << std::endl;
                    }
                }
            }
            ret = true;
        }
    }
    // name alias
    {
        auto it = ns.MapNameToName().find(target);
        if (it != ns.MapNameToName().end()) {
            auto name2name = it->second;
            std::cout << target << " is an alias macro of " << 
                name2name.m_to << ", defined in " <<
                name2name.m_location.str() << "." << std::endl;
            ret = true;
        }
    }
    if (!ret) {
        std::cout << "I don't know." << std::endl;
    }
    return ret;
}

bool IwJustDoIt(
    const std::string& prefix, const std::string& suffix,
    const std::string& target)
{
    auto error_info = make_shared<CR_ErrorInfo>();
    #ifdef _WIN64
        CR_NameScope ns(error_info, true);
    #else
        CR_NameScope ns(error_info, false);
    #endif

    #if 0
        std::cerr << "Type [Enter] key" << std::endl;
        getchar();
    #endif

    std::cout << "Loading data..." << std::endl;
    if (!ns.LoadFromFiles(prefix, suffix)) {
        std::cerr << "ERROR: cannot load data" << std::endl;
        return false;
    }
    std::cout << "Loaded." << std::endl << std::endl;

    if (target.size()) {
        return IwJustDoIt(ns, target);
    } else {
        std::cout << "Enter 'quit' to quit." << std::endl;
        std::cout << std::endl;
        std::string line;
        for (;;) {
            std::cout << std::endl;
            std::cout << "Enter name of type, function or constant: ";
            std::getline(std::cin, line);
            CrTrimString(line);
            CrReplaceString(line, "  ", " ");
            if (line == "quit" || line == "Quit" || line == "QUIT") {
                std::cout << "See you!" << std::endl;
                break;
            }
            IwJustDoIt(ns, line);
        }
        return true;
    }

}

int main(int argc, char **argv) {
    bool show_help = false;
    bool show_version = false;
    const char *prefix = "";
    const char *suffix = ".dat";

#if 0
    printf("Hit [Enter] key!");
    getchar();
#endif

    std::string target;
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
        } else if (argv[i][0] == '-' || argv[i][0] == '/') {
            std::cerr << "ERROR: Invalid option '" << argv[i] <<
                         "'." << std::endl;
            return 1;
        } else {
            target = argv[i];
        }
    }

    if (show_help) {
        IwShowHelp();
        return 0;
    }

    if (show_version) {
        IwShowVersion();
        return 0;
    }

    IwJustDoIt(prefix, suffix, target);

    return 0;
}
