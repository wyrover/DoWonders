#ifndef CPARSEHEADER_H_
#define CPARSEHEADER_H_

#include <istream>          // for std::ifstream
#include <fstream>          // for std::ifstream
#include <iterator>         // for std::istreambuf_iterator

#include "CParser.h"        // for cparser::Parser
#include "CScanner.h"       // for cparser::Scanner
#include "CActions.h"    // for cparser::Actions

namespace cparser
{
    template <class Iterator>
    bool parse(shared_ptr<TransUnit>& tu, Iterator begin, Iterator end,
               bool is_64bit = false)
    {
        using namespace cparser;
        Actions as;
        Scanner<Iterator, Actions> scanner(as, is_64bit);

        std::vector<CR_TokenInfo> infos;
        scanner.scan(infos, begin, end);
        #if 0
            scanner.show_tokens(infos.begin(), infos.end());
            std::printf("\n--------------\n");
            fflush(stdout);
        #endif

        Parser<shared_ptr<Node>, Actions> parser(as);
        std::vector<CR_TokenInfo>::iterator it, end2 = infos.end();
        for (it = infos.begin(); it != end2; ++it) {
            #if 0
                std::printf("%s\n", scanner.token_to_string(*it).c_str());
                fflush(stdout);
            #endif
            if (parser.post(it->m_token, make_shared<CR_TokenInfo>(*it))) {
                if (parser.error()) {
                    as.location() = it->location();
                    as.message("ERROR: syntax error near " + 
                               scanner.token_to_string(*it));

                    // show around tokens
                    std::string around;
                    int count = 50;
                    for (int i = 0; i < count / 2; ++i) {
                        if (infos.begin() != it)
                            --it;
                        else
                            break;
                    }
                    for (int i = 0; i < count; ++i) {
                        if (infos.end() != it) {
                            around += scanner.token_to_string(*it);
                            around += " ";
                            ++it;
                        }
                        else
                            break;
                    }
                    as.message("around: " + around);
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

        return false;
    }

    inline bool parse_string(shared_ptr<TransUnit>& ts, const char *s,
                             bool is_64bit = false)
    {
        return parse(ts, s, s + std::strlen(s), is_64bit);
    }

    inline bool parse_string(shared_ptr<TransUnit>& ts, const std::string& str,
                             bool is_64bit = false)
    {
        return parse(ts, str.begin(), str.end(), is_64bit);
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
