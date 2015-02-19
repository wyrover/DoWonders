#ifndef CPARSEHEADER_H_
#define CPARSEHEADER_H_

#include <istream>          // for std::ifstream
#include <fstream>          // for std::ifstream
#include <iterator>         // for std::istreambuf_iterator

#include "CParser.h"        // for cparser::Parser

namespace cparser
{
    typedef std::istreambuf_iterator<char>  my_iterator;
    typedef std::string::iterator           str_iterator;
    typedef TokenInfo<Token>                token_type;
    typedef std::vector<token_type>         token_container;
    typedef token_container::iterator       token_iterator;
}

#include "CScanner.h"       // for cparser::Scanner
#include "CActions.h"    // for cparser::Actions

namespace cparser
{
    inline bool parse(
        shared_ptr<TransUnit>& tu,
        my_iterator begin, my_iterator end,
        bool is_64bit = false)
    {
        using namespace cparser;
        auto error_info = make_shared<CR_ErrorInfo>();

        Actions as;
        Scanner scanner(error_info, is_64bit);

        std::vector<CR_TokenInfo> infos;
        scanner.scan(infos, begin, end);
        #if 0
            std::printf("\n#2\n");
            scanner.show_tokens(infos.begin(), infos.end());
            std::printf("\n--------------\n");
            fflush(stdout);
        #endif

        Parser<shared_ptr<Node>, Actions> parser(as);
        auto infos_end = infos.end();
        for (auto it = infos.begin(); it != infos_end; ++it) {
            if (parser.post(it->m_token, make_shared<CR_TokenInfo>(*it))) {
                if (parser.error()) {
                    // show around tokens
                    std::string around;
                    int count = 50;
                    auto it_save = it;
                    for (int i = 0; i < count / 2; ++i) {
                        if (infos.begin() != it)
                            --it;
                        else
                            break;
                    }
                    for (int i = 0; i < count; ++i) {
                        if (infos_end != it) {
                            around += scanner.token_to_string(*it);
                            around += " ";
                            ++it;
                        }
                        else
                            break;
                    }
                    error_info.get()->add_error(
                        it_save->location(),
                        "Syntax error near " + scanner.token_to_string(*it_save) +
                        "\n" + "around: " + around
                    );
                    error_info->emit_all();
                    return false;
                }
                break;
            }
        }

        shared_ptr<Node> node;
        if (parser.accept(node)) {
            std::fprintf(stderr, "Parser accepted!\n");
            tu = static_pointer_cast<TransUnit, Node>(node);
            return true;
        }

        error_info->emit_all();
        return false;
    }

    inline bool parse_file(shared_ptr<TransUnit>& ts, const char *filename,
                           bool is_64bit = false)
    {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::istreambuf_iterator<char> begin(file), end;
            bool ok = parse(ts, begin, end, is_64bit);
            file.close();
            return ok;
        }
        return false;
    }
} // namespace cparser

#endif  // ndef CPARSEHEADER_H_
