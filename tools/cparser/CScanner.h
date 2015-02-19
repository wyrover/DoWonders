#ifndef CSCANNER_H_
#define CSCANNER_H_

namespace cparser
{
    //
    // Packing
    //
    class Packing {
    public:
        Packing() : m_pack(8) { }

        void push_pack(int pack) {
            assert(pack > 0);
            m_packs.push_back(pack);
            m_pack = pack;
        }

        void pop_pack() {
            assert(m_packs.size());
            m_pack = m_packs.back();
            m_packs.pop_back();
        }

        int  get_pack() const       { return m_pack; }
        void set_pack(int pack = 8) { m_pack = pack; }
        operator int() const        { return get_pack(); }

    protected:
        int m_pack;
        std::vector<int> m_packs;
    }; // Packing

    //
    // ScannerBase
    //
    class ScannerBase {
    public:
        ScannerBase(scanner_iterator begin, scanner_iterator end) :
            m_current(begin), m_end(end) { }

        char getch() {
            char c;
            if (is_eof()) {
                c = -1;
            } else {
                c = *m_current++;
            }
            return c;
        } // getch

        bool is_eof() const {
            return m_current == m_end || *m_current == -1;
        }

              Packing& packing()       { return m_packing; }
        const Packing& packing() const { return m_packing; }
              CR_Location& location()       { return m_location; }
        const CR_Location& location() const { return m_location; }

    protected:
        scanner_iterator    m_current;
        scanner_iterator    m_end;
        Packing             m_packing;
        CR_Location         m_location;
    };

    //
    // Scanner
    //
    class Scanner {
    public:
        Scanner(shared_ptr<CR_ErrorInfo> error_info, bool is_64bit = false) :
            m_error_info(error_info), m_is_64bit(is_64bit),
            m_in_c_comment(false), m_in_cxx_comment(false) { }

        bool is_in_block_comment() const { return m_in_c_comment; }
        bool is_in_line_comment() const { return m_in_cxx_comment; }

        void scan(token_container& infos,
                  scanner_iterator begin, scanner_iterator end)
        {
            ScannerBase base(begin, end);

            // get tokens
            token_container read_infos;
            while (get_tokens(base, read_infos)) {
                infos.insert(infos.end(), read_infos.begin(), read_infos.end());
                read_infos.clear();
            }
            infos.insert(infos.end(), read_infos.begin(), read_infos.end());
            read_infos.clear();
            infos.emplace_back(eof);

            #if 0
                std::printf("\n#0\n");
                show_tokens(infos.begin(), infos.end());
                std::printf("\n--------------\n");
                fflush(stdout);
            #endif

            // token resynthesization
            resynth1(base, infos);
            resynth2(infos);
            resynth3(infos.begin(), infos.end());
            resynth4(infos);
            resynth5(infos.begin(), infos.end());

            m_type_names.clear();   // no use
        } // scan

        void show_tokens(token_iterator begin, token_iterator end) const {
            for (auto it = begin; it != end; ++it) {
                std::printf("%s ", token_to_string(*it).data());
            }
        }

        std::string token_to_string(const token_type& info) const {
            std::string str = token_label(info.m_token);
            if (info.m_text.size()) {
                str += "(" + info.m_text + ")";
            }
            return str;
        }

        bool token_pattern_match(token_iterator it, token_iterator end,
                                 const std::vector<Token>& tokens) const {
            token_iterator saved = it;
            for (auto token : tokens) {
                if (it == end ||
                    (it->m_token != token && it->m_token != eof))
                {
                    it = saved;
                    return false;
                }
                ++it;
            }
            return true;
        }

        void newline(token_container& infos) {
            infos.emplace_back(T_NEWLINE, "\\n");
        }

        void skip_block_comment(ScannerBase& base, token_container& infos) {
            char c;
            do {
                c = base.getch();
                if (c == '\n') {
                    newline(infos);
                } else if (c == '*') {
                    c = base.getch();
                    if (c == '/') {
                        infos.emplace_back(T_C_COMMENT_END, "*/");
                        m_in_c_comment = false;
                        break;  // closed
                    }
                }
            } while (c != -1);
        } // skip_block_comment

        void skip_line_comment(ScannerBase& base, token_container& infos) {
            char c;
            do {
                c = base.getch();
                if (c == '\n') {
                    newline(infos);
                    m_in_cxx_comment = false;
                    break;  // closed
                }
            } while (c != EOF);
        } // skip_line_comment

