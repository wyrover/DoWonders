#include "stdafx.h"
#include "CParseHeader.h"

////////////////////////////////////////////////////////////////////////////

const char * const cr_logo =
    "///////////////////////////////////////////////\n"
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
# ifdef __GNUC__
    "// CParser sample 0.2.4 (64-bit) for gcc     //\n"
# elif defined(__clang__)
    "// CParser sample 0.2.4 (64-bit) for clang    //\n"
# elif defined(_MSC_VER)
    "// CParser sample 0.2.4 (64-bit) for cl      //\n"
# else
#  error You lose!
# endif
#else   // !64-bit
# ifdef __GNUC__
    "// CParser sample 0.2.4 (32-bit) for gcc     //\n"
# elif defined(__clang__)
    "// CParser sample 0.2.4 (32-bit) for clang    //\n"
# elif defined(_MSC_VER)
    "// CParser sample 0.2.4 (32-bit) for cl      //\n"
# else
#  error You lose!
# endif
#endif  // !64-bit
    "// Public Domain Software (PDS)              //\n"
    "// by Katayama Hirofumi MZ (katahiromz)      //\n"
    "// katayama.hirofumi.mz@gmail.com            //\n"
    "///////////////////////////////////////////////\n";

using namespace std;

////////////////////////////////////////////////////////////////////////////

// temporary file
static char *cr_tmpfile = NULL;

void CrDeleteTempFileAtExit(void) {
    if (cr_tmpfile) {
        std::remove(cr_tmpfile);
        cr_tmpfile = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////

using namespace cparser;

////////////////////////////////////////////////////////////////////////////
// cparser::Lexer

std::string cparser::Lexer::node_to_string(const node_type& node) const {
    std::string str = token_label(node.m_token);
    if (node.m_text.size()) {
        str += "(";
        str += node.m_text;
        str += ")";
    }
    return str;
}

void cparser::Lexer::skip_block_comment(
    LexerBase& base, node_container& infos)
{
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

void cparser::Lexer::skip_line_comment(
    LexerBase& base, node_container& infos)
{
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

bool cparser::Lexer::token_pattern_match(
    LexerBase& base, node_iterator it, node_iterator end,
    const std::vector<Token>& tokens) const
{
    node_iterator saved = it;
    for (const auto& token : tokens) {
        if (it == end) {
            it = saved;
        }
        if (it->m_token == T_NEWLINE) {
            base.location()++;
        } else if (token != eof && it->m_token != token) {
            it = saved;
            return false;
        }
        ++it;
    }
    it = saved;
    return true;
}

std::string
cparser::Lexer::guts_escape_sequence(
    str_iterator& it, str_iterator end) const
{
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

std::string
cparser::Lexer::guts_string(str_iterator& it, str_iterator end) const {
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

std::string
cparser::Lexer::guts_char(str_iterator& it, str_iterator end) const {
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

std::string
cparser::Lexer::guts_hex(str_iterator& it, str_iterator end) const {
    std::string result;
    for (; it != end; ++it) {
        if (isxdigit(*it)) {
            result += *it;
        } else {
            break;
        }
    }
    return result;
} // guts_hex

std::string
cparser::Lexer::guts_digits(str_iterator& it, str_iterator end) const {
    std::string result;
    for (; it != end; ++it) {
        if (isdigit(*it)) {
            result += *it;
        } else {
            break;
        }
    }
    return result;
} // guts_digits

std::string
cparser::Lexer::guts_octal(str_iterator& it, str_iterator end) const {
    std::string result;
    for (; it != end; ++it) {
        if ('0' <= *it && *it <= '7') {
            result += *it;
            if (result.size() == 3) {
                break;
            }
        } else {
            break;
        }
    }
    return result;
} // guts_octal

std::string
cparser::Lexer::guts_floating(str_iterator& it, str_iterator end) const {
    std::string result;
    if (it != end && *it == '.') {
        ++it;
        std::string digits = guts_digits(it, end);
    }
    return result;
} // guts_floating

std::string
cparser::Lexer::guts_indentifier(str_iterator& it, str_iterator end) const {
    std::string result;
    if (it != end && (isalpha(*it) || *it == '_')) {
        result += *it;
        for (++it; it != end; ++it) {
            if (isalnum(*it) || *it == '_') {
                result += *it;
            } else {
                break;
            }
        }
    }
    //std::puts(result.data());
    return result;
} // guts_indentifier

std::string
cparser::Lexer::guts_integer_suffix(
    str_iterator& it, str_iterator end, CR_TypeFlags& flags) const
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

std::string
cparser::Lexer::guts_floating_suffix(
    str_iterator& it, str_iterator end, CR_TypeFlags& flags) const
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

std::string
cparser::Lexer::guts_exponent(str_iterator& it, str_iterator end) const
{
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

bool cparser::Lexer::lexeme(
    str_iterator& it, str_iterator end, const std::string& str)
{
    str_iterator saved = it;
    std::size_t i = 0, siz = str.size();
    for (; i < siz && it != end; ++it, ++i) {
        if (*it != str[i]) {
            it = saved;
            return false;
        }
    }
    return i == siz;
} // lexeme

void cparser::Lexer::just_do_it(
    node_container& infos,
    scanner_iterator begin, scanner_iterator end)
{
    LexerBase base(begin, end);

    // get tokens
    node_container read_infos;
    while (get_tokens(base, read_infos)) {
        infos.insert(infos.end(), read_infos.begin(), read_infos.end());
        read_infos.clear();
    }
    infos.insert(infos.end(), read_infos.begin(), read_infos.end());
    read_infos.clear();
    infos.emplace_back(eof);

    // token resynthesization
    resynth(base, infos);

    m_type_names.clear();   // no use
} // just_do_it

std::string cparser::Lexer::get_line(LexerBase& base) {
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

bool
cparser::Lexer::get_tokens(LexerBase& base, node_container& infos) {
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
                --it;
                infos.emplace_back(T_STRING, text, extra);
            } else if (*it == '\'') {
                auto text = guts_char(it, end);
                --it;
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
                --it;
                infos.emplace_back(T_STRING, text, extra);
            } else if (*it == '\'') {
                auto text = guts_char(it, end);
                --it;
                infos.emplace_back(T_CONSTANT, text, extra);
            } else {
                --it;
                goto label_default;
            }
            break;

        case '"':
            {
                auto text = guts_string(it, end);
                --it;
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
                --it;
                infos.emplace_back(T_CONSTANT, "0x" + text, extra, flags);
            } else if (*it == '.') {
                // 0.
                auto text = guts_floating(it, end);
                CR_TypeFlags flags = TF_DOUBLE;
                extra = guts_floating_suffix(it, end, flags);
                --it;
                infos.emplace_back(T_CONSTANT, "0." + text, extra, flags);
            } else {
                // octal
                --it;
                auto text = guts_octal(it, end);
                CR_TypeFlags flags = 0;
                extra = guts_integer_suffix(it, end, flags);
                --it;
                infos.emplace_back(T_CONSTANT, text, extra, flags);
            }
            break;

        case '.':
            if (lexeme(it, end, "...")) {
                --it;
                infos.emplace_back(T_ELLIPSIS, "...");
            } else {
                // .
                auto text = guts_floating(it, end);
                if (text.size() > 1) {
                    CR_TypeFlags flags = TF_DOUBLE;
                    extra = guts_floating_suffix(it, end, flags);
                    --it;
                    infos.emplace_back(T_CONSTANT, "0." + text, extra, flags);
                } else {
                    --it;
                    infos.emplace_back(T_DOT, ".");
                }
            }
            break;

        case '<':
            if (lexeme(it, end, "<<=")) {
                --it;
                infos.emplace_back(T_L_SHIFT_ASSIGN, "<<=");
            } else if (lexeme(it, end, "<<")) {
                --it;
                infos.emplace_back(T_L_SHIFT, "<<");
            } else if (lexeme(it, end, "<=")) {
                --it;
                infos.emplace_back(T_LE, "<=");
            } else if (lexeme(it, end, "<:")) {
                --it;
                infos.emplace_back(T_L_BRACKET, "<:");
            } else {
                infos.emplace_back(T_LT, "<");
            }
            break;

        case '>':
            if (lexeme(it, end, ">>=")) {
                --it;
                infos.emplace_back(T_R_SHIFT_ASSIGN, ">>=");
            } else if (lexeme(it, end, ">>")) {
                --it;
                infos.emplace_back(T_R_SHIFT, ">>");
            } else if (lexeme(it, end, ">=")) {
                --it;
                infos.emplace_back(T_GE, ">=");
            } else {
                infos.emplace_back(T_GT, ">");
            }
            break;

        case '+':
            if (lexeme(it, end, "+=")) {
                --it;
                infos.emplace_back(T_ADD_ASSIGN, "+=");
            } else if (lexeme(it, end, "++")) {
                --it;
                infos.emplace_back(T_INC, "++");
            } else {
                infos.emplace_back(T_PLUS, "+");
            }
            break;

        case '-':
            if (lexeme(it, end, "-=")) {
                --it;
                infos.emplace_back(T_SUB_ASSIGN, "-=");
            } else if (lexeme(it, end, "--")) {
                --it;
                infos.emplace_back(T_DEC, "--");
            } else if (lexeme(it, end, "->")) {
                --it;
                infos.emplace_back(T_ARROW, "->");
            } else {
                infos.emplace_back(T_MINUS, "-");
            }
            break;

        case '*':
            if (lexeme(it, end, "*=")) {
                --it;
                infos.emplace_back(T_MUL_ASSIGN, "*=");
            } else {
                infos.emplace_back(T_ASTERISK, "*");
            }
            break;

        case '/':
            if (lexeme(it, end, "/*")) {    // */
                --it;
                m_in_c_comment = true;
                infos.emplace_back(T_C_COMMENT_BEGIN, "/*");    // */
                skip_block_comment(base, infos);
            } else if (lexeme(it, end, "//")) {
                --it;
                m_in_cxx_comment = true;
                infos.emplace_back(T_CPP_COMMENT, "//");
                skip_line_comment(base, infos);
            } else if (lexeme(it, end, "/=")) {
                --it;
                infos.emplace_back(T_DIV_ASSIGN, "/=");
            } else {
                infos.emplace_back(T_SLASH, "/");
            }
            break;

        case '%':
            if (lexeme(it, end, "%=")) {
                --it;
                infos.emplace_back(T_MOD_ASSIGN, "%=");
            } else if (lexeme(it, end, "%>")) {
                --it;
                infos.emplace_back(T_R_BRACE, "%>");
            } else {
                infos.emplace_back(T_PERCENT, "%");
            }
            break;

        case '&':
            if (lexeme(it, end, "&=")) {
                --it;
                infos.emplace_back(T_AND_ASSIGN, "&=");
            } else if (lexeme(it, end, "&&")) {
                --it;
                infos.emplace_back(T_L_AND, "&&");
            } else {
                infos.emplace_back(T_AND, "&");
            }
            break;
            
        case '^':
            if (lexeme(it, end, "^=")) {
                --it;
                infos.emplace_back(T_XOR_ASSIGN, "^=");
            } else {
                infos.emplace_back(T_XOR, "^");
            }
            break;

        case '|':
            if (lexeme(it, end, "|=")) {
                --it;
                infos.emplace_back(T_OR_ASSIGN, "|=");
            } else if (lexeme(it, end, "||")) {
                --it;
                infos.emplace_back(T_L_OR, "||");
            } else {
                infos.emplace_back(T_OR, "|");
            }
            break;

        case '=':
            if (lexeme(it, end, "==")) {
                --it;
                infos.emplace_back(T_EQUAL, "==");
            } else {
                infos.emplace_back(T_ASSIGN, "=");
            }
            break;

        case '!':
            if (lexeme(it, end, "!=")) {
                --it;
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
                --it;
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
                    --it;
                    infos.emplace_back(T_CONSTANT, text, extra, flags);
                } else {
                    auto exponent = guts_exponent(it, end);
                    if (exponent.size()) {
                        // exponent was found. it's a floating
                        CR_TypeFlags flags = TF_DOUBLE;
                        extra = guts_floating_suffix(it, end, flags);
                        --it;
                        auto text = digits + exponent + extra;
                        infos.emplace_back(T_CONSTANT, text, extra, flags);
                    } else {
                        // exponent not found. it's a integer
                        CR_TypeFlags flags = 0;
                        extra = guts_integer_suffix(it, end, flags);
                        --it;
                        infos.emplace_back(T_CONSTANT, digits, extra, flags);
                    }
                }
            } else if (isalpha(*it) || *it == '_') {
                // identifier or keyword
                auto text = guts_indentifier(it, end);
                --it;
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

void cparser::Lexer::resynth(LexerBase& base, node_container& c) {
    const bool c_show_tokens = false;

    if (c_show_tokens) {
        std::printf("\n#0\n");
        show_tokens(c.begin(), c.end());
        std::printf("\n--------------\n");
        fflush(stdout);
    }

    resynth1(base, c);

    if (c_show_tokens) {
        std::printf("\n#1\n");
        show_tokens(c.begin(), c.end());
        std::printf("\n--------------\n");
        fflush(stdout);
    }

    resynth2(c);
    resynth3(c.begin(), c.end());
    resynth4(c);
    resynth5(c.begin(), c.end());
}

void cparser::Lexer::resynth1(LexerBase& base, node_container& c) {
    node_container     newc;
    newc.reserve(c.size());

    bool    line_top = true;
    auto    end = c.end();
    for (auto it = c.begin(); it != end; ++it) {
        it->location() = base.location();
        it->m_pack = base.packing();

        if (it->m_token == T_INVALID_CHAR) {
            std::string text = "unexpected character '";
            text += it->m_text + "'";
            add_error(base.location(), text);
            continue;
        }

        if (it->m_token == T_SHARP) {
            if (!line_top) {
                add_error(base.location(), "invalid character '#'");
                continue;
            }

            // #
            bool is_lineno_directive = false;
            ++it;

            // #lineno "file"
            // #lineno
            // #line lineno "file"
            // #line lineno
            if (it != end && it->m_text == "line") {
                ++it;
                is_lineno_directive = true;
            }
            if (it != end && it->m_token == T_CONSTANT) {
                int lineno = std::atoi(it->m_text.data()) - 1;
                ++it;
                if (it != end && it->m_token == T_STRING) {
                    base.location().set(it->m_text, lineno);
                } else {
                    --it;
                    base.location().m_line = lineno;
                }
                is_lineno_directive = true;
            }

            if (!is_lineno_directive && it != end && it->m_text == "pragma") {
                // #pragma name("...")
                ++it;
                parse_pragma(base, it, end);
                //add_message(type, base.location(), "unknown pragma found");
            }

            // up to new line
            while (it != end) {
                if (it->m_token == T_NEWLINE) {
                    line_top = true;
                    ++base.location();
                    break;
                }
                ++it;
            }
            continue;
        }
        if (it->m_token == T_NEWLINE) {
            line_top = true;
            ++base.location();
        } else {
            line_top = false;
            newc.emplace_back(*it);
        }
    }
    std::swap(c, newc);
}

void cparser::Lexer::resynth2(node_container& c) {
    node_container newc;
    node_iterator it, it2, end = c.end();
    for (it = c.begin(); it != end; ++it) {
        if (it->m_token == T_R_PAREN && (it + 1)->m_token == T_ASM) {
            // int func() __asm__("..." "...");
            newc.push_back(*it);
            ++it;
            skip_paren_block(it, end);
            if (it == end)
                break;
        }

        if (it->m_token == T_GNU_ATTRIBUTE) {
            ++it;
            skip_paren_block(it, end);
        } else if (it->m_token == T_DECLSPEC || it->m_token == T_PRAGMA) {
            it2 = it;
            ++it2;
            bool f = false;
            int paren_nest = 0;
            for (; it2 != end; ++it2) {
                if (it2->m_token == T_L_PAREN) {
                    f = true;
                    paren_nest++;
                } else if (it2->m_token == T_R_PAREN) {
                    paren_nest--;
                    if (paren_nest == 0)
                        break;
                }
            }
            if (f)
                it = it2;
        } else {
            newc.push_back(*it);
        }
    }
    std::swap(c, newc);
} // resynth2

void cparser::Lexer::resynth3(node_iterator begin, node_iterator end) {
    m_type_names.clear();
    #ifdef __GNUC__
        m_type_names.insert("__builtin_va_list");   // fixup
    #else
        m_type_names.insert("va_list");
        m_type_names.insert("SOCKADDR_STORAGE");    // fixup
    #endif

    for (node_iterator it = begin; it != end; ++it) {
        if (it->m_token == T_ENUM || it->m_token == T_STRUCT ||
            it->m_token == T_UNION)
        {
            ++it;
            if (it->m_token == T_IDENTIFIER) {
                it->set_token(T_TAGNAME);
                if ((it + 1)->m_token == T_SEMICOLON) {
                    // struct tag_name; fixup
                    m_type_names.insert(it->m_text);
                }
            }
        }
    }

    for (node_iterator it = begin; it != end; ++it) {
        if (it->m_token == T_TYPEDEF) {
            it = resynth_typedef(++it, end);
        } else if (it->m_token == T_IDENTIFIER) {
            if (m_type_names.count(it->m_text)) {
                it->set_token(T_TYPEDEF_NAME);
            }
        }
    }
} // resynth3

cparser::node_iterator
cparser::Lexer::resynth_typedef(node_iterator begin, node_iterator end) {
    int paren_nest = 0, brace_nest = 0, bracket_nest = 0;
    node_iterator it;
    for (it = begin; it != end; ++it) {
        if (brace_nest == 0 && it->m_token == T_SEMICOLON)
            break;
        else if (it->m_token == T_L_BRACE)
            brace_nest++;
        else if (it->m_token == T_R_BRACE)
            brace_nest--;
        else if (it->m_token == T_L_BRACKET)
            bracket_nest++;
        else if (it->m_token == T_R_BRACKET)
            bracket_nest--;
        else if (it->m_token == T_L_PAREN)
            paren_nest++;
        else if (it->m_token == T_R_PAREN) {
            paren_nest--;
            ++it;
            if (it->m_token == T_L_PAREN) {
                it = resynth_parameter_list(++it, end);
            } else {
                --it;
            }
        } else if (it->m_token == T_IDENTIFIER) {
            if (brace_nest == 0 && bracket_nest == 0) {
                if (m_type_names.count(it->m_text)) {
                    ++it;
                    if (it->m_token == T_SEMICOLON || it->m_token == T_R_PAREN ||
                        it->m_token == T_L_BRACKET || it->m_token == T_COMMA)
                    {
                        --it;
                        it->set_token(T_TYPEDEF_TAG);
                    } else {
                        --it;
                        it->set_token(T_TYPEDEF_NAME);
                    }
                } else {
                    it->set_token(T_TYPEDEF_TAG);
                    m_type_names.insert(it->m_text);

                    ++it;
                    if (it->m_token == T_L_PAREN) {
                        it = resynth_parameter_list(++it, end);
                    } else
                        --it;
                }
            } else if (m_type_names.count(it->m_text)) {
                it->set_token(T_TYPEDEF_NAME);
            }
        }
    }
    return it;
} // resynth_typedef

cparser::node_iterator
cparser::Lexer::resynth_parameter_list(
    node_iterator begin, node_iterator end)
{
    int paren_nest = 1;
    bool fresh = true;
    node_iterator it;
    for (it = begin; it != end; ++it) {
        if (it->m_token == T_SEMICOLON)
            break;
        else if (it->m_token == T_L_PAREN) {
            paren_nest++;
            fresh = true;
        } else if (it->m_token == T_R_PAREN) {
            paren_nest--;
            if (paren_nest == 0)
                break;

            ++it;
            if (it->m_token == T_L_PAREN) {
                it = resynth_parameter_list(++it, end);
            } else
                --it;
        } else if (it->m_token == T_IDENTIFIER) {
            if (m_type_names.count(it->m_text)) {
                ++it;
                if (fresh) {
                    --it;
                    it->set_token(T_TYPEDEF_NAME);
                } else {
                    --it;
                }
            }
            fresh = false;
        } else if (it->m_token == T_COMMA) {
            fresh = true;
        }
    }
    return it;
} // resynth_parameter_list

void cparser::Lexer::skip_paren_block(
    node_iterator& begin, node_iterator end)
{
    if (begin != end && begin->m_token == T_L_PAREN) {
        ++begin;
        int paren_nest = 1;
        for (; begin != end; ++begin) {
            if (begin->m_token == T_L_PAREN) {
                paren_nest++;
            } else if (begin->m_token == T_R_PAREN) {
                paren_nest--;
                if (paren_nest == 0)
                    break;
            }
        }
    }
}

void cparser::Lexer::resynth4(node_container& c) {
    node_container newc;
    node_iterator it, it2, end = c.end();
    for (it = c.begin(); it != end; ++it) {
        switch (it->m_token) {
        case T_CDECL: case T_STDCALL: case T_FASTCALL:
            // TODO: do calling convention
            if ((it + 1)->m_token == T_ASTERISK) {
                if (it->m_token == T_CDECL)
                    (it + 1)->m_flags |= TF_CDECL;
                else if (it->m_token == T_STDCALL)
                    (it + 1)->m_flags |= TF_STDCALL;
                else if (it->m_token == T_FASTCALL)
                    (it + 1)->m_flags |= TF_FASTCALL;
            } else if ((it + 1)->m_token == T_IDENTIFIER ||
                       (it + 1)->m_token == T_TYPEDEF_NAME) {
                if (it->m_token == T_CDECL)
                    (it + 1)->m_flags |= TF_CDECL;
                else if (it->m_token == T_STDCALL)
                    (it + 1)->m_flags |= TF_STDCALL;
                else if (it->m_token == T_FASTCALL)
                    (it + 1)->m_flags |= TF_FASTCALL;
            }
            break;

        case T_L_BRACKET:
            it2 = it;
            ++it2;
            if (it2->m_token == T_IDENTIFIER ||
                it2->m_token == T_TYPEDEF_NAME)
            {
                bool f =
                    (it2->m_text == "returnvalue") ||
                    (it2->m_text == "SA_Pre") ||
                    (it2->m_text == "SA_Post") ||
                    (it2->m_text == "SA_FormatString") ||
                    (it2->m_text == "source_annotation_attribute");
                if (f) {
                    int paren_nest = 0;
                    f = false;
                    for (; it2 != end; ++it2) {
                        if (it2->m_token == T_L_PAREN) {
                            f = true;
                            paren_nest++;
                        } else if (it2->m_token == T_R_PAREN) {
                            paren_nest--;
                            if (paren_nest == 0)
                                break;
                        }
                    }
                    if (f) {
                        f = false;
                        if (it2 != end) {
                            ++it2;
                            if (it2->m_token == T_R_BRACKET) {
                                it = it2;
                                f = true;
                            }
                        }
                    }
                }
                if (!f)
                    newc.push_back(*it);
            } else {
                newc.push_back(*it);
            }
            break;

        case T_GNU_EXTENSION:
            break;

        default:
            newc.push_back(*it);
        }
    }
    std::swap(c, newc);
} // resynth4

void cparser::Lexer::resynth5(node_iterator begin, node_iterator end) {
    node_iterator it, paren_it, it2;
    int paren_nest = 0;
    for (it = begin; it != end; ++it) {
        if (it->m_token == T_L_PAREN) {
            paren_it = it;
            paren_nest++;
        } else if (it->m_token == T_R_PAREN) {
            paren_nest--;
        } else if (paren_nest >= 1 &&
            it->m_token == T_TYPEDEF_NAME &&
            ((it + 1)->m_token == T_R_PAREN || (it + 1)->m_token == T_COMMA))
        {
            it2 = it - 1;
            while (it2 != paren_it) {
                switch (it2->m_token) {
                case T_VOID: case T_CHAR: case T_SHORT: case T_INT:
                case T_INT32: case T_INT64: case T_INT128: case T_LONG:
                case T_FLOAT: case T_DOUBLE:
                case T_SIGNED: case T_UNSIGNED: case T_BOOL:
                case T_TYPEDEF_NAME: case T_TAGNAME:
                case T_ASTERISK:
                    it->m_token = T_IDENTIFIER;
                    break;

                case T_CONST:
                    --it2;
                    continue;

                default:
                    break;
                }
                break;
            }
        }
    }
} // resynth5

void cparser::Lexer::init_symbol_table() {
    auto& st = m_symbol_table;
    st.clear();
    st.reserve(76);
    st.emplace("_Alignas", T_ALIGNAS);
    st.emplace("_Alignof", T_ALIGNOF);
    st.emplace("_Atomic", T_ATOMIC);
    st.emplace("_Bool", T_BOOL);
    st.emplace("_Complex", T_COMPLEX);
    st.emplace("_Generic", T_GENERIC);
    st.emplace("_Imaginary", T_IMAGINARY);
    st.emplace("_Noreturn", T_NORETURN);
    st.emplace("_Static_assert", T_STATIC_ASSERT);
    st.emplace("_Thread_local", T_THREAD_LOCAL);
    st.emplace("__asm", T_ASM);
    st.emplace("__asm__", T_ASM);
    st.emplace("__attribute__", T_GNU_ATTRIBUTE);
    st.emplace("__cdecl", T_CDECL);
    st.emplace("__cdecl__", T_CDECL);
    st.emplace("__const__", T_CONST);
    st.emplace("__declspec", T_DECLSPEC);
    st.emplace("__extension__", T_GNU_EXTENSION);
    st.emplace("__fastcall", T_FASTCALL);
    st.emplace("__fastcall__", T_FASTCALL);
    st.emplace("__forceinline", T_FORCEINLINE);
    st.emplace("__inline", T_INLINE);
    st.emplace("__inline__", T_INLINE);
    st.emplace("__int32", T_INT32);
    st.emplace("__int64", T_INT64);
    st.emplace("__int128", T_INT128);
    st.emplace("__noreturn__", T_NORETURN);
    st.emplace("__nothrow__", T_NOTHROW);
    st.emplace("__pragma", T_PRAGMA);
    st.emplace("__ptr32", T_PTR32);
    st.emplace("__ptr64", T_PTR64);
    st.emplace("__restrict", T_RESTRICT);
    st.emplace("__restrict__", T_RESTRICT);
    st.emplace("__signed__", T_SIGNED);
    st.emplace("__stdcall", T_STDCALL);
    st.emplace("__stdcall__", T_STDCALL);
    st.emplace("__unaligned", T_UNALIGNED);
    st.emplace("__volatile__", T_VOLATILE);
    st.emplace("__w64", T_W64);
    st.emplace("asm", T_ASM);
    st.emplace("auto", T_AUTO);
    st.emplace("break", T_BREAK);
    st.emplace("case", T_CASE);
    st.emplace("char", T_CHAR);
    st.emplace("const", T_CONST);
    st.emplace("continue", T_CONTINUE);
    st.emplace("default", T_DEFAULT);
    st.emplace("do", T_DO);
    st.emplace("double", T_DOUBLE);
    st.emplace("else", T_ELSE);
    st.emplace("enum", T_ENUM);
    st.emplace("extern", T_EXTERN);
    st.emplace("float", T_FLOAT);
    st.emplace("for", T_FOR);
    st.emplace("goto", T_GOTO);
    st.emplace("if", T_IF);
    st.emplace("inline", T_INLINE);
    st.emplace("int", T_INT);
    st.emplace("long", T_LONG);
    st.emplace("noreturn", T_NORETURN);
    st.emplace("register", T_REGISTER);
    st.emplace("restrict", T_RESTRICT);
    st.emplace("return", T_RETURN);
    st.emplace("short", T_SHORT);
    st.emplace("signed", T_SIGNED);
    st.emplace("sizeof", T_SIZEOF);
    st.emplace("static", T_STATIC);
    st.emplace("struct", T_STRUCT);
    st.emplace("switch", T_SWITCH);
    st.emplace("typedef", T_TYPEDEF);
    st.emplace("union", T_UNION);
    st.emplace("unsigned", T_UNSIGNED);
    st.emplace("void", T_VOID);
    st.emplace("volatile", T_VOLATILE);
    st.emplace("while", T_WHILE);
}

cparser::Token
cparser::Lexer::parse_identifier(const std::string& text) const {
    const auto& st = m_symbol_table;
    auto it = st.find(text);
    auto end = st.end();
    if (it == end) {
        return T_IDENTIFIER;
    } else {
        return it->second;
    }
} // parse_identifier

CR_ErrorInfo::Type
cparser::Lexer::parse_pack(
    LexerBase& base, node_iterator it, node_iterator end)
{
    const bool c_show_pack = false;
    bool flag;

    // #pragma pack...
    flag = token_pattern_match(base, it, end, {T_L_PAREN, T_R_PAREN});
    if (flag) {
        // #pragma pack()
        base.packing().set(base.default_packing());
        return CR_ErrorInfo::NOTHING;
    }

    // #pragma pack(...)
	flag = token_pattern_match(base, it, end, { T_L_PAREN, eof, T_R_PAREN });
    if (flag) {
        if ((it + 1)->m_text == "pop") {
            // #pragma pack(pop)
            if (c_show_pack)
                fprintf(stderr, "%s: pragma pack(pop)\n",
                    base.location().str().data());
            base.packing().pop();
        } else if ((it + 1)->m_text == "push"){
            // #pragma pack(push)
            if (c_show_pack)
                fprintf(stderr, "%s: pragma pack(push)\n",
                    base.location().str().data());
            base.packing().push(base.packing());
        } else {
            // #pragma pack(#)
            int pack = std::strtol((it + 1)->m_text.data(), NULL, 0);
            if (c_show_pack)
                fprintf(stderr, "%s: pragma pack(%d)\n",
                    base.location().str().data(), pack);
            base.packing().set(pack);
        }
        return CR_ErrorInfo::NOTHING;
    }

    // #pragma pack(..., ...)
	flag = token_pattern_match(base, it, end,
        {T_L_PAREN, eof, T_COMMA, eof, T_R_PAREN}
    );
    if (flag) {
        auto param = (it + 3)->m_text;
        if (param.size() && (isalpha(param[0]) || param[0] == '_')) {
            // #pragma pack(..., ident)
            if ((it + 1)->m_text == "pop") {
                // #pragma pack(pop, ident)
                if (c_show_pack)
                    fprintf(stderr, "%s: pragma pack(pop,%s)\n",
                        base.location().str().data(), param.data());
                base.packing().pop(param);
                return CR_ErrorInfo::NOTHING;
            } else if ((it + 1)->m_text == "push") {
                // #pragma pack(push, ident)
                if (c_show_pack)
                    fprintf(stderr, "%s: pragma pack(push,%s)\n",
                        base.location().str().data(), param.data());
                base.packing().push(param);
                return CR_ErrorInfo::NOTHING;
            }
        } else if (param.size() && isdigit(param[0])) {
            // #pragma pack(..., #)
            if ((it + 1)->m_text == "pop") {
                // #pragma pack(pop, #)
                int pack = std::strtol(param.data(), NULL, 0);
                if (c_show_pack)
                    fprintf(stderr, "%s: pragma pack(pop,%d)\n",
                        base.location().str().data(), pack);
                base.packing().pop(pack);
                return CR_ErrorInfo::NOTHING;
            } else if ((it + 1)->m_text == "push") {
                // #pragma pack(push, #)
                int pack = std::strtol(param.data(), NULL, 0);
                if (c_show_pack)
                    fprintf(stderr, "%s: pragma pack(push,%d)\n",
                        base.location().str().data(), pack);
                base.packing().push(pack);
                return CR_ErrorInfo::NOTHING;
            }
        }
        return CR_ErrorInfo::WARN;
    }

    // #pragma pack(..., ..., ...)
	flag = token_pattern_match(base, it, end,
        {T_L_PAREN, eof, T_COMMA, eof, T_COMMA, eof, T_R_PAREN}
    );
    if (flag) {
        // #pragma pack(push, ...   , #)
        auto op = (it + 1)->m_text;
        auto ident = (it + 3)->m_text;
        auto param = (it + 5)->m_text;
        if (op == "push") {
            if (c_show_pack)
                fprintf(stderr, "%s: pragma pack(push,%s,%s)\n",
                    base.location().str().data(),
                    ident.data(), param.data());
            base.packing().push(ident);
            base.packing().set(base.packing());
            return CR_ErrorInfo::NOTHING;
        } else if (op == "pop") {
            int pack = std::strtol(param.data(), NULL, 0);
            if (c_show_pack)
                fprintf(stderr, "%s: pragma pack(pop,%s,%s)\n",
                    base.location().str().data(),
                    ident.data(), param.data());
            if (base.packing().pop(ident)) {
                base.packing().set(pack);
            } else {
                base.packing().set(base.default_packing());
            }
            return CR_ErrorInfo::NOTHING;
        }
    }

    return CR_ErrorInfo::WARN;
}

CR_ErrorInfo::Type
cparser::Lexer::parse_pragma(
    LexerBase& base, node_iterator it, node_iterator end)
{
    if (it == end) {
        return CR_ErrorInfo::NOTHING;
    }
    // #pragma name...
    bool flag;
    auto name = it->m_text;
    ++it;
    if (name == "message") {
        // #pragma message("...")
        flag = token_pattern_match(base, it, end, 
			{ T_L_PAREN, T_STRING, T_R_PAREN }
		);
        if (flag) {
            message(base.location(), (it + 1)->m_text);
            it += 3;
        }
        return CR_ErrorInfo::NOTHING;
    }
    if (name == "pack") {
        return parse_pack(base, it, end);
    }
    if (name == "comment") {
        flag = token_pattern_match(base, it, end,
            {T_L_PAREN, eof, T_COMMA, eof, T_R_PAREN}
        );
        if (flag) {
            if ((it + 1)->m_text == "lib") {
                // #pragma comment(lib, "...")
                lib((it + 3)->m_text);
            } else if ((it + 1)->m_text == "linker") {
                // #pragma comment(linker, "...")
                linker((it + 3)->m_text);
            }
            it += 5;
            return CR_ErrorInfo::NOTHING;
        }
    }
    return CR_ErrorInfo::WARN;
} // parse_pragma

////////////////////////////////////////////////////////////////////////////
// CrCalcConstInt...Expr functions

int CrCalcConstIntPrimExpr(CR_NameScope& namescope, PrimExpr *pe);
int CrCalcConstIntPostfixExpr(CR_NameScope& namescope, PostfixExpr *pe);
int CrCalcConstIntUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue);
int CrCalcConstIntCastExpr(CR_NameScope& namescope, CastExpr *ce);
int CrCalcConstIntMulExpr(CR_NameScope& namescope, MulExpr *me);
int CrCalcConstIntAddExpr(CR_NameScope& namescope, AddExpr *ae);
int CrCalcConstIntShiftExpr(CR_NameScope& namescope, ShiftExpr *se);
int CrCalcConstIntRelExpr(CR_NameScope& namescope, RelExpr *re);
int CrCalcConstIntEqualExpr(CR_NameScope& namescope, EqualExpr *ee);
int CrCalcConstIntAndExpr(CR_NameScope& namescope, AndExpr *ae);
int CrCalcConstIntExclOrExpr(CR_NameScope& namescope, ExclOrExpr *eoe);
int CrCalcConstIntInclOrExpr(CR_NameScope& namescope, InclOrExpr *ioe);
int CrCalcConstIntLogAndExpr(CR_NameScope& namescope, LogAndExpr *lae);
int CrCalcConstIntLogOrExpr(CR_NameScope& namescope, LogOrExpr *loe);
int CrCalcConstIntAssignExpr(CR_NameScope& namescope, AssignExpr *ae);
int CrCalcConstIntExpr(CR_NameScope& namescope, Expr *e);
int CrCalcConstIntCondExpr(CR_NameScope& namescope, CondExpr *ce);

int CrCalcConstIntPrimExpr(CR_NameScope& namescope, PrimExpr *pe) {
    int n;
    switch (pe->m_prim_type) {
    case PrimExpr::IDENTIFIER:
        n = namescope.GetIntValueFromVarName(pe->m_text);
        return n;

    case PrimExpr::F_CONSTANT:
        return 0;

    case PrimExpr::I_CONSTANT:
        n = std::strtol(pe->m_text.data(), NULL, 0);
        return n;

    case PrimExpr::STRING:
        return 1;

    case PrimExpr::PAREN:
        n = CrCalcConstIntExpr(namescope, pe->m_expr.get());
        return n;

    case PrimExpr::SELECTION:
        // TODO:
        break;

    default:
        assert(0);
    }
    return 0;
}

int CrCalcConstIntPostfixExpr(CR_NameScope& namescope, PostfixExpr *pe) {
    int n;
    switch (pe->m_postfix_type) {
    case PostfixExpr::SINGLE:
        n = CrCalcConstIntPrimExpr(namescope, pe->m_prim_expr.get());
        return n;

    case PostfixExpr::ARRAYITEM:
        //pe->m_postfix_expr
        return 0;

    case PostfixExpr::FUNCCALL1:
        //pe->m_postfix_expr
        return 0;

    case PostfixExpr::FUNCCALL2:
        //pe->m_postfix_expr
        return 0;

    case PostfixExpr::DOT:
        //pe->m_postfix_expr
        return 0;

    case PostfixExpr::ARROW:
        //pe->m_postfix_expr
        return 0;

    case PostfixExpr::INC:
        n = CrCalcConstIntPostfixExpr(namescope, pe->m_postfix_expr.get());
        return n;

    case PostfixExpr::DEC:
        n = CrCalcConstIntPostfixExpr(namescope, pe->m_postfix_expr.get());
        return n;

    default:
        assert(0);
    }
    return 0;
}

int CrCalcSizeOfUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue) {
    return 0;
}

CR_TypeID CrAnalyseDeclSpecs(CR_NameScope& namescope, DeclSpecs *ds);

size_t CrCalcSizeOfTypeName(CR_NameScope& namescope, TypeName *tn) {
    CR_TypeID tid = CrAnalyseDeclSpecs(namescope, tn->m_decl_specs.get());
    if (tn->m_declor) {
        switch (tn->m_declor->m_declor_type) {
        case Declor::POINTERS:
        case Declor::FUNCTION:
            return (namescope.Is64Bit() ? 8 : 4);

        case Declor::ARRAY:
            {
                int count = CrCalcConstIntCondExpr(
                    namescope, tn->m_declor->m_const_expr.get());
                return namescope.SizeOfType(tid) * count;
            }

        case Declor::BITS:
            return 0;

        default:
            break;
        }
    }
    return namescope.SizeOfType(tid);
}

int CrCalcConstIntUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue) {
    int n;
    switch (ue->m_unary_type) {
    case UnaryExpr::SINGLE:
        n = CrCalcConstIntPostfixExpr(namescope, ue->m_postfix_expr.get());
        return n;

    case UnaryExpr::INC:
        n = CrCalcConstIntUnaryExpr(namescope, ue->m_unary_expr.get());
        return ++n;

    case UnaryExpr::DEC:
        n = CrCalcConstIntUnaryExpr(namescope, ue->m_unary_expr.get());
        return --n;

    case UnaryExpr::AND:
        return 0;

    case UnaryExpr::ASTERISK:
        return 0;

    case UnaryExpr::PLUS:
        n = CrCalcConstIntCastExpr(namescope, ue->m_cast_expr.get());
        return n;

    case UnaryExpr::MINUS:
        n = CrCalcConstIntCastExpr(namescope, ue->m_cast_expr.get());
        return n;

    case UnaryExpr::BITWISE_NOT:
        n = CrCalcConstIntCastExpr(namescope, ue->m_cast_expr.get());
        return ~n;

    case UnaryExpr::NOT:
        n = CrCalcConstIntCastExpr(namescope, ue->m_cast_expr.get());
        return !n;

    case UnaryExpr::SIZEOF1:
        n = static_cast<int>(CrCalcSizeOfUnaryExpr(namescope, ue->m_unary_expr.get()));
        return n;

    case UnaryExpr::SIZEOF2:
        n = static_cast<int>(CrCalcSizeOfTypeName(namescope, ue->m_type_name.get()));
        return n;

    default:
        assert(0);
    }
    return 0;
}

int CrCalcConstIntCastExpr(CR_NameScope& namescope, CastExpr *ce) {
    int result = 0;
    switch (ce->m_cast_type) {
    case CastExpr::UNARY:
        result = CrCalcConstIntUnaryExpr(namescope, ce->m_unary_expr.get());
        break;
    
    case CastExpr::INITERLIST:
        // TODO:
        //ce->m_type_name
        //ce->m_initer_list
        break;

    case CastExpr::CAST:
        //ce->m_type_name
        result = CrCalcConstIntCastExpr(namescope, ce->m_cast_expr.get());
        break;

    default:
        assert(0);
    }
    return result;
}

int CrCalcConstIntMulExpr(CR_NameScope& namescope, MulExpr *me) {
    int n1, n2, result = 0;
    switch (me->m_mul_type) {
    case MulExpr::SINGLE:
        result = CrCalcConstIntCastExpr(namescope, me->m_cast_expr.get());
        break;

    case MulExpr::ASTERISK:
        n1 = CrCalcConstIntMulExpr(namescope, me->m_mul_expr.get());
        n2 = CrCalcConstIntCastExpr(namescope, me->m_cast_expr.get());
        result = (n1 * n2);
        break;

    case MulExpr::SLASH:
        n1 = CrCalcConstIntMulExpr(namescope, me->m_mul_expr.get());
        n2 = CrCalcConstIntCastExpr(namescope, me->m_cast_expr.get());
        result = (n1 / n2);
        break;

    case MulExpr::PERCENT:
        n1 = CrCalcConstIntMulExpr(namescope, me->m_mul_expr.get());
        n2 = CrCalcConstIntCastExpr(namescope, me->m_cast_expr.get());
        result = (n1 % n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CrCalcConstIntAddExpr(CR_NameScope& namescope, AddExpr *ae) {
    int n1, n2, result = 0;
    switch (ae->m_add_type) {
    case AddExpr::SINGLE:
        result = CrCalcConstIntMulExpr(namescope, ae->m_mul_expr.get());
        break;

    case AddExpr::PLUS:
        n1 = CrCalcConstIntAddExpr(namescope, ae->m_add_expr.get());
        n2 = CrCalcConstIntMulExpr(namescope, ae->m_mul_expr.get());
        result = (n1 + n2);
        break;

    case AddExpr::MINUS:
        n1 = CrCalcConstIntAddExpr(namescope, ae->m_add_expr.get());
        n2 = CrCalcConstIntMulExpr(namescope, ae->m_mul_expr.get());
        result = (n1 - n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CrCalcConstIntShiftExpr(CR_NameScope& namescope, ShiftExpr *se) {
    int n1, n2, result = 0;
    switch (se->m_shift_type) {
    case ShiftExpr::SINGLE:
        result = CrCalcConstIntAddExpr(namescope, se->m_add_expr.get());
        break;

    case ShiftExpr::L_SHIFT:
        n1 = CrCalcConstIntShiftExpr(namescope, se->m_shift_expr.get());
        n2 = CrCalcConstIntAddExpr(namescope, se->m_add_expr.get());
        result = (n1 << n2);
        break;

    case ShiftExpr::R_SHIFT:
        n1 = CrCalcConstIntShiftExpr(namescope, se->m_shift_expr.get());
        n2 = CrCalcConstIntAddExpr(namescope, se->m_add_expr.get());
        result = (n1 >> n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CrCalcConstIntRelExpr(CR_NameScope& namescope, RelExpr *re) {
    int n1, n2, result = 0;
    switch (re->m_rel_type) {
    case RelExpr::SINGLE:
        result = CrCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        break;

    case RelExpr::LT:
        n1 = CrCalcConstIntRelExpr(namescope, re->m_rel_expr.get());
        n2 = CrCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        result = (n1 < n2);
        break;

    case RelExpr::GT:
        n1 = CrCalcConstIntRelExpr(namescope, re->m_rel_expr.get());
        n2 = CrCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        result = (n1 > n2);
        break;

    case RelExpr::LE:
        n1 = CrCalcConstIntRelExpr(namescope, re->m_rel_expr.get());
        n2 = CrCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        result = (n1 <= n2);
        break;

    case RelExpr::GE:
        n1 = CrCalcConstIntRelExpr(namescope, re->m_rel_expr.get());
        n2 = CrCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        result = (n1 >= n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CrCalcConstIntEqualExpr(CR_NameScope& namescope, EqualExpr *ee) {
    int n1, n2, result = 0;
    switch (ee->m_equal_type) {
    case EqualExpr::SINGLE:
        result = CrCalcConstIntRelExpr(namescope, ee->m_rel_expr.get());
        break;

    case EqualExpr::EQUAL:
        n1 = CrCalcConstIntEqualExpr(namescope, ee->m_equal_expr.get());
        n2 = CrCalcConstIntRelExpr(namescope, ee->m_rel_expr.get());
        result = (n1 == n2);
        break;

    case EqualExpr::NE:
        n1 = CrCalcConstIntEqualExpr(namescope, ee->m_equal_expr.get());
        n2 = CrCalcConstIntRelExpr(namescope, ee->m_rel_expr.get());
        result = (n1 != n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CrCalcConstIntAndExpr(CR_NameScope& namescope, AndExpr *ae) {
    int result = CrCalcConstIntEqualExpr(namescope, (*ae)[0].get());
    for (std::size_t i = 1; i < ae->size(); ++i) {
        result &= CrCalcConstIntEqualExpr(namescope, (*ae)[i].get());
    }
    return result;
}

int CrCalcConstIntExclOrExpr(CR_NameScope& namescope, ExclOrExpr *eoe) {
    int result = 0;
    for (auto& ae : *eoe) {
        result ^= CrCalcConstIntAndExpr(namescope, ae.get());
    }
    return result;
}

int CrCalcConstIntInclOrExpr(CR_NameScope& namescope, InclOrExpr *ioe) {
    int result = 0;
    for (auto& eoe : *ioe) {
        result |= CrCalcConstIntExclOrExpr(namescope, eoe.get());
    }
    return result;
}

int CrCalcConstIntLogAndExpr(CR_NameScope& namescope, LogAndExpr *lae) {
    int result = 1;
    if (lae->size() == 1) {
        result = CrCalcConstIntInclOrExpr(namescope, (*lae)[0].get());
    } else {
        for (auto& ioe : *lae) {
            result = result && CrCalcConstIntInclOrExpr(namescope, ioe.get());
            if (!result) {
                break;
            }
        }
    }
    return result;
}

int CrCalcConstIntLogOrExpr(CR_NameScope& namescope, LogOrExpr *loe) {
    int result = 0;
    if (loe->size() == 1) {
        result = CrCalcConstIntLogAndExpr(namescope, (*loe)[0].get());
    } else {
        for (auto& lae : *loe) {
            result = CrCalcConstIntLogAndExpr(namescope, lae.get());
            if (result) {
                result = 1;
                break;
            }
        }
    }
    return result;
}

int CrCalcConstIntAssignExpr(CR_NameScope& namescope, AssignExpr *ae) {
    int n1, n2;
    switch (ae->m_assign_type) {
    case AssignExpr::COND:
        n1 = CrCalcConstIntCondExpr(namescope, ae->m_cond_expr.get());
        return n1;

    case AssignExpr::SINGLE:
        n1 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        return n1;

    case AssignExpr::MUL:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 *= n2;
        return n1;

    case AssignExpr::DIV:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 /= n2;
        return n1;

    case AssignExpr::MOD:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 %= n2;
        return n1;

    case AssignExpr::ADD:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 += n2;
        return n1;

    case AssignExpr::SUB:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 -= n2;
        return n1;

    case AssignExpr::L_SHIFT:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 <<= n2;
        return n1;

    case AssignExpr::R_SHIFT:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 >>= n2;
        return n1;

    case AssignExpr::AND:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 &= n2;
        return n1;

    case AssignExpr::XOR:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 ^= n2;
        return n1;

    case AssignExpr::OR:
        n1 = CrCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 |= n2;
        return n1;

    default:
        assert(0);
    }
    return 0;
}

int CrCalcConstIntExpr(CR_NameScope& namescope, Expr *e) {
    int result = 0;
    for (auto& ae : *e) {
        result = CrCalcConstIntAssignExpr(namescope, ae.get());
    }
    return result;
}

int CrCalcConstIntCondExpr(CR_NameScope& namescope, CondExpr *ce) {
    int result = 0;
    switch (ce->m_cond_type) {
    case CondExpr::SINGLE:
        result = CrCalcConstIntLogOrExpr(namescope, ce->m_log_or_expr.get());
        break;

    case CondExpr::QUESTION:
        if (CrCalcConstIntLogOrExpr(namescope, ce->m_log_or_expr.get())) {
            result = CrCalcConstIntExpr(namescope, ce->m_expr.get());
        } else {
            result = CrCalcConstIntCondExpr(namescope, ce->m_cond_expr.get());
        }
        break;

    default:
        assert(0);
        break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////
// CrAnalyse... functions

CR_TypeID CrAnalysePointer(CR_NameScope& namescope, Pointers *pointers,
                           CR_TypeID tid);
void CrAnalyseTypedefDeclorList(CR_NameScope& namescope, CR_TypeID tid,
                                DeclorList *dl, const CR_Location& location);
void CrAnalyseDeclorList(CR_NameScope& namescope, CR_TypeID tid,
                         DeclorList *dl);
void CrAnalyseStructDeclorList(CR_NameScope& namescope, CR_TypeID tid,
                               DeclorList *dl, CR_LogStruct& ls);
void CrAnalyseDeclList(CR_NameScope& namescope, DeclList *dl);
void CrAnalyseParamList(CR_NameScope& namescope, CR_LogFunc& func,
                        ParamList *pl);
void CrAnalyseFunc(CR_NameScope& namescope, CR_TypeID return_type,
                   Declor *declor, DeclList *decl_list);
CR_TypeID CrAnalyseStructDeclList(CR_NameScope& namescope,
                                  const CR_String& name, DeclList *dl,
                                  int pack, const CR_Location& location);
CR_TypeID CrAnalyseUnionDeclList(CR_NameScope& namescope,
                                 const CR_String& name, DeclList *dl,
                                 const CR_Location& location);
CR_TypeID CrAnalyseEnumorList(CR_NameScope& namescope,
                              const CR_String& name, EnumorList *el);
CR_TypeID CrAnalyseAtomic(CR_NameScope& namescope, AtomicTypeSpec *ats);
CR_TypeID CrAnalyseDeclSpecs(CR_NameScope& namescope, DeclSpecs *ds);

////////////////////////////////////////////////////////////////////////////

CR_TypeID CrAnalysePointers(CR_NameScope& namescope, Pointers *pointers,
                            CR_TypeID tid, const CR_Location& location)
{
    assert(pointers);
    for (auto& ac : *pointers) {
        assert(ac);
        if (tid == cr_invalid_id)
            return 0;
        tid = namescope.AddPtrType(tid, ac->m_flags, location);
    }
    return tid;
}

void CrAnalyseTypedefDeclorList(CR_NameScope& namescope, CR_TypeID tid,
                                DeclorList *dl, const CR_Location& location)
{
    assert(dl);
    for (auto& declor : *dl) {
        CR_TypeID tid2 = tid;

        int value;
        Declor *d = declor.get();
        while (d) {
            CR_String name;
            switch (d->m_declor_type) {
            case Declor::TYPEDEF_TAG:
                assert(!d->m_name.empty());
                name = d->m_name;
                #ifdef __GNUC__
                    if (name == "__builtin_va_list")
                        name = "va_list";
                #endif
                if (d->m_flags && namescope.IsFuncType(tid2))
                    namescope.AddTypeFlags(tid2, d->m_flags);
                namescope.AddAliasType(name, tid2, location);
                d = NULL;
                break;

            case Declor::POINTERS:
                if (namescope.IsFuncType(tid2)) {
                    Pointers *pointers = d->m_pointers.get();
                    auto ac = (*pointers)[0];
                    namescope.AddTypeFlags(tid2, ac->m_flags);
                }
                tid2 = CrAnalysePointers(namescope, d->m_pointers.get(), tid2, location);
                d = d->m_declor.get();
                continue;

            case Declor::ARRAY:
                if (d->m_const_expr)
                    value = CrCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                else
                    value = 0;
                tid2 = namescope.AddArrayType(tid2, value, location);
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc func;
                    func.m_return_type = tid2;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, func, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(func, d->location());
                    d = d->m_declor.get();
                }
                continue;

            case Declor::BITS:
                // TODO:
                assert(0);
                d = NULL;
                break;

            default:
                assert(0);
                d = NULL;
                break;
            }
        }
    }
}

void CrAnalyseDeclorList(CR_NameScope& namescope, CR_TypeID tid,
                         DeclorList *dl)
{
    assert(dl);
    for (auto& declor : *dl) {
        CR_TypeID tid2 = tid;

        int value;
        Declor *d = declor.get();
        while (d) {
            #ifdef DEEPDEBUG
                printf("DeclorList#%s\n", namescope.StringOfType(tid2, "").data());
            #endif

            switch (d->m_declor_type) {
            case Declor::IDENTIFIER:
                if (d->m_flags && namescope.IsFuncType(tid2))
                    namescope.AddTypeFlags(tid2, d->m_flags);
                namescope.AddVar(d->m_name, tid2, d->location());
                #ifdef DEEPDEBUG
                    printf("#%s\n", namescope.StringOfType(tid2, d->m_name).data());
                #endif
                d = d->m_declor.get();
                break;

            case Declor::POINTERS:
                if (namescope.IsFuncType(tid2)) {
                    Pointers *pointers = d->m_pointers.get();
                    auto ac = (*pointers)[0];
                    namescope.AddTypeFlags(tid2, ac->m_flags);
                }
                tid2 = CrAnalysePointers(namescope, d->m_pointers.get(), tid2, d->location());
                d = d->m_declor.get();
                break;

            case Declor::ARRAY:
                if (d->m_const_expr)
                    value = CrCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                else
                    value = 0;
                tid2 = namescope.AddArrayType(tid2, value, d->location());
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc lf;
                    lf.m_return_type = tid2;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, lf, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(lf, d->location());
                    d = d->m_declor.get();
                }
                break;

            default:
                assert(0);
                break;
            }
        }
    }
}

void CrAnalyseStructDeclorList(CR_NameScope& namescope, CR_TypeID tid,
                               DeclorList *dl, CR_LogStruct& ls)
{
    assert(dl);
    for (auto& declor : *dl) {
        CR_TypeID tid2 = tid;

        int value, bits = 0;
        CR_String name;
        Declor *d = declor.get();
        while (d) {
            switch (d->m_declor_type) {
            case Declor::IDENTIFIER:
                if (d->m_flags && namescope.IsFuncType(tid2))
                    namescope.AddTypeFlags(tid2, d->m_flags);
                name = d->m_name;
                d = NULL;
                break;

            case Declor::POINTERS:
                if (namescope.IsFuncType(tid2)) {
                    Pointers *pointers = d->m_pointers.get();
                    auto ac = (*pointers)[0];
                    namescope.AddTypeFlags(tid2, ac->m_flags);
                }
                tid2 = CrAnalysePointers(namescope, d->m_pointers.get(), tid2,
                                         d->location());
                d = d->m_declor.get();
                continue;

            case Declor::ARRAY:
                if (d->m_const_expr)
                    value = CrCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                else
                    value = 0;
                tid2 = namescope.AddArrayType(tid2, value, d->location());
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc lf;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, lf, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(lf, d->location());
                    d = d->m_declor.get();
                }
                continue;

            case Declor::BITS:
                assert(ls.m_is_struct);   // must be struct
                assert(d->m_const_expr);
                bits = CrCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                d = d->m_declor.get();
                continue;

            default:
                assert(0);
                d = NULL;
                break;
            }
        }
        ls.m_type_list.push_back(tid2);
        ls.m_name_list.push_back(name);
        ls.m_offset_list.push_back(0);
        ls.m_bits_list.push_back(bits);
    }
}

void CrAnalyseDeclList(CR_NameScope& namescope, DeclList *dl) {
    assert(dl);
    for (auto& decl : *dl) {
        CR_TypeID tid = CrAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
        switch (decl->m_decl_type) {
        case Decl::TYPEDEF:
            if (decl->m_declor_list.get()) {
                CrAnalyseTypedefDeclorList(namescope, tid,
                    decl->m_declor_list.get(), decl->location());
            }
            break;

        case Decl::DECLORLIST:
            CrAnalyseDeclorList(namescope, tid, decl->m_declor_list.get());
            break;

        case Decl::STATIC_ASSERT:
            {
                shared_ptr<CondExpr> const_expr =
                    decl->m_static_assert_decl->m_const_expr;
                if (CrCalcConstIntCondExpr(namescope, const_expr.get()) == 0)
                {
                    assert(0);
                }
            }
            break;

        default:
            break;
        }
    }
}

void CrAnalyseParamList(CR_NameScope& namescope, CR_LogFunc& func,
                        ParamList *pl)
{
    assert(pl);
    func.m_ellipsis = pl->m_ellipsis;
    for (auto& decl : *pl) {
        assert(decl->m_decl_type == Decl::PARAM);
        assert(decl->m_declor_list->size() <= 1);

        DeclorList *dl = decl->m_declor_list.get();
        Declor *d;
        if (decl->m_declor_list->size())
            d = (*dl)[0].get();
        else
            d = NULL;
        CR_TypeID tid;
        tid = CrAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());

        #ifdef DEEPDEBUG
            printf("ParamList##%s\n", namescope.StringOfType(tid, "").data());
        #endif

        CR_TypeID tid2 = tid;
        int value;
        CR_String name;
        while (d) {
            switch (d->m_declor_type) {
            case Declor::IDENTIFIER:
                name = d->m_name;
                d = NULL;
                break;

            case Declor::POINTERS:
                if (namescope.IsFuncType(tid2)) {
                    Pointers *pointers = d->m_pointers.get();
                    auto ac = (*pointers)[0];
                    namescope.AddTypeFlags(tid2, ac->m_flags);
                }
                tid2 = CrAnalysePointers(namescope, d->m_pointers.get(), tid2,
                                         d->location());
                d = d->m_declor.get();
                continue;

            case Declor::ARRAY:
                if (d->m_const_expr)
                    value = CrCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                else
                    value = 0;
                tid2 = namescope.AddArrayType(tid2, value, d->location());
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc lf;
                    lf.m_return_type = tid2;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, lf, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(lf, d->location());
                    d = d->m_declor.get();
                }
                continue;

            case Declor::BITS:
                // TODO:
                d = NULL;
                break;

            default:
                assert(0);
                d = NULL;
            }
        }
        func.m_type_list.push_back(tid2);
        func.m_name_list.push_back(name);
    }
}

void CrAnalyseFunc(CR_NameScope& namescope, CR_TypeID return_type,
                   Declor *declor, DeclList *decl_list)
{
    CR_LogFunc func;
    assert(declor);

    if (declor->m_declor_type == Declor::FUNCTION) {
        if (!declor->m_name.empty()) {
            if (decl_list) {
                CrAnalyseDeclList(namescope, decl_list);
                if (declor->m_param_list) {
                    CrAnalyseParamList(namescope, func, declor->m_param_list.get());
                    namescope.AddFuncType(func, declor->location());
                } else {
                    assert(0);
                }
            } else {
                assert(declor->m_param_list);
                if (declor->m_param_list) {
                    CrAnalyseParamList(namescope, func, declor->m_param_list.get());
                    namescope.AddFuncType(func, declor->location());
                }
            }
        }
    }
}

CR_TypeID CrAnalyseStructDeclList(CR_NameScope& namescope,
                                  const CR_String& name, DeclList *dl, int pack,
                                  const CR_Location& location)
{
    CR_LogStruct ls(true);  // struct
    ls.m_pack = pack;
    assert(pack >= 1);

    CR_TypeID tid;
    assert(dl);
    for (auto& decl : *dl) {
        switch (decl->m_decl_type) {
        case Decl::DECLORLIST:
            tid = CrAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
            CrAnalyseStructDeclorList(namescope, tid, decl->m_declor_list.get(), ls);
            break;

        case Decl::SINGLE:
            tid = CrAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
            if (tid != cr_invalid_id) {
                ls.m_type_list.push_back(tid);
                ls.m_name_list.push_back("");
                ls.m_offset_list.push_back(0);
                ls.m_bits_list.push_back(0);
            }
            break;

        case Decl::STATIC_ASSERT:
            {
                shared_ptr<CondExpr> const_expr =
                    decl->m_static_assert_decl->m_const_expr;
                if (CrCalcConstIntCondExpr(namescope, const_expr.get()) == 0) {
                    assert(0);
                }
            }
            break;

        default:
            assert(0);
            return cr_invalid_id;
        }
    }

    return namescope.AddStructType(name, ls, location);
}

CR_TypeID CrAnalyseUnionDeclList(CR_NameScope& namescope,
                                 const CR_String& name, DeclList *dl,
                                 const CR_Location& location)
{
    CR_LogStruct ls(false);     // union
    ls.m_pack = 1;
    assert(dl);

    CR_TypeID tid;
    assert(dl);
    for (auto& decl : *dl) {
        switch (decl->m_decl_type) {
        case Decl::DECLORLIST:
            tid = CrAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
            CrAnalyseStructDeclorList(namescope, tid, decl->m_declor_list.get(), ls);
            break;

        case Decl::SINGLE:
            tid = CrAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
            if (tid != cr_invalid_id) {
                ls.m_type_list.push_back(tid);
                ls.m_name_list.push_back("");
                ls.m_offset_list.push_back(0);
                ls.m_bits_list.push_back(0);
            }
            break;

        case Decl::STATIC_ASSERT:
            {
                shared_ptr<CondExpr> const_expr =
                    decl->m_static_assert_decl->m_const_expr;
                if (CrCalcConstIntCondExpr(namescope, const_expr.get()) == 0) {
                    assert(0);
                }
            }
            break;

        default:
            assert(0);
            return cr_invalid_id;
        }
    }

    return namescope.AddUnionType(name, ls, location);
}

CR_TypeID CrAnalyseEnumorList(CR_NameScope& namescope,
                              const CR_String& name, EnumorList *el,
                              const CR_Location& location)
{
    CR_LogEnum le;

    int value, next_value = 0;
    assert(el);
    CR_TypeID tid_enumitem = namescope.TypeIDFromName("enumitem");
    CR_IDSet vids;
    for (auto& e : *el) {
        if (e->m_const_expr)
            value = CrCalcConstIntCondExpr(namescope, e->m_const_expr.get());
        else
            value = next_value;

        le.MapNameToValue()[e->m_name.data()] = value;
        le.MapValueToName()[value] = e->m_name.data();
        CR_VarID vid = namescope.AddVar(e->m_name, tid_enumitem, location);
        namescope.LogVar(vid).m_has_value = true;
        namescope.LogVar(vid).m_int_value = value;
        vids.push_back(vid);
        next_value = value + 1;
    }

    CR_TypeID tid = namescope.AddEnumType(name, le, location);
    for (const auto& vid : vids) {
        namescope.LogVar(vid).m_enum_type_id = tid;
    }
    return tid;
}

CR_TypeID CrAnalyseAtomic(CR_NameScope& namescope, AtomicTypeSpec *ats) {
    // TODO: TF_ATOMIC
    assert(0);
    return 0;
}

CR_TypeID CrAnalyseDeclSpecs(CR_NameScope& namescope, DeclSpecs *ds) {
    CR_TypeID tid;
    CR_TypeFlags flag, flags = 0;
    if (ds == NULL)
        return namescope.TypeIDFromName("int");

    while (ds) {
        CR_String name;
        switch (ds->m_spec_type) {
        case DeclSpecs::STORCLSSPEC:
            flag = ds->m_stor_cls_spec->m_flag;
            flags |= flag;
            if (ds->m_decl_specs) {
                ds = ds->m_decl_specs.get();
                continue;
            }
            break;

        case DeclSpecs::FUNCSPEC:
            flag = ds->m_func_spec->m_flag;
            flags |= flag;
            if (ds->m_decl_specs) {
                ds = ds->m_decl_specs.get();
                continue;
            }
            break;

        case DeclSpecs::TYPESPEC:
            assert(ds->m_type_spec);
            flag = ds->m_type_spec->m_flag;
            switch (flag) {
            case TF_ALIAS:
                name = ds->m_type_spec->m_name;
                assert(!name.empty());
                #ifdef __GNUC__
                    if (name == "__builtin_va_list")
                        name = "va_list";
                #endif
                tid = namescope.TypeIDFromName(name);
                if (tid == 0 || tid == cr_invalid_id) {
                    return 0;
                }
                return tid;

            case TF_STRUCT:
                {
                    TypeSpec *ts = ds->m_type_spec.get();
                    name = ts->m_name;
                    if (ts->m_decl_list) {
                        tid = CrAnalyseStructDeclList(
                            namescope, name, ts->m_decl_list.get(), ts->m_pack,
                            ts->location());
                    } else {
                        tid = namescope.TypeIDFromName(name);
                        if (tid == cr_invalid_id) {
                            CR_LogStruct ls(true);
                            tid = namescope.AddStructType(name, ls, ts->location());
                        }
                    }
                }
                if (flags & TF_CONST) {
                    tid = namescope.AddConstType(tid);
                }
                assert(tid != cr_invalid_id);
                return tid;

            case TF_UNION:
                {
                    TypeSpec *ts = ds->m_type_spec.get();
                    name = ts->m_name;
                    if (ts->m_decl_list) {
                        tid = CrAnalyseUnionDeclList(
                            namescope, name, ts->m_decl_list.get(),
                            ts->location());
                    } else {
                        tid = namescope.TypeIDFromName(name);
                        if (tid == cr_invalid_id) {
                            CR_LogStruct ls(false);
                            tid = namescope.AddUnionType(name, ls, ts->location());
                        }
                    }
                }
                if (flags & TF_CONST) {
                    tid = namescope.AddConstType(tid);
                }
                assert(tid != cr_invalid_id);
                return tid;

            case TF_ENUM:
                name = ds->m_type_spec->m_name;
                if (ds->m_type_spec->m_enumor_list) {
                    tid = CrAnalyseEnumorList(
                        namescope, name, ds->m_type_spec->m_enumor_list.get(),
                        ds->m_type_spec->location());
                } else {
                    CR_LogEnum le;
                    tid = namescope.AddEnumType(
                        name, le, ds->m_type_spec->location());
                }
                if (flags & TF_CONST) {
                    tid = namescope.AddConstType(tid);
                }
                assert(tid != cr_invalid_id);
                return tid;

            case TF_ATOMIC:
                return CrAnalyseAtomic(namescope,
                    ds->m_type_spec.get()->m_atomic_type_spec.get());

            default:
                if ((flags & TF_LONG) && (flag == TF_LONG)) {
                    flags &= ~TF_LONG;
                    flags |= TF_LONGLONG;
                } else {
                    flags |= flag;
                }
                if (ds->m_decl_specs) {
                    ds = ds->m_decl_specs.get();
                    continue;
                }
            }
            break;

        case DeclSpecs::TYPEQUAL:
            flag = ds->m_type_qual->m_flag;
            flags |= flag;
            if (ds->m_decl_specs) {
                ds = ds->m_decl_specs.get();
                continue;
            }
            break;

        case DeclSpecs::ALIGNSPEC:
            if (ds->m_decl_specs) {
                ds = ds->m_decl_specs.get();
                continue;
            }
        }
        break;
    }

    flags = CrNormalizeTypeFlags(flags);
    tid = namescope.TypeIDFromFlags(flags & ~TF_CONST);
    assert(tid != cr_invalid_id);
    if (flags & TF_CONST) {
        tid = namescope.AddConstType(tid);
        assert(tid != cr_invalid_id);
    }
    return tid;
}

////////////////////////////////////////////////////////////////////////////

enum CR_ExitCode {
    cr_exit_ok                = 0,
    cr_exit_parse_error       = 1,
    cr_exit_cpp_error         = 2,
    cr_exit_wrong_ext         = 3,
    cr_exit_bits_mismatched   = 4,
    cr_exit_cant_load         = 5,
    cr_exit_no_input          = 6,
    cr_exit_invalid_option    = 7,
    cr_exit_cannot_open_input = 8
};

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

std::string CrConvertCmdLineParam(const std::string& str) {
    std::string result = str;
    CrReplaceString(result, "\"", "\"\"");
    if (result.find_first_of("\" \t\f\v") != std::string::npos) {
        result = "\"" + str + "\"";
    }
    return result;
}

// do input
int CrInputCSrc(
    shared_ptr<CR_ErrorInfo> error_info, shared_ptr<TransUnit>& tu,
    int i, int argc, char **argv, bool is_64bit)
{
    char *pchDotExt = strrchr(argv[i], '.');

    // if file extension is ".i",
    if (strcmp(pchDotExt, ".i") == 0) {
        // directly parse
        if (!cparser::parse_file(error_info, tu, argv[i], is_64bit)) {
            error_info.get()->add_error(
                std::string("failed to parse file '") + argv[i] + "'"
            );
            return cr_exit_parse_error;   // failure
        }
    } else if (strcmp(pchDotExt, ".h") == 0 ||
               strcmp(pchDotExt, ".c") == 0)
    {
        // if file extension is ".h",
        static char filename[MAX_PATH];
        ::GetTempFileNameA(".", "cpa", 0, filename);
        cr_tmpfile = filename;
        #if 1
            atexit(CrDeleteTempFileAtExit);
        #endif

        // build command line
        #ifdef __GNUC__
            CR_String cmdline("gcc -E");
        #elif defined(__clang__)
            CR_String cmdline("clang -E");
        #elif defined(_MSC_VER)
            CR_String cmdline("cl /nologo /E");
        #else
            #error You lose.
        #endif

        int k = -1;
        for (; i < argc; i++) {
            if (k == -1 && ::GetFileAttributesA(argv[i]) != 0xFFFFFFFF) {
                k = i;
            }
            cmdline += " ";
            cmdline += CrConvertCmdLineParam(argv[i]);
        }

        MProcessMaker pmaker;
        MFile input, output, error;

        pmaker.SetShowWindow(SW_HIDE);
        pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

        if (pmaker.PrepareForRedirect(&input, &output, &error) &&
            pmaker.CreateProcess(NULL, cmdline.data()))
        {
            MFile tfile;
            tfile.OpenFileForOutput(cr_tmpfile);

            DWORD cbAvail, cbRead;
            static BYTE szBuf[2048];
            const DWORD cbBuf = 2048;

            BOOL bOK = TRUE;
            for (;;) {
                if (output.PeekNamedPipe(NULL, 0, NULL, &cbAvail) && cbAvail > 0) {
                    if (output.ReadFile(szBuf, cbBuf, &cbRead)) {
                        if (!tfile.WriteFile(szBuf, cbRead, &cbRead)) {
                            error_info.get()->add_error(
                                std::string("cannot write to temporary file'") +
                                    cr_tmpfile + "'");
                            bOK = FALSE;
                            break;
                        }
                    } else {
                        DWORD dwError = ::GetLastError();
                        if (dwError != ERROR_MORE_DATA) {
                            error_info.get()->add_error(
                                std::string("cannot read output"));
                            break;
                        }
                    }
                } else {
                    if (error.PeekNamedPipe(NULL, 0, NULL, &cbAvail) && cbAvail > 0) {
                        error.ReadFile(szBuf, cbBuf, &cbRead);
                        fwrite(szBuf, cbRead, 1, stderr);
                        if (cbRead == 0)
                            break;
                    } else if (!pmaker.IsRunning()) {
                        break;
                    }
                }
            }
            tfile.CloseHandle();

            if (bOK) {
                bOK = (pmaker.GetExitCode() == 0);
            }

            if (bOK) {
                if (!cparser::parse_file(error_info, tu, cr_tmpfile, is_64bit)) {
                    return cr_exit_parse_error;   // failure
                }
            } else {
                return cr_exit_cpp_error;   // failure
            }
        } else {
            return cr_exit_cpp_error;   // failure
        }
    } else {
        error_info.get()->add_error(
            std::string("unknown input file extension '") +
                pchDotExt + "'. please use .i or .h"
        );
        return cr_exit_wrong_ext;   // failure
    }
    return cr_exit_ok;   // success
}

////////////////////////////////////////////////////////////////////////////
// semantic analysis

int CrSemanticAnalysis(
    shared_ptr<CR_ErrorInfo> error_info,
    CR_NameScope& namescope, shared_ptr<TransUnit>& tu)
{
    assert(tu.get());
    for (auto& decl : *tu.get()) {
        switch (decl->m_decl_type) {
        case Decl::FUNCTION: {
                fflush(stderr);
                auto& ds = decl->m_decl_specs;
                CR_TypeID tid = CrAnalyseDeclSpecs(namescope, ds.get());
                auto& dl = decl->m_declor_list;
                assert(dl.get());
                auto& declor = (*dl.get())[0];
                CrAnalyseFunc(namescope, tid, declor.get(),
                              decl->m_decl_list.get());
            }
            break;

        case Decl::TYPEDEF:
        case Decl::DECLORLIST:
            {
                auto& ds = decl->m_decl_specs;
                auto& dl = decl->m_declor_list;
                CR_TypeID tid = CrAnalyseDeclSpecs(namescope, ds.get());
                if (decl->m_decl_type == Decl::TYPEDEF) {
                    fflush(stderr);
                    if (dl.get()) {
                        CrAnalyseTypedefDeclorList(namescope, tid, dl.get(),
                                                   decl->location());
                    }
                } else {
                    fflush(stderr);
                    CrAnalyseDeclorList(namescope, tid, dl.get());
                }
            }
            break;

        case Decl::SINGLE:
            {
                auto& ds = decl->m_decl_specs;
                CrAnalyseDeclSpecs(namescope, ds.get());
            }
            break;

        default:
            break;
        }
    }

    namescope.CompleteTypeInfo();

    return cr_exit_ok;   // success
}

////////////////////////////////////////////////////////////////////////////

void CrDumpSemantic(
    CR_NameScope& namescope,
    const CR_String& strPrefix,
    const CR_String& strSuffix)
{
    FILE *fp;

    fp = fopen((strPrefix + "types" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(type_id)\t(name)\t(flags)\t(sub_id)\t(count)\t(size)\t(align)\t(file)\t(line)\t(definition)\n");
        for (CR_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
            const auto& name = namescope.MapTypeIDToName()[tid];
            const auto& type = namescope.LogType(tid);
            const auto& location = type.location();
            if (namescope.IsCrExtendedType(tid)) {
                ;
            } else if (namescope.IsPredefinedType(tid)) {
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%d\t(predefined)\t0\t(predefined)\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_sub_id), static_cast<int>(type.m_count),
                    static_cast<int>(type.m_size), static_cast<int>(type.m_align));
            } else if (type.m_flags & (TF_STRUCT | TF_UNION | TF_ENUM)) {
                auto strDef = namescope.StringOfType(tid, "", true);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%d\t%s\t%d\t%s;\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_sub_id), static_cast<int>(type.m_count),
                    static_cast<int>(type.m_size), static_cast<int>(type.m_align),
                    location.m_file.data(), location.m_line, strDef.data());
            } else if (type.m_flags & (TF_POINTER | TF_FUNCTION | TF_ARRAY)) {
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%d\t%s\t%d\t\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_sub_id),
                    static_cast<int>(type.m_count), static_cast<int>(type.m_size),
                    static_cast<int>(type.m_align),
                    location.m_file.data(), location.m_line);
            } else {
                auto strDef = namescope.StringOfType(tid, name, true);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%d\t%s\t%d\ttypedef %s;\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_sub_id), static_cast<int>(type.m_count),
                    static_cast<int>(type.m_size), static_cast<int>(type.m_align),
                    location.m_file.data(), location.m_line, strDef.data());
            }
        }
        fclose(fp);
    }

    fp = fopen((strPrefix + "structures" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(struct_id)\t(name)\t(type_id)\t(is_struct)\t(size)\t(count)\t(pack)\t(align)\t(file)\t(line)\t(definition)\t(item_1_name)\t(item_1_type_id)\t(item_1_offset)\t(item_1_bits)\t(item_2_type_id)\t...\n");
        for (CR_TypeID sid = 0; sid < namescope.LogStructs().size(); ++sid) {
            const auto& ls = namescope.LogStruct(sid);
            auto tid = ls.m_tid;
            const auto& type = namescope.LogType(tid);
            const auto& location = type.location();
            const auto& name = namescope.MapTypeIDToName()[tid];
            auto strDef = namescope.StringOfType(tid, "");
            if (name.empty()) {
                strDef.clear();
            }
            size_t size = namescope.SizeOfType(tid);
            assert(ls.m_type_list.size() == ls.m_name_list.size());
            fprintf(fp, "%d\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s;",
                static_cast<int>(sid), name.data(), static_cast<int>(tid), 
                static_cast<int>(ls.m_is_struct),
                static_cast<int>(size), static_cast<int>(ls.m_type_list.size()),
                static_cast<int>(ls.m_pack), static_cast<int>(ls.m_align),
                location.m_file.data(), location.m_line, strDef.data());
            assert(ls.m_type_list.size() == ls.m_offset_list.size());
            assert(ls.m_type_list.size() == ls.m_bits_list.size());
            for (size_t i = 0; i < ls.m_type_list.size(); ++i) {
                fprintf(fp, "\t%s\t%d\t%d\t%d",
                    ls.m_name_list[i].data(),
                    static_cast<int>(ls.m_type_list[i]),
                    static_cast<int>(ls.m_offset_list[i]),
                    static_cast<int>(ls.m_bits_list[i]));
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }

    fp = fopen((strPrefix + "enums" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(type_id)\t(name)\t(num_items)\t(file)\t(line)\t(item_name_1)\t(item_value_1)\t(item_name_2)\t...\n");
        for (CR_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
            const auto& type = namescope.LogType(tid);
            if (type.m_flags & TF_ENUM) {
                const auto& name = namescope.MapTypeIDToName()[tid];
                const auto& le = namescope.LogEnum(type.m_sub_id);
                size_t num_items = le.MapNameToValue().size();
                const auto& location = type.location();
                fprintf(fp, "%d\t%s\t%d\t%s\t%d",
                    static_cast<int>(tid), name.data(), static_cast<int>(num_items),
                    location.m_file.data(), location.m_line);
                for (const auto& item : le.MapNameToValue()) {
                    fprintf(fp, "\t%s\t%d",
                        item.first.data(), item.second);
                }
                fprintf(fp, "\n");
            }
        }
        fclose(fp);
    }

    fp = fopen((strPrefix + "enumitems" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(enum_type_id)\t(name)\t(value)\t(enum_name)\t(file)\t(line)\n");
        const auto& vars = namescope.Vars();
        for (CR_VarID i = 0; i < vars.size(); ++i) {
            const auto& var = vars[i];
            const auto& type = namescope.LogType(var.m_type_id);
            if (var.m_has_value && (type.m_flags & TF_ENUMITEM)) {
                const auto& name = namescope.MapVarIDToName()[i];
                const auto& enum_name = namescope.MapTypeIDToName()[var.m_enum_type_id];
                const auto& location = var.location();
                fprintf(fp, "%d\t%s\t%d\t%s\t%s\t%d\n",
                    static_cast<int>(var.m_enum_type_id), name.data(), var.m_int_value,
                    enum_name.data(), location.m_file.data(), location.m_line);
            }
        }
        fclose(fp);
    }

    fp = fopen((strPrefix + "functions" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(type_id)\t(name)\t(param_count)\t(file)\t(line)\t(definition)\t(retval_type)\t(param_1_typeid)\t(param_1_name)\t(param_2_typeid)\t...\n");
        auto& vars = namescope.Vars();
        for (CR_VarID i = 0; i < vars.size(); ++i) {
            const auto& var = vars[i];
            CR_TypeID tid = namescope.ResolveAlias(var.m_type_id);
            if (namescope.IsFuncType(tid)) {
                const auto& name = namescope.MapVarIDToName()[i];
                const auto& type = namescope.LogType(tid);
                const auto& lf = namescope.LogFunc(type.m_sub_id);
                const auto& location = var.location();
                fprintf(fp, "%d\t%s\t%d\t%s\t%d\t%s;\t%d",
                    static_cast<int>(tid), name.data(), 
                    static_cast<int>(lf.m_type_list.size()),
                    location.m_file.data(), location.m_line,
                    namescope.StringOfType(tid, name).data(),
                    static_cast<int>(lf.m_return_type));
                for (size_t j = 0; j < lf.m_type_list.size(); j++) {
                    fprintf(fp, "\t%d\t%s",
                        static_cast<int>(lf.m_type_list[j]),
                        lf.m_name_list[j].data());
                }
                fprintf(fp, "\n");
            }
        }
        fclose(fp);
    }
}

////////////////////////////////////////////////////////////////////////////

void CrShowHelp(void) {
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
    fprintf(stderr,
        " Usage: cparser64 [options] [input-file.h [compiler_options]]\n");
#else
    fprintf(stderr,
        " Usage: cparser [options] [input-file.h [compiler_options]]\n");
#endif
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
    fprintf(stderr, " -32               32-bit mode\n");
    fprintf(stderr, " -64               64-bit mode (default)\n");
#else
    fprintf(stderr, " -32               32-bit mode (default)\n");
    fprintf(stderr, " -64               64-bit mode\n");
#endif
    fprintf(stderr, "--nologo           Don't show logo.\n");
    fprintf(stderr, "--version          Show version only.\n");
    fprintf(stderr, "--prefix prefix    The prefix of output file names (default is empty).\n");
    fprintf(stderr, "--suffix suffix    The suffix of output file names (default is '.dat').\n");
}

////////////////////////////////////////////////////////////////////////////

extern "C"
int main(int argc, char **argv) {
    int i;

    #if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
        bool is_64bit = true;
    #else
        bool is_64bit = false;
    #endif

    bool show_help = false;
    bool show_version = false;
    bool no_logo = false;
    CR_String strPrefix, strSuffix = ".dat";
    for (i = 1; i < argc; ++i) {
        if (::lstrcmpiA(argv[i], "/?") == 0 ||
            ::lstrcmpiA(argv[i], "--help") == 0)
        {
            show_help = true;
        } else if (::lstrcmpiA(argv[i], "/nologo") == 0 ||
                   ::lstrcmpiA(argv[i], "--nologo") == 0)
        {
            no_logo = true;
        } else if (::lstrcmpiA(argv[i], "--version") == 0 ||
                   ::lstrcmpiA(argv[i], "/version") == 0)
        {
            show_version = true;
        } else if (::lstrcmpiA(argv[i], "-32") == 0) {
            is_64bit = false;
        } else if (::lstrcmpiA(argv[i], "-64") == 0) {
            is_64bit = true;
        } else if (::lstrcmpiA(argv[i], "--prefix") == 0 ||
                   ::lstrcmpiA(argv[i], "-p") == 0)
        {
            ++i;
            strPrefix = argv[i];
        } else if (::lstrcmpiA(argv[i], "--suffix") == 0 ||
                   ::lstrcmpiA(argv[i], "-s") == 0)
        {
            ++i;
            strSuffix = argv[i];
        } else {
            if (::GetFileAttributesA(argv[i]) == 0xFFFFFFFF) {
                if (argv[i][0] == '-' || argv[i][0] == '/') {
                    fprintf(stderr, "ERROR: Invalid option '%s'.\n", argv[i]);
                    return cr_exit_invalid_option;
                } else {
                    fprintf(stderr, "ERROR: File '%s' doesn't exist.\n", argv[i]);
                    return cr_exit_cannot_open_input;
                }
            }
            break;
        }
    }

    if (show_version) {
        puts(cr_logo);
        return cr_exit_ok;
    }

    if (!no_logo)
        puts(cr_logo);

    if (show_help) {
        CrShowHelp();
        return cr_exit_ok;
    }

    int result = cr_exit_ok;

    shared_ptr<TransUnit> tu;
    auto error_info = make_shared<CR_ErrorInfo>();
    if (i < argc) {
        fprintf(stderr, "Parsing...\n");
        // argv[i] == input-file
        // argv[i + 1] == compiler option #1
        // argv[i + 2] == compiler option #2
        // argv[i + 3] == ...
        result = CrInputCSrc(error_info, tu, i, argc, argv, is_64bit);
    } else {
        error_info.get()->add_error("no input files");
        result = cr_exit_no_input;
    }

    if (result) {
        error_info->emit_all();
        return result;
    }

    if (tu) {
        fprintf(stderr, "Semantic analysis...\n");
        if (is_64bit) {
            CR_NameScope namescope(error_info, true);

            result = CrSemanticAnalysis(error_info, namescope, tu);
            tu = shared_ptr<TransUnit>();
            if (result == 0) {
                CrDumpSemantic(namescope, strPrefix, strSuffix);
            }
        } else {
            CR_NameScope namescope(error_info, false);

            result = CrSemanticAnalysis(error_info, namescope, tu);
            tu = shared_ptr<TransUnit>();
            if (result == 0) {
                CrDumpSemantic(namescope, strPrefix, strSuffix);
            }
        }
    }

    error_info->emit_all();

    fprintf(stderr, "Done.\n");
    return result;
}

////////////////////////////////////////////////////////////////////////////