        std::string
        guts_escape_sequence(str_iterator& it, str_iterator end) const {
            std::string result;
            if (it != end && *it == '\\') {
                ++it;
                if (it == end) {
                    return result;
                }
                switch (*it) {
                case '\'': case '\"': case '?': case '\\':
                    result += *it;
                    break;

                case '0': result += '\0'; break;
                case 'a': result += '\a'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'v': result += '\v'; break;

                case 'x':
                    {
                        ++it;
                        auto hex = guts_hex(it, end);
                        result +=
                            static_cast<char>(
                                std::strtoul(hex.data(), NULL, 16));
                    }
                    break;

                case 'u':
                case 'U':
                    {
                        ++it;
                        auto hex = guts_hex(it, end);
                        result += '?';  // FIXME & TODO
                    }
                    break;

                default:
                    if ('0' <= *it && *it <= '7') {
                        auto octal = guts_octal(it, end);
                        result +=
                            static_cast<char>(
                                std::strtoul(octal.data(), NULL, 8));
                    } else {
                        result += *it;
                    }
                }
            }
            return result;
        } // guts_escape_sequence

        std::string guts_string(str_iterator& it, str_iterator end) const {
            std::string result;
            assert(it != end && *it == '\"');
            for (++it; it != end; ++it) {
                switch (*it) {
                case '\"':
                    ++it;
                    if (it != end) {
                        if (*it == '\"') {
                            result += '\"';
                        } else {
                            --it;
                            return result;
                        }
                    }
                    break;

                case '\\':
                    {
                        auto text = guts_escape_sequence(it, end);
                        result += text;
                    }
                    break;

                default:
                    result += *it;
                }
            }
            return result;
        } // guts_string

        std::string guts_char(str_iterator& it, str_iterator end) const {
            std::string result;
            assert(it != end && *it == '\'');
            for (++it; it != end; ++it) {
                switch (*it) {
                case '\'':
                    return result;

                case '\\':
                    {
                        auto text = guts_escape_sequence(it, end);
                        result += text;
                    }
                    break;

                default:
                    result += *it;
                }
            }
            return result;
        } // guts_char

        std::string guts_hex(str_iterator& it, str_iterator end) const {
            std::string result;
            for (; it != end; ++it) {
                if (isxdigit(*it)) {
                    result += *it;
                } else {
                    --it;
                    break;
                }
            }
            return result;
        } // guts_hex

        std::string guts_digits(str_iterator& it, str_iterator end) const {
            std::string result;
            for (; it != end; ++it) {
                if (isdigit(*it)) {
                    result += *it;
                } else {
                    --it;
                    break;
                }
            }
            return result;
        } // guts_digits

        std::string guts_octal(str_iterator& it, str_iterator end) const {
            std::string result;
            for (; it != end; ++it) {
                if ('0' <= *it && *it <= '7') {
                    result += *it;
                    if (result.size() == 3) {
                        break;
                    }
                } else {
                    --it;
                    break;
                }
            }
            return result;
        } // guts_octal

        std::string guts_floating(str_iterator& it, str_iterator end) const {
            std::string result;
            if (it != end && *it == '.') {
                ++it;
                std::string digits = guts_digits(it, end);
            }
            return result;
        } // guts_floating

        std::string guts_indentifier(str_iterator& it, str_iterator end) const {
            std::string result;
            if (it != end && (isalpha(*it) || *it == '_')) {
                result += *it;
                for (++it; it != end; ++it) {
                    if (isalnum(*it) || *it == '_') {
                        result += *it;
                    } else {
                        --it;
                        break;
                    }
                }
            }
            //std::puts(result.data());
            return result;
        } // guts_indentifier

        std::string guts_integer_suffix(
            str_iterator& it, str_iterator end, CR_TypeFlags& flags)
        {
            std::string result;
            for (; it != end; ++it) {
                if (*it == 'u' || *it == 'U') {
                    result += *it;
                    flags |= TF_UNSIGNED;
                } else if (*it == 'l' || *it == 'L') {
                    result += *it;
                    if (flags & TF_LONG) {
                        flags &= ~TF_LONG;
                        flags |= TF_LONGLONG;
                    } else {
                        flags |= TF_LONG;
                    }
                } else {
                    break;
                }
            }
            return result;
        } // guts_integer_suffix

        std::string guts_floating_suffix(
            str_iterator& it, str_iterator end, CR_TypeFlags& flags)
        {
            std::string result;
            for (; it != end; ++it) {
                if (*it == 'f' || *it == 'F') {
                    flags &= ~TF_DOUBLE;
                    flags |= TF_FLOAT;
                } else if (*it == 'l' || *it == 'L') {
                    flags |= TF_LONG | TF_DOUBLE;
                } else {
                    break;
                }
            }
            return result;
        } // guts_floating_suffix

        std::string guts_exponent(str_iterator& it, str_iterator end) {
            std::string result;
            if (it != end) {
                if (*it == 'e' || *it == 'E') {
                    result += *it;
                    ++it;
                    if (it != end) {
                        if (*it == '+') {
                            result += *it;
                            ++it;
                        } else if (*it == '-') {
                            result += *it;
                            ++it;
                        }
                        if (it != end) {
                            auto exponent = guts_digits(it, end);
                            result += exponent;
                        }
                    }
                }
            }
            return result;
        } // guts_exponent

        bool lexeme(str_iterator& it, str_iterator end, const std::string& str) {
            str_iterator saved = it;
            std::size_t i = 0, siz = str.size();
            for (; it != end && i < siz; ++it, ++i) {
                if (*it != str[i]) {
                    it = saved;
                    return false;
                }
            }
            return i == siz;
        } // lexeme

        std::string get_line(ScannerBase& base) {
            std::string line;
            // get line
            char ch;
            line.reserve(64);
            do {
                ch = base.getch();
                if (ch == '\n') {
                    line += ch;
                    break;
                }
                line += ch;
            } while (ch != -1);
            return line;
        }

        bool get_tokens(ScannerBase& base, token_container& infos) {
retry:
            if (base.is_eof()) {
                return false;
            }

            if (m_in_c_comment) {
                // /* ... */
                skip_block_comment(base, infos);
                goto retry;
            }

            if (m_in_cxx_comment) {
                // // ...
                skip_line_comment(base, infos);
                goto retry;
            }

            auto line = get_line(base);

            auto end = line.end();
            for (auto it = line.begin(); it != end; ++it) {
                std::string extra;
                switch (*it) {
                case ' ': case '\t':
                    continue;

                case '\n':
                    newline(infos);
                    break;

                case '#':
                    infos.emplace_back(T_SHARP, "#");
                    break;

                case 'u':
                    ++it;
                    if (*it == '8') {
                        ++it;
                        extra = "u8";
                    } else {
                        extra = "u";
                    }
                    if (*it == '"') {
                        auto text = guts_string(it, end);
                        infos.emplace_back(T_STRING, text, extra);
                    } else if (*it == '\'') {
                        auto text = guts_char(it, end);
                        infos.emplace_back(T_CONSTANT, text, extra);
                    } else {
                        --it;
                        goto label_default;
                    }
                    break;

                case 'L': case 'U':
                    extra += *it;
                    ++it;
                    if (*it == '"') {
                        auto text = guts_string(it, end);
                        infos.emplace_back(T_STRING, text, extra);
                    } else if (*it == '\'') {
                        auto text = guts_char(it, end);
                        infos.emplace_back(T_CONSTANT, text, extra);
                    } else {
                        --it;
                        goto label_default;
                    }
                    break;

                case '"':
                    {
                        auto text = guts_string(it, end);
                        infos.emplace_back(T_STRING, text);
                    }
                    break;

                case '\'':
                    {
                        auto text = guts_char(it, end);
                        infos.emplace_back(T_STRING, text);
                    }
                    break;

                case '0':
                    ++it;
                    if (*it == 'x' || *it == 'X') {
                        // 0x or 0X
                        ++it;
                        auto text = guts_hex(it, end);
                        CR_TypeFlags flags = 0;
                        extra = guts_integer_suffix(it, end, flags);
                        infos.emplace_back(T_CONSTANT, "0x" + text, extra, flags);
                    } else if (*it == '.') {
                        // 0.
                        auto text = guts_floating(it, end);
                        CR_TypeFlags flags = TF_DOUBLE;
                        extra = guts_floating_suffix(it, end, flags);
                        infos.emplace_back(T_CONSTANT, "0." + text, extra, flags);
                    } else if (isdigit(*it)) {
                        // octal
                        auto text = guts_octal(it, end);
                        CR_TypeFlags flags = 0;
                        extra = guts_integer_suffix(it, end, flags);
                        infos.emplace_back(T_CONSTANT, "0" + text, extra, flags);
                    }
                    break;

                case '.':
                    if (lexeme(it, end, "...")) {
                        infos.emplace_back(T_ELLIPSIS, "...");
                    } else {
                        // .
                        auto text = guts_floating(it, end);
                        if (text.size() > 1) {
                            CR_TypeFlags flags = TF_DOUBLE;
                            extra = guts_floating_suffix(it, end, flags);
                            infos.emplace_back(T_CONSTANT, "0." + text, extra, flags);
                        } else {
                            infos.emplace_back(T_DOT, ".");
                        }
                    }
                    break;

                case '<':
                    if (lexeme(it, end, "<<=")) {
                        infos.emplace_back(T_L_SHIFT_ASSIGN, "<<=");
                    } else if (lexeme(it, end, "<<")) {
                        infos.emplace_back(T_L_SHIFT, "<<");
                    } else if (lexeme(it, end, "<=")) {
                        infos.emplace_back(T_GE, "<=");
                    } else if (lexeme(it, end, "<:")) {
                        infos.emplace_back(T_L_BRACKET, "<:");
                    } else {
                        infos.emplace_back(T_GT, "<");
                    }
                    break;

                case '>':
                    if (lexeme(it, end, ">>=")) {
                        infos.emplace_back(T_L_SHIFT_ASSIGN, ">>=");
                    } else if (lexeme(it, end, ">>")) {
                        infos.emplace_back(T_L_SHIFT, ">>");
                    } else if (lexeme(it, end, ">=")) {
                        infos.emplace_back(T_GE, ">=");
                    } else {
                        infos.emplace_back(T_GT, ">");
                    }
                    break;

                case '+':
                    if (lexeme(it, end, "+=")) {
                        infos.emplace_back(T_ADD_ASSIGN, "+=");
                    } else if (lexeme(it, end, "++")) {
                        infos.emplace_back(T_INC, "++");
                    } else {
                        infos.emplace_back(T_PLUS, "+");
                    }
                    break;

                case '-':
                    if (lexeme(it, end, "-=")) {
                        infos.emplace_back(T_SUB_ASSIGN, "-=");
                    } else if (lexeme(it, end, "--")) {
                        infos.emplace_back(T_DEC, "--");
                    } else if (lexeme(it, end, "->")) {
                        infos.emplace_back(T_ARROW, "->");
                    } else {
                        infos.emplace_back(T_MINUS, "-");
                    }
                    break;

                case '*':
                    if (lexeme(it, end, "*=")) {
                        infos.emplace_back(T_MUL_ASSIGN, "*=");
                    } else {
                        infos.emplace_back(T_ASTERISK, "*");
                    }
                    break;

                case '/':
                    if (lexeme(it, end, "/*")) {    // */
                        m_in_c_comment = true;
                        infos.emplace_back(T_C_COMMENT_BEGIN, "/*");    // */
                        skip_block_comment(base, infos);
                    } else if (lexeme(it, end, "//")) {
                        m_in_cxx_comment = true;
                        infos.emplace_back(T_CPP_COMMENT, "//");
                        skip_line_comment(base, infos);
                    } else if (lexeme(it, end, "/=")) {
                        infos.emplace_back(T_DIV_ASSIGN, "/=");
                    } else {
                        infos.emplace_back(T_SLASH, "/");
                    }
                    break;

                case '%':
                    if (lexeme(it, end, "%=")) {
                        infos.emplace_back(T_MOD_ASSIGN, "%=");
                    } else if (lexeme(it, end, "%>")) {
                        infos.emplace_back(T_R_BRACE, "%>");
                    } else {
                        infos.emplace_back(T_PERCENT, "%");
                    }
                    break;

                case '&':
                    if (lexeme(it, end, "&=")) {
                        infos.emplace_back(T_AND_ASSIGN, "&=");
                    } else if (lexeme(it, end, "&&")) {
                        infos.emplace_back(T_L_AND, "&&");
                    } else {
                        infos.emplace_back(T_AND, "&");
                    }
                    break;
                    
                case '^':
                    if (lexeme(it, end, "^=")) {
                        infos.emplace_back(T_XOR_ASSIGN, "^=");
                    } else {
                        infos.emplace_back(T_XOR, "^");
                    }
                    break;

                case '|':
                    if (lexeme(it, end, "|=")) {
                        infos.emplace_back(T_OR_ASSIGN, "|=");
                    } else if (lexeme(it, end, "||")) {
                        infos.emplace_back(T_L_OR, "||");
                    } else {
                        infos.emplace_back(T_OR, "|");
                    }
                    break;

                case '=':
                    if (lexeme(it, end, "==")) {
                        infos.emplace_back(T_EQUAL, "==");
                    } else {
                        infos.emplace_back(T_ASSIGN, "=");
                    }
                    break;

                case '!':
                    if (lexeme(it, end, "!=")) {
                        infos.emplace_back(T_NE, "!=");
                    } else {
                        infos.emplace_back(T_NOT, "!");
                    }
                    break;

                case ';':
                    infos.emplace_back(T_SEMICOLON, ";");
                    break;

                case ':':
                    if (lexeme(it, end, ":>")) {
                        infos.emplace_back(T_R_BRACKET, ":>");
                    } else {
                        infos.emplace_back(T_COLON, ":");
                    }
                    break;

                case ',': infos.emplace_back(T_COMMA, ","); break;
                case '{': infos.emplace_back(T_L_BRACE, "{"); break;
                case '}': infos.emplace_back(T_R_BRACE, "}"); break;
                case '(': infos.emplace_back(T_L_PAREN, "("); break;
                case ')': infos.emplace_back(T_R_PAREN, ")"); break;
                case '[': infos.emplace_back(T_L_BRACKET, "["); break;
                case ']': infos.emplace_back(T_R_BRACKET, "]"); break;
                case '~': infos.emplace_back(T_BITWISE_NOT, "~"); break;
                case '?': infos.emplace_back(T_QUESTION, "?"); break;

label_default:
                default:
                    if (isspace(*it)) {
                        assert(*it != '\n');
                        continue;
                    }
                    if (isdigit(*it)) {
                        auto digits = guts_digits(it, end);
                        if (*it == '.') {
                            ++it;
                            // 123.1232
                            auto decimals = guts_digits(it, end);
                            auto text = digits + '.' + decimals;
                            CR_TypeFlags flags = TF_DOUBLE;
                            extra = guts_floating_suffix(it, end, flags);
                            infos.emplace_back(T_CONSTANT, text, extra, flags);
                        } else {
                            auto exponent = guts_exponent(it, end);
                            if (exponent.size()) {
                                // exponent was found. it's a floating
                                CR_TypeFlags flags = TF_DOUBLE;
                                extra = guts_floating_suffix(it, end, flags);
                                auto text = digits + exponent + extra;
                                infos.emplace_back(T_CONSTANT, text, extra, flags);
                            } else {
                                // exponent not found. it's a integer
                                CR_TypeFlags flags = 0;
                                extra = guts_integer_suffix(it, end, flags);
                                infos.emplace_back(T_CONSTANT, digits, extra, flags);
                            }
                        }
                    } else if (isalpha(*it) || *it == '_') {
                        // identifier or keyword
                        auto text = guts_indentifier(it, end);
                        if (text.find("__builtin_") == 0 && text.size() > 10) {
                            if (text != "__builtin_va_list") {
                                text = text.substr(10);
                            }
                        }
                        Token token = parse_identifier(text);
                        infos.emplace_back(token, text);
                    } else {
                        std::string text;
                        text += *it;
                        infos.emplace_back(T_INVALID_CHAR, text);
                    }
                } // switch (*it)
            } // for (auto it = line.begin(); it != end; ++it)
            return true;
        } // get_tokens

    protected:
        //
        // resynth
        //

        void resynth1(ScannerBase& base, token_container& c);
        void resynth2(token_container& c);
        void resynth3(token_iterator begin, token_iterator end);

        token_iterator
        resynth_typedef(token_iterator begin, token_iterator end);

        token_iterator resynth_parameter_list(
            token_iterator begin, token_iterator end);

        token_iterator skip_gnu_attribute(
            token_iterator begin, token_iterator end);

        token_iterator skip_asm_for_fn_decl(
            token_iterator begin, token_iterator end);

        void resynth4(token_container& c);
        void resynth5(token_iterator begin, token_iterator end);

        Token parse_identifier(const std::string& text) const;
        CR_ErrorInfo::Type
        parse_pragma(ScannerBase& base, token_iterator& it, token_iterator end);

    protected:
        shared_ptr<CR_ErrorInfo>    m_error_info;
        bool                        m_is_64bit;
        bool                        m_in_c_comment;
        bool                        m_in_cxx_comment;

        std::set<std::string>       m_type_names;

        //
        // pragma
        //
        void lib(const std::string str) { }

        void linker(const std::string str) { }

        void add_notice(const CR_Location& location, const std::string& text) {
            m_error_info.get()->add_notice(location, text);
        }
        void add_warning(const CR_Location& location, const std::string& text) {
            m_error_info.get()->add_warning(location, text);
        }
        void add_error(const CR_Location& location, const std::string& text) {
            m_error_info.get()->add_error(location, text);
        }
        void add(CR_ErrorInfo::Type type, const CR_Location& location,
                 const std::string& text)
        {
            m_error_info.get()->add(type, location, text);
        }

        void message(const CR_Location& location, const std::string& text) {
            add_notice(location, text);
        }

    private:
        // NOTE: Scanner is not copyable.
        Scanner(const Scanner&) = delete;
        Scanner& operator=(const Scanner&) = delete;
    }; // class Scanner
} // namespace cparser

#endif  // ndef CSCANNER_H_
