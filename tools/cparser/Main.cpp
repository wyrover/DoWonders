////////////////////////////////////////////////////////////////////////////
// Main.cpp
// Copyright (C) 2014-2015 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse. See file ReadMe.txt and License.txt.
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CParseHeader.h"

////////////////////////////////////////////////////////////////////////////

const char * const cr_logo =
    "///////////////////////////////////////////\n"
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
# ifdef __GNUC__
    "// CParser 0.3.1 (64-bit) for gcc        //\n"
# elif defined(__clang__)
    "// CParser 0.3.1 (64-bit) for clang      //\n"
# elif defined(_MSC_VER)
    "// CParser 0.3.1 (64-bit) for cl (MSVC)  //\n"
# else
#  error You lose!
# endif
#else   // !64-bit
# ifdef __GNUC__
    "// CParser 0.3.1 (32-bit) for gcc        //\n"
# elif defined(__clang__)
    "// CParser 0.3.1 (32-bit) for clang      //\n"
# elif defined(_MSC_VER)
    "// CParser 0.3.1 (32-bit) for cl (MSVC)  //\n"
# else
#  error You lose!
# endif
#endif  // !64-bit
    "// Public Domain Software (PDS)          //\n"
    "// by Katayama Hirofumi MZ (katahiromz)  //\n"
    "// katayama.hirofumi.mz@gmail.com        //\n"
    "///////////////////////////////////////////\n";

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
        str += '(';
        str += node.m_text;
        str += ')';
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

void cparser::Lexer::skip_block_comment2(
    LexerBase2& base, node_container& infos)
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
    } while (c != -1);  // EOF?
} // skip_line_comment

void cparser::Lexer::skip_line_comment2(
    LexerBase2& base, node_container& infos)
{
    char c;
    do {
        c = base.getch();
        if (c == '\n') {
            newline(infos);
            m_in_cxx_comment = false;
            break;  // closed
        }
    } while (c != -1);  // EOF?
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
            base.m_location++;
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
cparser::Lexer::guts_escape_sequence(const char *& it) const {
    std::string ret;
    if (*it == 0) {
        return ret;
    }
    switch (*it) {
    case '\'': case '\"': case '?': case '\\':
        ret += *it;
        break;

    case '0': ret += '\0'; break;
    case 'a': ret += '\a'; break;
    case 'b': ret += '\b'; break;
    case 'f': ret += '\f'; break;
    case 'n': ret += '\n'; break;
    case 'r': ret += '\r'; break;
    case 't': ret += '\t'; break;
    case 'v': ret += '\v'; break;

    case 'x':
        {
            ++it;
            if (*it) {
                auto hex = guts_hex2(it);
                if (hex.size()) {
                    unsigned long value = std::stoul(hex, NULL, 16);
                    ret += static_cast<char>(value);
                }
            }
        }
        break;

    case 'u':
    case 'U':
        {
            ++it;
            auto hex = guts_hex(it);
            ret += '?';  // FIXME & TODO
        }
        break;

    default:
        if ('0' <= *it && *it <= '7') {
            auto octal = guts_octal(it);
            long value = std::stol(octal, NULL, 8);
            ret += static_cast<char>(value);
        } else {
            ret += *it;
        }
    }
    return ret;
} // guts_escape_sequence

std::string
cparser::Lexer::guts_string(const char *& it) const {
    std::string ret;
    assert(*it && *it == '\"');
    for (++it; *it; ++it) {
        switch (*it) {
        case '\"':
            ++it;
            if (*it) {
                if (*it == '\"') {
                    ret += '\"';
                } else {
                    return ret;
                }
            } else {
                return ret;
            }
            break;

        case '\\':
            {
                ++it;
                auto text = guts_escape_sequence(it);
                ret += text;
                if (*it == 0) {
                    return ret;
                }
            }
            break;

        default:
            ret += *it;
        }
    }
    return ret;
} // guts_string

std::string
cparser::Lexer::guts_char(const char *& it) const {
    std::string ret;
    assert(*it && *it == '\'');
    for (++it; *it; ++it) {
        switch (*it) {
        case '\'':
            ++it;
            return ret;

        case '\\':
            {
                ++it;
                auto text = guts_escape_sequence(it);
                ret += text;
                if (*it == 0) {
                    break;
                }
                --it;
            }
            break;

        default:
            ret += *it;
        }
    }
    return ret;
} // guts_char

std::string
cparser::Lexer::guts_hex(const char *& it) const {
    std::string ret;
    for (; *it; ++it) {
        if (isxdigit(*it)) {
            ret += *it;
        } else {
            break;
        }
    }
    return ret;
} // guts_hex

std::string
cparser::Lexer::guts_hex2(const char *& it) const {
    std::string ret;
    for (; *it && ret.size() < 2; ++it) {
        if (isxdigit(*it)) {
            ret += *it;
        } else {
            break;
        }
    }
    return ret;
} // guts_hex

std::string
cparser::Lexer::guts_digits(const char *& it) const {
    std::string ret;
    for (; *it; ++it) {
        if (isdigit(*it)) {
            ret += *it;
        } else {
            break;
        }
    }
    return ret;
} // guts_digits

std::string
cparser::Lexer::guts_octal(const char *& it) const {
    std::string ret;
    for (; *it && ret.size() < 3; ++it) {
        if ('0' <= *it && *it <= '7') {
            ret += *it;
        } else {
            break;
        }
    }
    return ret;
} // guts_octal

std::string
cparser::Lexer::guts_floating(const char *& it) const {
    std::string ret;
    if (*it == '.') {
        ++it;
        std::string digits = guts_digits(it);
    }
    return ret;
} // guts_floating

std::string
cparser::Lexer::guts_indentifier(const char *& it) const {
    std::string ret;
    if (isalpha(*it) || *it == '_') {
        ret += *it;
        for (++it; *it; ++it) {
            if (isalnum(*it) || *it == '_') {
                ret += *it;
            } else {
                break;
            }
        }
    }
    //std::puts(ret.data());
    return ret;
} // guts_indentifier

std::string
cparser::Lexer::guts_integer_suffix(
    const char *& it, CR_TypeFlags& flags) const
{
    std::string ret;
    for (; *it; ++it) {
        if (*it == 'u' || *it == 'U') {
            ret += *it;
            flags |= TF_UNSIGNED;
        } else if (*it == 'l' || *it == 'L') {
            ret += *it;
            if (flags & TF_LONG) {
                flags &= ~TF_LONG;
                flags |= TF_LONGLONG;
            } else {
                flags |= TF_LONG;
            }
        } else if (*it == 'i') {
            ++it;
            if (*it && *it == '6') {
                ++it;
                if (*it && *it == '4') {
                    ++it;
                    ret = "i64";
                    flags |= TF_LONGLONG;
                }
            }
            if (*it && *it == '3') {
                ++it;
                if (*it && *it == '2') {
                    ++it;
                    ret = "i32";
                    flags |= TF_LONG;
                }
            }
            if (*it && *it == '1') {
                ++it;
                if (*it && *it == '6') {
                    ++it;
                    ret = "i16";
                    flags |= TF_SHORT;
                }
            }
            if (*it && *it == '8') {
                ++it;
                ret = "i8";
                flags |= TF_CHAR;
            }
            break;
        } else {
            break;
        }
    }
    return ret;
} // guts_integer_suffix

std::string
cparser::Lexer::guts_floating_suffix(
    const char *& it, CR_TypeFlags& flags) const
{
    std::string ret;
    for (; *it; ++it) {
        if (*it == 'f' || *it == 'F') {
            flags &= ~TF_DOUBLE;
            flags |= TF_FLOAT;
        } else if (*it == 'l' || *it == 'L') {
            flags |= TF_LONG | TF_DOUBLE;
        } else {
            break;
        }
    }
    return ret;
} // guts_floating_suffix

std::string
cparser::Lexer::guts_exponent(const char *& it) const
{
    std::string ret;
    if (*it) {
        if (*it == 'e' || *it == 'E') {
            ret += *it;
            ++it;
            if (*it) {
                if (*it == '+') {
                    ret += *it;
                    ++it;
                } else if (*it == '-') {
                    ret += *it;
                    ++it;
                }
                if (*it) {
                    auto exponent = guts_digits(it);
                    ret += exponent;
                }
            }
        }
    }
    return ret;
} // guts_exponent

bool cparser::Lexer::lexeme(
    const char *& it, const std::string& str)
{
    const char *saved = it;
    std::size_t i = 0, siz = str.size();
    for (; i < siz && *it; ++it, ++i) {
        if (*it != str[i]) {
            it = saved;
            return false;
        }
    }
    if (i != siz) {
        it = saved;
        return false;
    }
    return true;
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
} // just_do_it

void cparser::Lexer::just_do_it2(
    node_container& infos, Token token,
    str_iterator begin, str_iterator end)
{
    LexerBase2 base(begin, end);

    infos.emplace_back(token);

    // get tokens
    node_container read_infos;
    while (get_tokens2(base, read_infos)) {
        infos.insert(infos.end(), read_infos.begin(), read_infos.end());
        read_infos.clear();
    }
    infos.insert(infos.end(), read_infos.begin(), read_infos.end());
    read_infos.clear();

    infos.emplace_back(token);
    infos.emplace_back(eof);

    // token resynthesization
    resynth(base, infos);
} // just_do_it2

std::string cparser::Lexer::get_line(LexerBase& base) {
    std::string line;
    char ch;
    line.reserve(64);
    for (;;) {
        ch = base.getch();
        if (ch == -1 || ch == 0) {
            // EOF or NUL
            break;
        }
        line += ch;
        if (ch == '\n') {
            break;
        }
    }
    return line;
}

std::string cparser::Lexer::get_line2(LexerBase2& base) {
    std::string line;
    char ch;
    line.reserve(64);
    for (;;) {
        ch = base.getch();
        if (ch == -1 || ch == 0) {
            // EOF or NUL
            break;
        }
        line += ch;
        if (ch == '\n') {
            break;
        }
    }
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
    if (line.empty()) {
        return false;
    }
    do_line(line, infos);

    return true;
} // get_tokens

bool
cparser::Lexer::get_tokens2(LexerBase2& base, node_container& infos) {
retry:
    if (base.is_eof()) {
        return false;
    }

    if (m_in_c_comment) {
        // /* ... */
        skip_block_comment2(base, infos);
        goto retry;
    }

    if (m_in_cxx_comment) {
        // // ...
        skip_line_comment2(base, infos);
        goto retry;
    }

    auto line = get_line2(base);
    if (line.empty()) {
        return false;
    }
    do_line(line, infos);

    return true;
} // get_tokens2

void cparser::Lexer::do_line(
    const std::string& line, node_container& infos)
{
    for (const char *it = line.c_str(); *it; ) {
        std::string extra;
        switch (*it) {
        case ' ': case '\t':
            ++it;
            continue;

        case '\n':
            newline(infos);
            ++it;
            break;

        case '#':
            infos.emplace_back(T_SHARP, "#");
            ++it;
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
                auto text = guts_string(it);
                infos.emplace_back(T_STRING, text, extra);
            } else if (*it == '\'') {
                auto text = guts_char(it);
                auto n = static_cast<int>(text[0]);
                text = std::to_string(n);
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
                auto text = guts_string(it);
                infos.emplace_back(T_STRING, text, extra);
            } else if (*it == '\'') {
                auto text = guts_char(it);
                auto n = static_cast<int>(text[0]);
                text = std::to_string(n);
                infos.emplace_back(T_CONSTANT, text, extra);
            } else {
                --it;
                goto label_default;
            }
            break;

        case '"':
            {
                auto text = guts_string(it);
                infos.emplace_back(T_STRING, text);
            }
            break;

        case '\'':
            {
                auto text = guts_char(it);
                auto n = static_cast<int>(text[0]);
                text = std::to_string(n);
                infos.emplace_back(T_CONSTANT, text);
            }
            break;

        case '0':
            ++it;
            if (*it == 'x' || *it == 'X') {
                // 0x or 0X
                ++it;
                auto text = guts_hex(it);
                CR_TypeFlags flags = 0;
                if (*it) {
                    extra = guts_integer_suffix(it, flags);
                }
                infos.emplace_back(T_CONSTANT, "0x" + text, extra, flags);
            } else if (*it == '.') {
                // 0.
                auto text = guts_floating(it);
                CR_TypeFlags flags = TF_DOUBLE;
                if (*it) {
                    extra = guts_floating_suffix(it, flags);
                }
                infos.emplace_back(T_CONSTANT, "0." + text, extra, flags);
            } else {
                // octal
                --it;
                auto text = guts_octal(it);
                CR_TypeFlags flags = 0;
                if (*it) {
                    extra = guts_integer_suffix(it, flags);
                }
                infos.emplace_back(T_CONSTANT, text, extra, flags);
            }
            break;

        case '.':
            if (lexeme(it, "...")) {
                infos.emplace_back(T_ELLIPSIS, "...");
            } else {
                // .
                auto text = guts_floating(it);
                if (text.size() > 1) {
                    CR_TypeFlags flags = TF_DOUBLE;
                    if (*it) {
                        extra = guts_floating_suffix(it, flags);
                    }
                    infos.emplace_back(T_CONSTANT, "0." + text, extra, flags);
                } else {
                    infos.emplace_back(T_DOT, ".");
                }
            }
            break;

        case '<':
            if (lexeme(it, "<<=")) {
                infos.emplace_back(T_L_SHIFT_ASSIGN, "<<=");
            } else if (lexeme(it, "<<")) {
                infos.emplace_back(T_L_SHIFT, "<<");
            } else if (lexeme(it, "<=")) {
                infos.emplace_back(T_LE, "<=");
            } else if (lexeme(it, "<:")) {
                infos.emplace_back(T_L_BRACKET, "<:");
            } else {
                infos.emplace_back(T_LT, "<");
                ++it;
            }
            break;

        case '>':
            if (lexeme(it, ">>=")) {
                infos.emplace_back(T_R_SHIFT_ASSIGN, ">>=");
            } else if (lexeme(it, ">>")) {
                infos.emplace_back(T_R_SHIFT, ">>");
            } else if (lexeme(it, ">=")) {
                infos.emplace_back(T_GE, ">=");
            } else {
                infos.emplace_back(T_GT, ">");
                ++it;
            }
            break;

        case '+':
            if (lexeme(it, "+=")) {
                infos.emplace_back(T_ADD_ASSIGN, "+=");
            } else if (lexeme(it, "++")) {
                infos.emplace_back(T_INC, "++");
            } else {
                infos.emplace_back(T_PLUS, "+");
                ++it;
            }
            break;

        case '-':
            if (lexeme(it, "-=")) {
                infos.emplace_back(T_SUB_ASSIGN, "-=");
            } else if (lexeme(it, "--")) {
                infos.emplace_back(T_DEC, "--");
            } else if (lexeme(it, "->")) {
                infos.emplace_back(T_ARROW, "->");
            } else {
                infos.emplace_back(T_MINUS, "-");
                ++it;
            }
            break;

        case '*':
            if (lexeme(it, "*=")) {
                infos.emplace_back(T_MUL_ASSIGN, "*=");
            } else {
                infos.emplace_back(T_ASTERISK, "*");
                ++it;
            }
            break;

        case '/':
            if (lexeme(it, "/*")) {    // */
                assert(0);
            } else if (lexeme(it, "//")) {
                assert(0);
            } else if (lexeme(it, "/=")) {
                infos.emplace_back(T_DIV_ASSIGN, "/=");
            } else {
                infos.emplace_back(T_SLASH, "/");
                ++it;
            }
            break;

        case '%':
            if (lexeme(it, "%=")) {
                infos.emplace_back(T_MOD_ASSIGN, "%=");
            } else if (lexeme(it, "%>")) {
                infos.emplace_back(T_R_BRACE, "%>");
            } else {
                infos.emplace_back(T_PERCENT, "%");
                ++it;
            }
            break;

        case '&':
            if (lexeme(it, "&=")) {
                infos.emplace_back(T_AND_ASSIGN, "&=");
            } else if (lexeme(it, "&&")) {
                infos.emplace_back(T_L_AND, "&&");
            } else {
                infos.emplace_back(T_AND, "&");
                ++it;
            }
            break;
            
        case '^':
            if (lexeme(it, "^=")) {
                infos.emplace_back(T_XOR_ASSIGN, "^=");
            } else {
                infos.emplace_back(T_XOR, "^");
                ++it;
            }
            break;

        case '|':
            if (lexeme(it, "|=")) {
                infos.emplace_back(T_OR_ASSIGN, "|=");
            } else if (lexeme(it, "||")) {
                infos.emplace_back(T_L_OR, "||");
            } else {
                infos.emplace_back(T_OR, "|");
                ++it;
            }
            break;

        case '=':
            if (lexeme(it, "==")) {
                infos.emplace_back(T_EQUAL, "==");
            } else {
                infos.emplace_back(T_ASSIGN, "=");
                ++it;
            }
            break;

        case '!':
            if (lexeme(it, "!=")) {
                infos.emplace_back(T_NE, "!=");
            } else {
                infos.emplace_back(T_BANG, "!");
                ++it;
            }
            break;

        case ';':
            infos.emplace_back(T_SEMICOLON, ";");
            ++it;
            break;

        case ':':
            if (lexeme(it, ":>")) {
                infos.emplace_back(T_R_BRACKET, ":>");
            } else {
                infos.emplace_back(T_COLON, ":");
                ++it;
            }
            break;

        case ',': infos.emplace_back(T_COMMA, ","); ++it; break;
        case '{': infos.emplace_back(T_L_BRACE, "{"); ++it; break;
        case '}': infos.emplace_back(T_R_BRACE, "}"); ++it; break;
        case '(': infos.emplace_back(T_L_PAREN, "("); ++it; break;
        case ')': infos.emplace_back(T_R_PAREN, ")"); ++it; break;
        case '[': infos.emplace_back(T_L_BRACKET, "["); ++it; break;
        case ']': infos.emplace_back(T_R_BRACKET, "]"); ++it; break;
        case '~': infos.emplace_back(T_TILDA, "~"); ++it; break;
        case '?': infos.emplace_back(T_QUESTION, "?"); ++it; break;

        case -1: // EOF
            ++it;
            break;

label_default:
        default:
            if (isspace(*it)) {
                assert(*it != '\n');
                ++it;
                continue;
            }
            if (isdigit(*it)) {
                auto digits = guts_digits(it);
                if (*it == '.') {
                    ++it;
                    // 123.1232
                    auto decimals = guts_digits(it);
                    auto text = digits + '.' + decimals;
                    CR_TypeFlags flags = TF_DOUBLE;
                    if (*it) {
                        extra = guts_floating_suffix(it, flags);
                    }
                    infos.emplace_back(T_CONSTANT, text, extra, flags);
                } else {
                    std::string exponent;
                    if (*it) {
                        exponent = guts_exponent(it);
                    }
                    if (exponent.size()) {
                        // exponent was found. it's a floating
                        CR_TypeFlags flags = TF_DOUBLE;
                        if (*it) {
                            extra = guts_floating_suffix(it, flags);
                        }
                        auto text = digits + exponent + extra;
                        infos.emplace_back(T_CONSTANT, text, extra, flags);
                    } else {
                        // exponent not found. it's a integer
                        CR_TypeFlags flags = 0;
                        if (*it) {
                            extra = guts_integer_suffix(it, flags);
                        }
                        infos.emplace_back(T_CONSTANT, digits, extra, flags);
                    }
                }
            } else if (isalpha(*it) || *it == '_') {
                // identifier or keyword
                auto text = guts_indentifier(it);
                #ifdef __GNUC__
                    if (text.find("__builtin_") == 0 && text.size() > 10) {
                        if (text != "__builtin_va_list") {
                            text = text.substr(10);
                        }
                    }
                #endif
                Token token = parse_identifier(text);
                infos.emplace_back(token, text);
            } else {
                std::string text;
                text += *it;
                infos.emplace_back(T_INVALID_CHAR, text);
                ++it;
            }
        } // switch (*it)
    } // for (auto it = line.begin(); it != end; ++it)
}

// token resynthesization
void cparser::Lexer::resynth(LexerBase& base, node_container& c) {
    const bool c_show_tokens = false;

    if (c_show_tokens) {
        printf("\n#0\n");
        show_tokens(c.begin(), c.end());
        printf("\n--------------\n");
        fflush(stdout);
    }

    resynth1(base, c);

    if (c_show_tokens) {
        printf("\n#1\n");
        show_tokens(c.begin(), c.end());
        printf("\n--------------\n");
        fflush(stdout);
    }

    resynth2(base, c);
    resynth3(c);
    resynth4(c);
    resynth5(c.begin(), c.end());
    resynth6(c);
    resynth7(c.begin(), c.end());
}

// token resynthesization
void cparser::Lexer::resynth(LexerBase2& base, node_container& c) {
    const bool c_show_tokens = false;

    if (c_show_tokens) {
        printf("\n#0\n");
        show_tokens(c.begin(), c.end());
        printf("\n--------------\n");
        fflush(stdout);
    }

    resynth3(c);
    resynth4(c);
    resynth5(c.begin(), c.end());
    resynth6(c);
    resynth7(c.begin(), c.end());
}

// 1. Delete all T_UNALIGNED.
// 2. Delete and warn all invalid characters.
// 3. Process all pragmas and delete them.
// 4. Count all T_NEWLINE and delete them.
void cparser::Lexer::resynth1(LexerBase& base, node_container& c) {
    node_container     newc;
    newc.reserve(c.size());

    bool    line_top = true;
    auto    end = c.end();
    for (auto it = c.begin(); it != end; ++it) {
        it->m_location = base.m_location;
        it->m_pack = base.packing();

        // invalid character
        if (it->m_token == T_INVALID_CHAR) {
            std::string text = "unexpected character '";
            text += it->m_text + "'";
            add_error(base.m_location, text);
            continue;
        }

        // NOTE: Itanium not supported yet.
        if (it->m_token == T_UNALIGNED) {
            continue;
        }

        // found '#'?
        if (it->m_token == T_SHARP) {
            if (!line_top) {
                add_error(base.m_location, "invalid character '#'");
                continue;
            }

            // #...
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
                    base.m_location.set(it->m_text, lineno);
                } else {
                    --it;
                    base.m_location.m_line = lineno;
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
                    ++base.m_location;
                    break;
                }
                ++it;
            }
            continue;
        }

        // is it '\n' ?
        if (it->m_token == T_NEWLINE) {
            line_top = true;
            ++base.m_location;
        } else {
            line_top = false;
            newc.emplace_back(*it);
        }
    }
    std::swap(c, newc);
}

// 1. Convert __declspec(align(#)) to _Alignas(#).
// 2. Convert __attribute__((__aligned__(#))) to _Alignas(#).
// 3. Convert __attribute__((__vector_size__(#))) to __vector_size__(#).
// 4. Convert __attribute__((__vector_size__(#), __may_alias__)) to __vector_size__(#).
void cparser::Lexer::resynth2(LexerBase& base, node_container& c) {
    node_container newc;
    newc.reserve(c.size());

    node_iterator it, it0, end = c.end();
    bool flag;
    for (it = c.begin(); it != end; ++it) {
        if (it->m_token == T_TYPEDEF) {
            it0 = it;
        }

        // __declspec(align(#))
        flag = token_pattern_match(base, it, end,
            {T_DECLSPEC, T_L_PAREN, eof, T_L_PAREN, eof, T_R_PAREN, T_R_PAREN}
        );
        if (flag && (it + 2)->m_text == "align") {
            it += 2;
            it->m_token = T_ALIGNAS;
            newc.push_back(*it);    // _Alignas
            ++it;
            newc.push_back(*it);    // (
            ++it;
            newc.push_back(*it);    // #
            ++it;
            newc.push_back(*it);    // )
            ++it;
            continue;
        }
        // __attribute__((...(#)))
        flag = token_pattern_match(base, it, end,
            {
                T_ATTRIBUTE, T_L_PAREN, T_L_PAREN,
                    eof, T_L_PAREN, eof, T_R_PAREN,
                T_R_PAREN, T_R_PAREN
            }
        );
        if (flag && ((it + 3)->m_text == "__aligned__" ||
                     (it + 3)->m_text == "__aligned" ||
                     (it + 3)->m_text == "aligned"))
        {
            // __attribute__((__aligned__(#)))
            it += 3;
            it->m_token = T_ALIGNAS;
            newc.push_back(*it);    // _Alignas
            ++it;
            newc.push_back(*it);    // (
            ++it;
            newc.push_back(*it);    // #
            ++it;
            newc.push_back(*it);    // )
            it += 2;
            continue;
        }
        if (flag && ((it + 3)->m_text == "__vector_size__" ||
                     (it + 3)->m_text == "__vector_size" ||
                     (it + 3)->m_text == "vector_size"))
        {
            // __attribute__((__vector_size__(#)))
            it += 3;
            it->m_token = T_VECTOR_SIZE;
            newc.push_back(*it);    // __vector_size__
            ++it;
            newc.push_back(*it);    // (
            ++it;
            newc.push_back(*it);    // #
            ++it;
            newc.push_back(*it);    // )
            it += 2;
            continue;
        }
        // __attribute__((...(#), ?))
        flag = token_pattern_match(base, it, end,
            {
                T_ATTRIBUTE, T_L_PAREN, T_L_PAREN,
                    eof, T_L_PAREN, eof, T_R_PAREN, T_COMMA, eof,
                T_R_PAREN, T_R_PAREN
            }
        );
        if (flag && ((it + 3)->m_text == "__vector_size__" ||
                     (it + 3)->m_text == "__vector_size" ||
                     (it + 3)->m_text == "vector_size") &&
                    ((it + 8)->m_text == "__may_alias__" ||
                     (it + 8)->m_text == "__may_alias" ||
                     (it + 8)->m_text == "may_alias"))
        {
            // __attribute__((__vector_size__(#), __may_alias__))
            it += 3;
            it->m_token = T_VECTOR_SIZE;
            newc.push_back(*it);    // __vector_size__
            ++it;
            newc.push_back(*it);    // (
            ++it;
            newc.push_back(*it);    // #
            ++it;
            newc.push_back(*it);    // )
            it += 4;
            continue;
        }
        newc.push_back(*it);
    }
    std::swap(c, newc);
} // resynth2

// 1. Join string sequence
void cparser::Lexer::resynth3(node_container& c) {
    node_container newc;
    newc.reserve(c.size());

    node_iterator it, it0, end = c.end();
    for (it = c.begin(); it != end; ++it) {
        if (it->m_token == T_STRING) {
            it0 = it;
            for (;;) {
                ++it;
                if (it != end && it->m_token == T_STRING) {
                    it0->m_text += it->m_text;
                } else {
                    --it;
                    break;
                }
            }
            newc.push_back(*it0);
        } else {
            newc.push_back(*it);
        }
    }
    std::swap(c, newc);
}

// 1. Delete __asm__("..." "...") of all function prototypes.
// 2. Delete all __attribute__(...).
// 3. Delete all __declspec(...) and/or __pragma(...)
void cparser::Lexer::resynth4(node_container& c) {
    node_container newc;
    newc.reserve(c.size());

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

        if (it->m_token == T_ATTRIBUTE) {
            ++it;
            skip_paren_block(it, end);
        } else if (it->m_token == T_DECLSPEC || it->m_token == T_PRAGMA) {
            ++it;
            skip_paren_block(it, end);
        } else {
            newc.push_back(*it);
        }
    }
    std::swap(c, newc);
} // resynth4

// 1. Convert tag_name of enum tag_name to T_TAGNAME
// 2. Convert tag_name of struct tag_name to T_TAGNAME
// 3. Convert tag_name of union tag_name to T_TAGNAME
// 4. Process typedef ... or T_TYPE ... by resynth_typedef.
// 5. If a registered type name was found, convert it to T_TYPEDEF_NAME.
void cparser::Lexer::resynth5(node_iterator begin, node_iterator end) {
    #ifdef __GNUC__
        m_type_names->emplace("__builtin_va_list");   // fixup
    #else
        m_type_names->emplace("SOCKADDR_STORAGE");    // fixup
    #endif

    for (node_iterator it = begin; it != end; ++it) {
        if (it->m_token == T_ENUM || it->m_token == T_STRUCT ||
            it->m_token == T_UNION)
        {
            ++it;
            if (it->m_token == T_ALIGNAS) {
                ++it;
                skip_paren_block(it, end);
                ++it;
            }
            if (it->m_token == T_IDENTIFIER) {
                it->set_token(T_TAGNAME);
                if ((it + 1)->m_token == T_SEMICOLON ||
                    (it + 1)->m_token == T_VECTOR_SIZE)
                {
                    // struct tag_name; fixup
                    m_type_names->emplace(it->m_text);
                }
            }
        }
    }

    for (node_iterator it = begin; it != end; ++it) {
        if (it->m_token == T_TYPEDEF) {
            it = resynth_typedef(++it, end);
        } else if (it->m_token == T_IDENTIFIER) {
            if (m_type_names->count(it->m_text)) {
                it->set_token(T_TYPEDEF_NAME);
            }
        }
    }
} // resynth5

// 1. Scan a typedef declaration.
// 2. If a registered type name (not in brace or bracket) was found,
//    then convert it to T_TYPEDEF_TAG or T_TYPEDEF_NAME.
// 3. If an unregistered type name found, register it.
// NOTE: T_TYPEDEF_NAME can be a type specifier in grammar.
// NOTE: T_TYPEDEF_TAG will be defined in the typedef declaration.
cparser::node_iterator
cparser::Lexer::resynth_typedef(node_iterator begin, node_iterator end) {
    int paren_nest = 0, brace_nest = 0, bracket_nest = 0;
    node_iterator it;
    for (it = begin; it != end; ++it) {
        if (brace_nest == 0 && (it->m_token == T_SEMICOLON || it->m_token == T_VECTOR_SIZE))
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
                if (m_type_names->count(it->m_text)) {
                    ++it;
                    if (it->m_token == T_SEMICOLON || it->m_token == T_VECTOR_SIZE ||
                        it->m_token == T_R_PAREN ||
                        it->m_token == T_L_BRACKET || it->m_token == T_COMMA)
                    {
                        // type name followed by ;)[,
                        --it;
                        it->set_token(T_TYPEDEF_TAG);
                    } else {
                        --it;
                        it->set_token(T_TYPEDEF_NAME);
                    }
                } else {
                    it->set_token(T_TYPEDEF_TAG);
                    m_type_names->emplace(it->m_text);

                    ++it;
                    if (it->m_token == T_L_PAREN) {
                        it = resynth_parameter_list(++it, end);
                    } else {
                        --it;
                    }
                }
            } else if (m_type_names->count(it->m_text)) {
                // found a type name not in brace or bracket
                it->set_token(T_TYPEDEF_NAME);
            }
        }
    }
    return it;
} // resynth_typedef

// 1. Scan the parameter list of a function.
// 2. If a "fresh" registered type name was found,
//    then convert it to T_TYPEDEF_NAME.
// NOTE: T_TYPEDEF_NAME can be a type specifier in grammar.
cparser::node_iterator
cparser::Lexer::resynth_parameter_list(
    node_iterator begin, node_iterator end)
{
    int paren_nest = 1;
    bool fresh = true;
    node_iterator it;
    for (it = begin; it != end; ++it) {
        if (it->m_token == T_SEMICOLON || it->m_token == T_VECTOR_SIZE)
            break;
        else if (it->m_token == T_L_PAREN) {
            paren_nest++;
            // it is "fresh" when T_L_PAREN came
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
            if (m_type_names->count(it->m_text)) {
                ++it;
                if (fresh) {
                    --it;
                    it->set_token(T_TYPEDEF_NAME);
                } else {
                    --it;
                }
            }
            // it is not "fresh" when type name came
            fresh = false;
        } else if (it->m_token == T_COMMA) {
            // it is "fresh" when T_COMMA came
            fresh = true;
        }
    }
    return it;
} // resynth_parameter_list

// 1. Skip ( ... ) block. ( ... ) are nestable.
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

// 1. Add calling convention info to T_ASTERISK node.
// 2. Delete all SAL markers (the M$ Source Code Annotation Language).
// 3. Delete all T_GNU_EXTENSION nodes
void cparser::Lexer::resynth6(node_container& c) {
    node_container newc;
    newc.reserve(c.size());

    node_iterator it, it2, end = c.end();
    for (it = c.begin(); it != end; ++it) {
        switch (it->m_token) {
        case T_CDECL: case T_STDCALL: case T_FASTCALL:
            // add calling convention info to the T_ASTERISK node.
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
                // a SAL marker?
                bool f =
                    (it2->m_text == "returnvalue") ||
                    (it2->m_text == "SA_Pre") ||
                    (it2->m_text == "SA_Post") ||
                    (it2->m_text == "SA_FormatString") ||
                    (it2->m_text == "source_annotation_attribute");
                if (f) {
                    f = false;
                    ++it2;
                    skip_paren_block(it2, end);
                    ++it2;
                    if (it2->m_token == T_R_BRACKET) {
                        it = it2;
                        f = true;
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
} // resynth6

// 1. Scan and check parenthesis nesting.
// 2. If T_TYPEDEF_NAME followed by T_R_PAREN or T_COMMA, in a pair of
//    parenthesis, was found, then check the preceding node of it.
//    (It's node in parameter list of function)
// 3. If the preceding node of the T_TYPEDEF_NAME node is T_VOID, T_CHAR,
//    T_SHORT, ..., T_TYPEDEF_NAME, T_TAGNAME, or T_ASTERISK, then
//    convert the T_TYPEDEF_NAME node to T_IDENTIFIER.
void cparser::Lexer::resynth7(node_iterator begin, node_iterator end) {
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
} // resynth7

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
    st.emplace("__attribute__", T_ATTRIBUTE);
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
            if (c_show_pack) {
                std::cerr << base.m_location.str() <<
                    ": pragma pack(pop)" << std::endl;
            }
            base.packing().pop();
        } else if ((it + 1)->m_text == "push"){
            // #pragma pack(push)
            if (c_show_pack) {
                std::cerr << base.m_location.str() <<
                    ": pragma pack(push)" << std::endl;
            }
            base.packing().push(base.packing());
        } else {
            // #pragma pack(#)
            int pack = std::stoi((it + 1)->m_text, NULL, 0);
            if (c_show_pack) {
                std::cerr << base.m_location.str() <<
                    ": pragma pack(" << pack << ")" << std::endl;
            }
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
                if (c_show_pack) {
                    std::cerr << base.m_location.str() <<
                        ": pragma pack(pop," << param << ")" << std::endl;
                }
                base.packing().pop(param);
                return CR_ErrorInfo::NOTHING;
            } else if ((it + 1)->m_text == "push") {
                // #pragma pack(push, ident)
                if (c_show_pack) {
                    std::cerr << base.m_location.str() <<
                        ": pragma pack(push," << param << ")" << std::endl;
                }
                base.packing().push(param);
                return CR_ErrorInfo::NOTHING;
            }
        } else if (param.size() && isdigit(param[0])) {
            // #pragma pack(..., #)
            if ((it + 1)->m_text == "pop") {
                // #pragma pack(pop, #)
                int pack = std::stoi(param, NULL, 0);
                if (c_show_pack) {
                    std::cerr << base.m_location.str() <<
                        ": pragma pack(pop," << pack << ")" << std::endl;
                }
                base.packing().pop(pack);
                return CR_ErrorInfo::NOTHING;
            } else if ((it + 1)->m_text == "push") {
                // #pragma pack(push, #)
                int pack = std::stoi(param, NULL, 0);
                if (c_show_pack) {
                    std::cerr << base.m_location.str() <<
                        ": pragma pack(push," << pack << ")" << std::endl;
                }
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
            if (c_show_pack) {
                std::cerr << base.m_location.str() <<
                    ": pragma pack(push," << ident << "," <<
                        param << ")" << std::endl;
            }
            base.packing().push(ident);
            base.packing().set(base.packing());
            return CR_ErrorInfo::NOTHING;
        } else if (op == "pop") {
            int pack = std::stoi(param, NULL, 0);
            if (c_show_pack) {
                std::cerr << base.m_location.str() <<
                    ": pragma pack(pop," << ident << "," <<
                        param << ")" << std::endl;
            }
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
            message(base.m_location, (it + 1)->m_text);
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
// CrValueOn...Expr functions

CR_TypedValue CrValueOnPrimExpr(CR_NameScope& namescope, PrimExpr *pe);
CR_TypedValue CrValueOnPostfixExpr(CR_NameScope& namescope, PostfixExpr *pe);
CR_TypedValue CrValueOnUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue);
CR_TypedValue CrValueOnCastExpr(CR_NameScope& namescope, CastExpr *ce);
CR_TypedValue CrValueOnMulExpr(CR_NameScope& namescope, MulExpr *me);
CR_TypedValue CrValueOnAddExpr(CR_NameScope& namescope, AddExpr *ae);
CR_TypedValue CrValueOnShiftExpr(CR_NameScope& namescope, ShiftExpr *se);
CR_TypedValue CrValueOnRelExpr(CR_NameScope& namescope, RelExpr *re);
CR_TypedValue CrValueOnEqualExpr(CR_NameScope& namescope, EqualExpr *ee);
CR_TypedValue CrValueOnAndExpr(CR_NameScope& namescope, AndExpr *ae);
CR_TypedValue CrValueOnExclOrExpr(CR_NameScope& namescope, ExclOrExpr *eoe);
CR_TypedValue CrValueOnInclOrExpr(CR_NameScope& namescope, InclOrExpr *ioe);
CR_TypedValue CrValueOnLogAndExpr(CR_NameScope& namescope, LogAndExpr *lae);
CR_TypedValue CrValueOnLogOrExpr(CR_NameScope& namescope, LogOrExpr *loe);
CR_TypedValue CrValueOnAssignExpr(CR_NameScope& namescope, AssignExpr *ae);
CR_TypedValue CrValueOnExpr(CR_NameScope& namescope, Expr *e);
CR_TypedValue CrValueOnCondExpr(CR_NameScope& namescope, CondExpr *ce);

int CrCalcSizeOfTypedValue(CR_NameScope& namescope, const CR_TypedValue& ret) {
    return namescope.SizeOfType(ret.m_type_id);
}

CR_TypedValue CrValueOnPrimExpr(CR_NameScope& namescope, PrimExpr *pe) {
    CR_TypedValue ret;
    switch (pe->m_prim_type) {
    case PrimExpr::IDENTIFIER:
        if (pe->m_text.size()) {
            auto it = namescope.MapNameToVarID().find(pe->m_text);
            if (it != namescope.MapNameToVarID().end()) {
                CR_VarID vid = it->second;
                ret = namescope.LogVar(vid).m_typed_value;
            }
        }
        break;

    case PrimExpr::F_CONSTANT:
        ret.m_text = pe->m_text;
        ret.m_extra = pe->m_extra;
        if (pe->m_extra.find('f') != std::string::npos ||
            pe->m_extra.find('F') != std::string::npos)
        {
            ret.m_type_id = namescope.m_float_type;
            ret.assign<float>(std::stof(pe->m_text, NULL));
        } else if (pe->m_extra.find('l') != std::string::npos ||
                   pe->m_extra.find('L') != std::string::npos)
        {
            ret.m_type_id = namescope.m_long_double_type;
            ret.assign<long double>(std::stold(pe->m_text, NULL));
        } else {
            ret.m_type_id = namescope.m_double_type;
            ret.assign<double>(std::stod(pe->m_text, NULL));
        }
        break;

    case PrimExpr::I_CONSTANT:
        ret.m_text = pe->m_text;
        ret.m_extra = pe->m_extra;
        {
            auto ull = std::stoull(pe->m_text, NULL, 0);
            bool is_unsigned = false;
            if (ret.m_extra.find("U") != std::string::npos ||
                ret.m_extra.find("u") != std::string::npos)
            {
                is_unsigned = true;
            }

            if (ret.m_extra.find("LL") != std::string::npos ||
                ret.m_extra.find("ll") != std::string::npos ||
                ret.m_extra.find("i64") != std::string::npos)
            {
                if (is_unsigned) {
                    ret.m_type_id = namescope.m_ulong_long_type;
                    ret.assign<unsigned long long>(ull);
                } else {
                    ret.m_type_id = namescope.m_long_long_type;
                    ret.assign<long long>(ull);
                }
            } else if (ret.m_extra.find('L') != std::string::npos ||
                       ret.m_extra.find('l') != std::string::npos ||
                       ret.m_extra.find("i32") != std::string::npos)
            {
                if (is_unsigned) {
                    ret.m_type_id = namescope.m_ulong_type;
                    ret.assign<unsigned long>(static_cast<unsigned long>(ull));
                } else {
                    ret.m_type_id = namescope.m_long_type;
                    ret.assign<long>(static_cast<long>(ull));
                }
            } else if (ret.m_extra.find("i16") != std::string::npos) {
                if (is_unsigned) {
                    ret.m_type_id = namescope.m_ushort_type;
                    ret.assign<unsigned short>(static_cast<unsigned short>(ull));
                } else {
                    ret.m_type_id = namescope.m_short_type;
                    ret.assign<short>(static_cast<short>(ull));
                }
            } else if (ret.m_extra.find("i8") != std::string::npos) {
                if (is_unsigned) {
                    ret.m_type_id = namescope.m_uchar_type;
                    ret.assign<unsigned char>(static_cast<unsigned char>(ull));
                } else {
                    ret.m_type_id = namescope.m_char_type;
                    ret.assign<short>(static_cast<char>(ull));
                }
            } else {
                if (is_unsigned) {
                    ret.m_type_id = namescope.m_uint_type;
                    ret.assign<unsigned int>(static_cast<unsigned int>(ull));
                } else {
                    ret.m_type_id = namescope.m_int_type;
                    ret.assign<int>(static_cast<int>(ull));
                }
            }
        }
        break;

    case PrimExpr::STRING:
        ret.m_text = CrEscapeString(pe->m_text);
        ret.m_extra = pe->m_extra;
        if (ret.m_extra.find("L") != std::string::npos ||
            ret.m_extra.find("l") != std::string::npos)
        {
            ret.m_type_id = namescope.AddConstWStringType();
            std::wstring wstr = MAnsiToWide(pe->m_text.data());
            ret.assign(wstr.data(), (wstr.size() + 1) * sizeof(WCHAR));
        } else {
            ret.m_type_id = namescope.AddConstStringType();
            ret.assign(pe->m_text.data(), pe->m_text.size() + 1);
        }
        break;

    case PrimExpr::PAREN:
        ret = CrValueOnExpr(namescope, pe->m_expr.get());
        break;

    case PrimExpr::SELECTION:
        // TODO:
        assert(0);
        break;

    default:
        assert(0);
    }
    return ret;
}

CR_TypedValue CrValueOnPostfixExpr(CR_NameScope& namescope, PostfixExpr *pe) {
    CR_TypedValue ret;
    switch (pe->m_postfix_type) {
    case PostfixExpr::SINGLE:
        ret = CrValueOnPrimExpr(namescope, pe->m_prim_expr.get());
        break;

    case PostfixExpr::ARRAYITEM:
        {
            ret = CrValueOnPostfixExpr(namescope, pe->m_postfix_expr.get());
            CR_TypedValue expr_value = CrValueOnExpr(namescope, pe->m_expr.get());
            int n = namescope.GetIntValue(expr_value);
            ret = namescope.ArrayItem(ret, n);
        }
        break;

    case PostfixExpr::FUNCCALL1:
        break;

    case PostfixExpr::FUNCCALL2:
        break;

    case PostfixExpr::DOT:
        ret = CrValueOnPostfixExpr(namescope, pe->m_postfix_expr.get());
        ret = namescope.Dot(ret, pe->m_text);
        break;

    case PostfixExpr::ARROW:
        ret = CrValueOnPostfixExpr(namescope, pe->m_postfix_expr.get());
        ret = namescope.Arrow(ret, pe->m_text);
        break;

    case PostfixExpr::INC:
        break;

    case PostfixExpr::DEC:
        break;

    default:
        assert(0);
    }
    return ret;
}

int CrCalcSizeOfUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue);

int CrCalcSizeOfCastExpr(CR_NameScope& namescope, CastExpr *ce) {
    assert(ce);
    switch (ce->m_cast_type) {
    case CastExpr::UNARY:
        return CrCalcSizeOfUnaryExpr(namescope, ce->m_unary_expr.get());

    default:
        return 4;
    }
}

int CrCalcSizeOfMulExpr(CR_NameScope& namescope, MulExpr *me) {
    assert(me);
    switch (me->m_mul_type) {
    case MulExpr::SINGLE:
        return CrCalcSizeOfCastExpr(namescope, me->m_cast_expr.get());

    default:
        return 4;
    }
}

int CrCalcSizeOfAddExpr(CR_NameScope& namescope, AddExpr *ae) {
    assert(ae);
    switch (ae->m_add_type) {
    case AddExpr::SINGLE:
        return CrCalcSizeOfMulExpr(namescope, ae->m_mul_expr.get());

    default:
        return 4;
    }
}

int CrCalcSizeOfShiftExpr(CR_NameScope& namescope, ShiftExpr *se) {
    assert(se);
    switch (se->m_shift_type) {
    case ShiftExpr::SINGLE:
        return CrCalcSizeOfAddExpr(namescope, se->m_add_expr.get());

    default:
        return 4;
    }
}

int CrCalcSizeOfRelExpr(CR_NameScope& namescope, RelExpr *re) {
    assert(re);
    switch (re->m_rel_type) {
    case RelExpr::SINGLE:
        return CrCalcSizeOfShiftExpr(namescope, re->m_shift_expr.get());

    default:
        return 4;
    }
}

int CrCalcSizeOfEqualExpr(CR_NameScope& namescope, EqualExpr *ee) {
    assert(ee);
    switch (ee->m_equal_type) {
    case EqualExpr::SINGLE:
        return CrCalcSizeOfRelExpr(namescope, ee->m_rel_expr.get());

    default:
        return 4;
    }
}

int CrCalcSizeOfAndExpr(CR_NameScope& namescope, AndExpr *ae) {
    assert(ae);
    return CrCalcSizeOfEqualExpr(namescope, (*ae)[0].get());
}

int CrCalcSizeOfExclOrExpr(CR_NameScope& namescope, ExclOrExpr *eoe) {
    assert(eoe);
    return CrCalcSizeOfAndExpr(namescope, (*eoe)[0].get());
}

int CrCalcSizeOfInclOrExpr(CR_NameScope& namescope, InclOrExpr *ioe) {
    assert(ioe);
    return CrCalcSizeOfExclOrExpr(namescope, (*ioe)[0].get());
}

int CrCalcSizeOfLogAndExpr(CR_NameScope& namescope, LogAndExpr *lae) {
    assert(lae);
    return CrCalcSizeOfInclOrExpr(namescope, (*lae)[0].get());
}

int CrCalcSizeOfLogOrExpr(CR_NameScope& namescope, LogOrExpr *loe) {
    assert(loe);
    return CrCalcSizeOfLogAndExpr(namescope, (*loe)[0].get());
}

int CrCalcSizeOfExpr(CR_NameScope& namescope, Expr *e);

int CrCalcSizeOfCondExpr(CR_NameScope& namescope, CondExpr *ce) {
    assert(ce);
    switch (ce->m_cond_type) {
    case CondExpr::SINGLE:
        return CrCalcSizeOfLogOrExpr(namescope, ce->m_log_or_expr.get());

    case CondExpr::QUESTION:
        return 
            namescope.GetIntValue(CrValueOnLogOrExpr(namescope, ce->m_log_or_expr.get())) ?
                CrCalcSizeOfExpr(namescope, ce->m_expr.get()) :
                CrCalcSizeOfCondExpr(namescope, ce->m_cond_expr.get());

    default:
        return 0;
    }
}


int CrCalcSizeOfAssignExpr(CR_NameScope& namescope, AssignExpr *ae) {
    assert(ae);
    switch (ae->m_assign_type) {
    case AssignExpr::COND:
        return CrCalcSizeOfCondExpr(namescope, ae->m_cond_expr.get());

    case AssignExpr::SINGLE:
        return CrCalcSizeOfUnaryExpr(namescope, ae->m_unary_expr.get());

    default:
        return 4;
    }
}

int CrCalcSizeOfExpr(CR_NameScope& namescope, Expr *e) {
    assert(e);
    return CrCalcSizeOfAssignExpr(namescope, (*e)[0].get());
}

int CrCalcSizeOfPrimExpr(CR_NameScope& namescope, PrimExpr *pe) {
    assert(pe);

    switch (pe->m_prim_type) {
    case PrimExpr::IDENTIFIER:
        if (pe->m_text.size()) {
            auto it = namescope.MapNameToVarID().find(pe->m_text);
            if (it != namescope.MapNameToVarID().end()) {
                auto& var = namescope.LogVar(it->second);
                return namescope.SizeOfType(var.m_typed_value.m_type_id);
            }
        }
        return 0;

    case PrimExpr::I_CONSTANT:
        if (pe->m_extra.empty()) {
            auto n = std::stoull(pe->m_text, NULL, 0);
            if (n & 0xFFFFFFFF00000000) {
                return sizeof(long long);
            } else {
                return sizeof(int);
            }
        } else if (pe->m_extra.find("LL") != std::string::npos ||
                   pe->m_extra.find("ll") != std::string::npos ||
                   pe->m_extra.find("i64") != std::string::npos)
        {
            return sizeof(long long);
        }
        return sizeof(int);

    case PrimExpr::F_CONSTANT:
        if (pe->m_extra.find('f') != std::string::npos ||
            pe->m_extra.find('F') != std::string::npos)
        {
            return sizeof(float);
        }
        if (pe->m_extra.find('l') != std::string::npos ||
            pe->m_extra.find('L') != std::string::npos)
        {
            return sizeof(long double);
        }
        return sizeof(double);

    case PrimExpr::STRING:
        if (pe->m_extra.find('L') != std::string::npos) {
            return static_cast<int>((pe->m_text.size() + 1) * sizeof(wchar_t));
        } else {
            return static_cast<int>((pe->m_text.size() + 1) * sizeof(char));
        }

    case PrimExpr::PAREN:
        return CrCalcSizeOfExpr(namescope, pe->m_expr.get());

    default:
        return 0;
    }
}

int CrCalcSizeOfPostfixExpr(CR_NameScope& namescope, PostfixExpr *pe) {
    assert(pe);
    switch (pe->m_postfix_type) {
    case PostfixExpr::SINGLE:
        return CrCalcSizeOfPrimExpr(namescope, pe->m_prim_expr.get());

    case PostfixExpr::ARRAYITEM:
        {
            CR_TypedValue ret =
                CrValueOnPostfixExpr(namescope, pe->m_postfix_expr.get());
            CR_TypedValue index =
                CrValueOnExpr(namescope, pe->m_expr.get());
            int n = namescope.GetIntValue(index);
            ret = namescope.ArrayItem(ret, n);
            return CrCalcSizeOfTypedValue(namescope, ret);
        }
        break;

    case PostfixExpr::FUNCCALL1:
    case PostfixExpr::FUNCCALL2:
        assert(0);
        break;

    case PostfixExpr::DOT:
        {
            CR_TypedValue ret =
                CrValueOnPostfixExpr(namescope, pe->m_postfix_expr.get());
            ret = namescope.Dot(ret, pe->m_text);
            return CrCalcSizeOfTypedValue(namescope, ret);
        }

    case PostfixExpr::ARROW:
        {
            CR_TypedValue ret =
                CrValueOnPostfixExpr(namescope, pe->m_postfix_expr.get());
            ret = namescope.Arrow(ret, pe->m_text);
            return CrCalcSizeOfTypedValue(namescope, ret);
        }

    case PostfixExpr::INC:
    case PostfixExpr::DEC:
        return CrCalcSizeOfPostfixExpr(namescope, pe->m_postfix_expr.get());

    default:
        assert(0);
    }
    return 0;
}

int CrCalcSizeOfUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue) {
    assert(ue);
    switch (ue->m_unary_type) {
    case UnaryExpr::SINGLE:
        return CrCalcSizeOfPostfixExpr(namescope, ue->m_postfix_expr.get());

    case UnaryExpr::INC:
    case UnaryExpr::DEC:
        return CrCalcSizeOfUnaryExpr(namescope, ue->m_unary_expr.get());

    case UnaryExpr::ADDRESS:
        {
            CR_TypedValue ret =
                CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
            ret = namescope.Address(ret);
            return CrCalcSizeOfTypedValue(namescope, ret);
        }

    case UnaryExpr::ASTERISK:
        {
            CR_TypedValue ret =
                CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
            ret = namescope.Asterisk(ret);
            return CrCalcSizeOfTypedValue(namescope, ret);
        }

    case UnaryExpr::PLUS:
    case UnaryExpr::MINUS:
    case UnaryExpr::BITWISE_NOT:
        {
            CR_TypedValue ret =
                CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
            return CrCalcSizeOfTypedValue(namescope, ret);
        }

    case UnaryExpr::NOT:
        {
            CR_TypedValue ret =
                CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
            ret = namescope.Not(ret);
            return CrCalcSizeOfTypedValue(namescope, ret);
        }

    case UnaryExpr::SIZEOF1:
    case UnaryExpr::SIZEOF2:
        return (namescope.Is64Bit() ? 8 : 4);

    default:
        assert(0);
        return 0;
    }
}

CR_TypeID CrAnalyseDeclSpecs(CR_NameScope& namescope, DeclSpecs *ds);

size_t CrGetTypeIDOfTypeName(CR_NameScope& namescope, TypeName *tn) {
    assert(tn);
    CR_TypeID tid = CrAnalyseDeclSpecs(namescope, tn->m_decl_specs.get());
    return tid;
}

int CrCalcSizeOfTypeName(CR_NameScope& namescope, TypeName *tn) {
    assert(tn);
    CR_TypeID tid = CrAnalyseDeclSpecs(namescope, tn->m_decl_specs.get());
    return namescope.SizeOfType(tid);
}

int CrCalcAlignOfTypeName(CR_NameScope& namescope, TypeName *tn) {
    assert(tn);
    CR_TypeID tid = CrAnalyseDeclSpecs(namescope, tn->m_decl_specs.get());
    return namescope.LogType(tid).m_align;
}

CR_TypedValue CrValueOnUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue) {
    CR_TypedValue ret;
    switch (ue->m_unary_type) {
    case UnaryExpr::SINGLE:
        ret = CrValueOnPostfixExpr(namescope, ue->m_postfix_expr.get());
        break;

    case UnaryExpr::INC:
        break;

    case UnaryExpr::DEC:
        break;

    case UnaryExpr::ADDRESS:
        ret = CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
        ret = namescope.Address(ret);
        break;

    case UnaryExpr::ASTERISK:
        ret = CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
        ret = namescope.Asterisk(ret);
        break;

    case UnaryExpr::PLUS:
        ret = CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
        break;

    case UnaryExpr::MINUS:
        ret = CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
        ret = namescope.Minus(ret);
        break;

    case UnaryExpr::BITWISE_NOT:
        ret = CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
        ret = namescope.Not(ret);
        break;

    case UnaryExpr::NOT:
        ret = CrValueOnCastExpr(namescope, ue->m_cast_expr.get());
        ret = namescope.LNot(ret);
        break;

    case UnaryExpr::SIZEOF1:
        {
            size_t size = CrCalcSizeOfUnaryExpr(namescope, ue->m_unary_expr.get());
            if (namescope.Is64Bit()) {
                ret.m_type_id = namescope.m_ulong_long_type;
                ret.assign<unsigned long long>(size);
            } else {
                ret.m_type_id = namescope.m_uint_type;
                ret.assign<unsigned int>(static_cast<unsigned int>(size));
            }
        }
        break;

    case UnaryExpr::SIZEOF2:
        {
            size_t size = CrCalcSizeOfTypeName(namescope, ue->m_type_name.get());
            if (namescope.Is64Bit()) {
                ret.m_type_id = namescope.m_ulong_long_type;
                ret.assign<unsigned long long>(size);
            } else {
                ret.m_type_id = namescope.m_uint_type;
                ret.assign<unsigned int>(static_cast<unsigned int>(size));
            }
        }
        break;

    default:
        assert(0);
    }
    return ret;
}

CR_TypedValue CrValueOnCastExpr(CR_NameScope& namescope, CastExpr *ce) {
    CR_TypedValue ret;
    switch (ce->m_cast_type) {
    case CastExpr::UNARY:
        ret = CrValueOnUnaryExpr(namescope, ce->m_unary_expr.get());
        break;
    
    case CastExpr::INITERLIST:
        // TODO:
        //ce->m_type_name
        //ce->m_initer_list
        assert(0);
        break;

    case CastExpr::CAST:
        {
            CR_TypedValue value = CrValueOnCastExpr(namescope, ce->m_cast_expr.get());
            CR_TypeID tid = CrGetTypeIDOfTypeName(namescope, ce->m_type_name.get());
            ret = namescope.Cast(tid, value);
        }
        break;

    default:
        assert(0);
    }
    return ret;
}

CR_TypedValue CrValueOnMulExpr(CR_NameScope& namescope, MulExpr *me) {
    CR_TypedValue n1, n2, ret;
    switch (me->m_mul_type) {
    case MulExpr::SINGLE:
        ret = CrValueOnCastExpr(namescope, me->m_cast_expr.get());
        break;

    case MulExpr::ASTERISK:
        n1 = CrValueOnMulExpr(namescope, me->m_mul_expr.get());
        n2 = CrValueOnCastExpr(namescope, me->m_cast_expr.get());
        ret = namescope.Mul(n1, n2);
        break;

    case MulExpr::SLASH:
        n1 = CrValueOnMulExpr(namescope, me->m_mul_expr.get());
        n2 = CrValueOnCastExpr(namescope, me->m_cast_expr.get());
        ret = namescope.Div(n1, n2);
        break;

    case MulExpr::PERCENT:
        n1 = CrValueOnMulExpr(namescope, me->m_mul_expr.get());
        n2 = CrValueOnCastExpr(namescope, me->m_cast_expr.get());
        ret = namescope.Mod(n1, n2);
        break;

    default:
        assert(0);
    }
    return ret;
}

CR_TypedValue CrValueOnAddExpr(CR_NameScope& namescope, AddExpr *ae) {
    CR_TypedValue n1, n2, ret;
    switch (ae->m_add_type) {
    case AddExpr::SINGLE:
        ret = CrValueOnMulExpr(namescope, ae->m_mul_expr.get());
        break;

    case AddExpr::PLUS:
        n1 = CrValueOnAddExpr(namescope, ae->m_add_expr.get());
        n2 = CrValueOnMulExpr(namescope, ae->m_mul_expr.get());
        ret = namescope.Add(n1, n2);
        break;

    case AddExpr::MINUS:
        n1 = CrValueOnAddExpr(namescope, ae->m_add_expr.get());
        n2 = CrValueOnMulExpr(namescope, ae->m_mul_expr.get());
        ret = namescope.Sub(n1, n2);
        break;

    default:
        assert(0);
    }
    return ret;
}

CR_TypedValue CrValueOnShiftExpr(CR_NameScope& namescope, ShiftExpr *se) {
    CR_TypedValue n1, n2, ret;
    switch (se->m_shift_type) {
    case ShiftExpr::SINGLE:
        ret = CrValueOnAddExpr(namescope, se->m_add_expr.get());
        break;

    case ShiftExpr::L_SHIFT:
        n1 = CrValueOnShiftExpr(namescope, se->m_shift_expr.get());
        n2 = CrValueOnAddExpr(namescope, se->m_add_expr.get());
        ret = namescope.Shl(n1, n2);
        break;

    case ShiftExpr::R_SHIFT:
        n1 = CrValueOnShiftExpr(namescope, se->m_shift_expr.get());
        n2 = CrValueOnAddExpr(namescope, se->m_add_expr.get());
        ret = namescope.Shr(n1, n2);
        break;

    default:
        assert(0);
    }
    return ret;
}

CR_TypedValue CrValueOnRelExpr(CR_NameScope& namescope, RelExpr *re) {
    CR_TypedValue n1, n2, ret;
    switch (re->m_rel_type) {
    case RelExpr::SINGLE:
        ret = CrValueOnShiftExpr(namescope, re->m_shift_expr.get());
        break;

    case RelExpr::LT:
        n1 = CrValueOnRelExpr(namescope, re->m_rel_expr.get());
        n2 = CrValueOnShiftExpr(namescope, re->m_shift_expr.get());
        ret = namescope.Lt(n1, n2);
        break;

    case RelExpr::GT:
        n1 = CrValueOnRelExpr(namescope, re->m_rel_expr.get());
        n2 = CrValueOnShiftExpr(namescope, re->m_shift_expr.get());
        ret = namescope.Gt(n1, n2);
        break;

    case RelExpr::LE:
        n1 = CrValueOnRelExpr(namescope, re->m_rel_expr.get());
        n2 = CrValueOnShiftExpr(namescope, re->m_shift_expr.get());
        ret = namescope.Le(n1, n2);
        break;

    case RelExpr::GE:
        n1 = CrValueOnRelExpr(namescope, re->m_rel_expr.get());
        n2 = CrValueOnShiftExpr(namescope, re->m_shift_expr.get());
        ret = namescope.Ge(n1, n2);
        break;

    default:
        assert(0);
    }
    return ret;
}

CR_TypedValue CrValueOnEqualExpr(CR_NameScope& namescope, EqualExpr *ee) {
    CR_TypedValue n1, n2, ret;
    switch (ee->m_equal_type) {
    case EqualExpr::SINGLE:
        ret = CrValueOnRelExpr(namescope, ee->m_rel_expr.get());
        break;

    case EqualExpr::EQUAL:
        n1 = CrValueOnEqualExpr(namescope, ee->m_equal_expr.get());
        n2 = CrValueOnRelExpr(namescope, ee->m_rel_expr.get());
        ret = namescope.Eq(n1, n2);
        break;

    case EqualExpr::NE:
        n1 = CrValueOnEqualExpr(namescope, ee->m_equal_expr.get());
        n2 = CrValueOnRelExpr(namescope, ee->m_rel_expr.get());
        ret = namescope.Ne(n1, n2);
        break;

    default:
        assert(0);
    }
    return ret;
}

CR_TypedValue CrValueOnAndExpr(CR_NameScope& namescope, AndExpr *ae) {
    CR_TypedValue ret;
    if (ae->size() == 1) {
        ret = CrValueOnEqualExpr(namescope, (*ae)[0].get());
    } else {
        ret = CrValueOnEqualExpr(namescope, (*ae)[0].get());
        for (std::size_t i = 1; i < ae->size(); ++i) {
            ret = namescope.And(
                ret, CrValueOnEqualExpr(namescope, (*ae)[i].get()));
        }
    }
    return ret;
}

CR_TypedValue CrValueOnExclOrExpr(CR_NameScope& namescope, ExclOrExpr *eoe) {
    CR_TypedValue ret;
    if (eoe->size() == 1) {
        ret = CrValueOnAndExpr(namescope, (*eoe)[0].get());
    } else {
        namescope.IntZero(ret);
        for (auto& ae : *eoe) {
            ret =
                namescope.Xor(
                    ret, CrValueOnAndExpr(namescope, ae.get()));
        }
    }
    return ret;
}

CR_TypedValue CrValueOnInclOrExpr(CR_NameScope& namescope, InclOrExpr *ioe) {
    CR_TypedValue ret;
    if (ioe->size() == 1) {
        ret = CrValueOnExclOrExpr(namescope, (*ioe)[0].get());
    } else {
        namescope.IntZero(ret);
        for (auto& eoe : *ioe) {
            ret = namescope.Or(
                ret, CrValueOnExclOrExpr(namescope, eoe.get()));
        }
    }
    return ret;
}

CR_TypedValue CrValueOnLogAndExpr(CR_NameScope& namescope, LogAndExpr *lae) {
    CR_TypedValue ret;
    if (lae->size() == 1) {
        ret = CrValueOnInclOrExpr(namescope, (*lae)[0].get());
    } else {
        namescope.IntOne(ret);
        for (auto& ioe : *lae) {
            ret = namescope.LAnd(
                ret, CrValueOnInclOrExpr(namescope, ioe.get()));
            if (namescope.GetIntValue(ret) == 0) {
                break;
            }
        }
    }
    return ret;
}

CR_TypedValue CrValueOnLogOrExpr(CR_NameScope& namescope, LogOrExpr *loe) {
    CR_TypedValue ret;
    if (loe->size() == 1) {
        ret = CrValueOnLogAndExpr(namescope, (*loe)[0].get());
    } else {
        for (auto& lae : *loe) {
            ret = CrValueOnLogAndExpr(namescope, lae.get());
            if (namescope.GetIntValue(ret) != 0) {
                namescope.IntOne(ret);
                break;
            }
        }
    }
    return ret;
}

CR_TypedValue CrValueOnAssignExpr(CR_NameScope& namescope, AssignExpr *ae) {
    CR_TypedValue n1, n2;
    switch (ae->m_assign_type) {
    case AssignExpr::COND:
        n1 = CrValueOnCondExpr(namescope, ae->m_cond_expr.get());
        return n1;

    case AssignExpr::SINGLE:
        n1 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        return n1;

    case AssignExpr::MUL:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Mul(n1, n2);
        return n1;

    case AssignExpr::DIV:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Div(n1, n2);
        return n1;

    case AssignExpr::MOD:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Mod(n1, n2);
        return n1;

    case AssignExpr::ADD:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Add(n1, n2);
        return n1;

    case AssignExpr::SUB:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Sub(n1, n2);
        return n1;

    case AssignExpr::L_SHIFT:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Shl(n1, n2);
        return n1;

    case AssignExpr::R_SHIFT:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Shr(n1, n2);
        return n1;

    case AssignExpr::AND:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.And(n1, n2);
        return n1;

    case AssignExpr::XOR:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Xor(n1, n2);
        return n1;

    case AssignExpr::OR:
        n1 = CrValueOnUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CrValueOnAssignExpr(namescope, ae->m_assign_expr.get());
        n1 = namescope.Or(n1, n2);
        return n1;

    default:
        assert(0);
    }
    return 0;
}

CR_TypedValue CrValueOnExpr(CR_NameScope& namescope, Expr *e) {
    CR_TypedValue ret;
    namescope.IntZero(ret);
    for (auto& ae : *e) {
        ret = CrValueOnAssignExpr(namescope, ae.get());
    }
    return ret;
}

CR_TypedValue CrValueOnCondExpr(CR_NameScope& namescope, CondExpr *ce) {
    CR_TypedValue ret;
    switch (ce->m_cond_type) {
    case CondExpr::SINGLE:
        ret = CrValueOnLogOrExpr(namescope, ce->m_log_or_expr.get());
        break;

    case CondExpr::QUESTION:
        ret = CrValueOnLogOrExpr(namescope, ce->m_log_or_expr.get());
        if (namescope.GetIntValue(ret) != 0) {
            ret = CrValueOnExpr(namescope, ce->m_expr.get());
        } else {
            ret = CrValueOnCondExpr(namescope, ce->m_cond_expr.get());
        }
        break;

    default:
        assert(0);
        break;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////
// CrAnalyse... functions

void CrAnalyseTypedefDeclorList(CR_NameScope& namescope, CR_TypeID tid,
                                DeclorList *dl, AlignSpec *as,
                                const CR_Location& location);
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
                                  const std::string& name, DeclList *dl,
                                  int pack, int alignas_,
                                  const CR_Location& location);
CR_TypeID CrAnalyseUnionDeclList(CR_NameScope& namescope,
                                 const std::string& name, DeclList *dl,
                                 int pack, int alignas_,
                                 const CR_Location& location);
CR_TypeID CrAnalyseEnumorList(CR_NameScope& namescope,
                              const std::string& name, EnumorList *el,
                              const CR_Location& location);
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
        tid = namescope.AddPointerType(tid, ac->m_flags, location);
    }
    return tid;
}

void CrAnalyseTypedefDeclorList(CR_NameScope& namescope, CR_TypeID tid,
                                DeclorList *dl, AlignSpec *as,
                                const CR_Location& location)
{
    assert(dl);
    for (auto& declor : *dl) {
        CR_TypeID tid2 = tid;

        int int_value;
        Declor *d = declor.get();
        while (d) {
            std::string name;
            switch (d->m_declor_type) {
            case Declor::TYPEDEF_TAG:
                assert(!d->m_name.empty());
                name = d->m_name;
                #ifdef __GNUC__
                    if (name == "__builtin_va_list")
                        name = "va_list";
                #endif
                if (as) {
                    int alignas_ = 0;
                    switch (as->m_align_spec_type) {
                    case AlignSpec::TYPENAME:
                        alignas_ =
                            CrCalcAlignOfTypeName(
                                namescope, as->m_type_name.get());
                        break;

                    case AlignSpec::CONSTEXPR:
                        alignas_ =
                            namescope.GetIntValue(
                                CrValueOnCondExpr(namescope,
                                    as->m_const_expr.get()));
                        break;
                    }
                    namescope.SetAlignas(tid2, alignas_);
                }
                if (d->m_flags && namescope.IsFuncType(tid2)) {
                    namescope.AddTypeFlags(tid2, d->m_flags);
                }
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
                if (d->m_const_expr) {
                    CR_TypedValue value;
                    value = CrValueOnCondExpr(namescope, d->m_const_expr.get());
                    int_value = namescope.GetIntValue(value);
                } else {
                    int_value = 0;
                }
                tid2 = namescope.AddArrayType(tid2, int_value, location);
                if (int_value < 0) {
                    namescope.ErrorInfo()->add_error(
                        location, "size of array is negative");
                }
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc func;
                    func.m_return_type = tid2;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, func, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(func, d->m_location);
                    d = d->m_declor.get();
                }
                continue;

            case Declor::BITS:
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

void CrAnalyseTypedefDeclorListVector(
    CR_NameScope& namescope, CR_TypeID tid,
    DeclorList *dl, AlignSpec *as, int vector_size,
    const CR_Location& location)
{
    assert(dl);
    for (auto& declor : *dl) {
        CR_TypeID tid2 = tid;

        int int_value;
        Declor *d = declor.get();
        while (d) {
            std::string name;
            switch (d->m_declor_type) {
            case Declor::TYPEDEF_TAG:
                assert(!d->m_name.empty());
                name = d->m_name;
                if (as) {
                    CR_TypedValue value;
                    int alignas_ = 0;
                    switch (as->m_align_spec_type) {
                    case AlignSpec::TYPENAME:
                        alignas_ =
                            CrCalcAlignOfTypeName(
                                namescope, as->m_type_name.get());
                        break;

                    case AlignSpec::CONSTEXPR:
                        value = 
                            CrValueOnCondExpr(namescope,
                                as->m_const_expr.get());
                        alignas_ = namescope.GetIntValue(value);
                        break;
                    }
                    namescope.SetAlignas(tid2, alignas_);
                }
                if (d->m_flags && namescope.IsFuncType(tid2)) {
                    namescope.AddTypeFlags(tid2, d->m_flags);
                }
                namescope.AddVectorType(name, tid2, vector_size, location);
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
                if (d->m_const_expr) {
                    CR_TypedValue value;
                    value = CrValueOnCondExpr(namescope, d->m_const_expr.get());
                    int_value = namescope.GetIntValue(value);
                } else {
                    int_value = 0;
                }
                tid2 = namescope.AddArrayType(tid2, int_value, location);
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc func;
                    func.m_return_type = tid2;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, func, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(func, d->m_location);
                    d = d->m_declor.get();
                }
                continue;

            case Declor::BITS:
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

        int int_value;
        Declor *d = declor.get();
        while (d) {
            #ifdef DEEPDEBUG
                printf("DeclorList#%s\n", namescope.StringOfType(tid2, "").data());
            #endif

            switch (d->m_declor_type) {
            case Declor::IDENTIFIER:
                if (d->m_flags && namescope.IsFuncType(tid2))
                    namescope.AddTypeFlags(tid2, d->m_flags);
                namescope.AddVar(d->m_name, tid2, d->m_location);
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
                tid2 = CrAnalysePointers(namescope, d->m_pointers.get(), tid2, d->m_location);
                d = d->m_declor.get();
                break;

            case Declor::ARRAY:
                if (d->m_const_expr) {
                    CR_TypedValue value;
                    value = CrValueOnCondExpr(namescope, d->m_const_expr.get());
                    int_value = namescope.GetIntValue(value);
                } else {
                    int_value = 0;
                }
                tid2 = namescope.AddArrayType(tid2, int_value, d->m_location);
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc lf;
                    lf.m_return_type = tid2;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, lf, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(lf, d->m_location);
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

        int int_value, bits = -1;
        std::string name;
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
                                         d->m_location);
                d = d->m_declor.get();
                continue;

            case Declor::ARRAY:
                if (d->m_const_expr) {
                    CR_TypedValue value;
                    value = CrValueOnCondExpr(namescope, d->m_const_expr.get());
                    int_value = namescope.GetIntValue(value);
                } else {
                    int_value = (ls.m_is_struct ? 0 : 1);
                }
                tid2 = namescope.AddArrayType(tid2, int_value, d->m_location);
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc lf;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, lf, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(lf, d->m_location);
                    d = d->m_declor.get();
                }
                continue;

            case Declor::BITS:
                assert(ls.m_is_struct);   // must be struct
                assert(d->m_const_expr);
                {
                    CR_TypedValue value;
                    value = CrValueOnCondExpr(namescope, d->m_const_expr.get());
                    bits = namescope.GetIntValue(value);
                }
                d = d->m_declor.get();
                continue;

            default:
                assert(0);
                d = NULL;
                break;
            }
        }
        ls.m_members.emplace_back(tid2, name, 0, bits);
    }
}

void CrAnalyseDeclList(CR_NameScope& namescope, DeclList *dl) {
    assert(dl);
    for (auto& decl : *dl) {
        CR_TypeID tid = CrAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
        switch (decl->m_decl_type) {
        case Decl::TYPEDEF:
            if (decl->m_declor_list.get()) {
                if (decl->m_constant.size()) {
                    int vector_size = std::stoul(decl->m_constant, NULL, 0);
                    CrAnalyseTypedefDeclorListVector(namescope, tid,
                        decl->m_declor_list.get(), decl->m_align_spec.get(),
                        vector_size, decl->m_location);
                } else {
                    CrAnalyseTypedefDeclorList(namescope, tid,
                        decl->m_declor_list.get(), decl->m_align_spec.get(),
                        decl->m_location);
                }
            }
            break;

        case Decl::DECLORLIST:
            CrAnalyseDeclorList(namescope, tid, decl->m_declor_list.get());
            break;

        case Decl::STATIC_ASSERT:
            {
                CR_TypedValue value;
                shared_ptr<CondExpr> const_expr =
                    decl->m_static_assert_decl->m_const_expr;
                value = CrValueOnCondExpr(namescope, const_expr.get());
                if (namescope.GetIntValue(value) == 0) {
                    namescope.ErrorInfo()->add_error(
                        decl->m_location,
                        "static assertion failed");
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
        std::string name;
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
                                         d->m_location);
                d = d->m_declor.get();
                continue;

            case Declor::ARRAY:
                tid2 = namescope.AddPointerType(tid2, 0, d->m_location);
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CR_LogFunc lf;
                    lf.m_return_type = tid2;
                    if (d->m_param_list) {
                        CrAnalyseParamList(namescope, lf, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(lf, d->m_location);
                    d = d->m_declor.get();
                }
                continue;

            case Declor::BITS:
                d = NULL;
                break;

            default:
                assert(0);
                d = NULL;
            }
        }
        func.m_params.emplace_back(tid2, name);
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
                    namescope.AddFuncType(func, declor->m_location);
                } else {
                    assert(0);
                }
            } else {
                assert(declor->m_param_list);
                if (declor->m_param_list) {
                    CrAnalyseParamList(namescope, func, declor->m_param_list.get());
                    namescope.AddFuncType(func, declor->m_location);
                }
            }
        }
    }
}

CR_TypeID CrAnalyseStructDeclList(CR_NameScope& namescope,
                                  const std::string& name, DeclList *dl,
                                  int pack, int alignas_,
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
                ls.m_members.emplace_back(tid, "");
            }
            break;

        case Decl::STATIC_ASSERT:
            {
                auto const_expr = decl->m_static_assert_decl->m_const_expr;
                CR_TypedValue value = 
                    CrValueOnCondExpr(namescope, const_expr.get());
                if (namescope.GetIntValue(value) == 0) {
                    namescope.ErrorInfo()->add_error(
                        decl->m_static_assert_decl->m_location,
                        "static assertion failed");
                }
            }
            break;

        default:
            assert(0);
            return cr_invalid_id;
        }
    }

    return namescope.AddStructType(name, ls, alignas_, location);
}

CR_TypeID CrAnalyseUnionDeclList(CR_NameScope& namescope,
                                 const std::string& name, DeclList *dl,
                                 int pack, int alignas_,
                                 const CR_Location& location)
{
    CR_LogStruct ls(false);     // union
    ls.m_pack = pack;
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
                ls.m_members.emplace_back(tid, "");
            }
            break;

        case Decl::STATIC_ASSERT:
            {
                auto const_expr = decl->m_static_assert_decl->m_const_expr;
                CR_TypedValue value =
                    CrValueOnCondExpr(namescope, const_expr.get());
                if (namescope.GetIntValue(value) == 0) {
                    namescope.ErrorInfo()->add_error(
                        decl->m_static_assert_decl->m_location,
                        "static assertion failed");
                }
            }
            break;

        default:
            assert(0);
            return cr_invalid_id;
        }
    }

    return namescope.AddUnionType(name, ls, alignas_, location);
}

CR_TypeID CrAnalyseEnumorList(CR_NameScope& namescope,
                              const std::string& name, EnumorList *el,
                              const CR_Location& location)
{
    CR_LogEnum le;

    auto const_int_tid = namescope.AddConstIntType();

    int int_value, next_value = 0;
    assert(el);
    for (auto& e : *el) {
        if (e->m_const_expr) {
            CR_TypedValue value;
            value = CrValueOnCondExpr(namescope, e->m_const_expr.get());
            int_value = namescope.GetIntValue(value);
        } else {
            int_value = next_value;
        }

        le.m_mNameToValue[e->m_name.data()] = int_value;
        le.m_mValueToName[int_value] = e->m_name.data();
        namescope.AddVar(e->m_name, const_int_tid, int_value, location);

        next_value = int_value + 1;
    }

    CR_TypeID tid = namescope.AddEnumType(name, le, location);
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
        std::string name;
        switch (ds->m_spec_type) {
        case DeclSpecs::STORCLSSPEC:
            // TODO: ds->m_stor_cls_spec->m_scs_type;
            if (ds->m_decl_specs) {
                ds = ds->m_decl_specs.get();
                continue;
            }
            break;

        case DeclSpecs::FUNCSPEC:
            // TODO: ds->m_func_spec
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
                    CR_TypedValue value;
                    int alignas_ = 0;
                    if (ts->m_align_spec) {
                        switch (ts->m_align_spec->m_align_spec_type) {
                        case AlignSpec::TYPENAME:
                            alignas_ =
                                CrCalcAlignOfTypeName(
                                    namescope, 
                                        ts->m_align_spec->m_type_name.get());
                            break;

                        case AlignSpec::CONSTEXPR:
                            value = 
                                CrValueOnCondExpr(namescope,
                                    ts->m_align_spec->m_const_expr.get());
                            alignas_ = namescope.GetIntValue(value);
                            break;
                        }
                    }
                    if (ts->m_decl_list) {
                        tid = CrAnalyseStructDeclList(
                            namescope, name, ts->m_decl_list.get(), ts->m_pack,
                            alignas_, ts->m_location);
                    } else {
                        tid = namescope.TypeIDFromName(name);
                        if (tid == cr_invalid_id) {
                            CR_LogStruct ls(true);
                            tid = namescope.AddStructType(name, ls, alignas_, ts->m_location);
                        }
                    }
                }
                if (flags & TF_CONST) {
                    tid = namescope.AddConstType(tid);
                }
                return tid;

            case TF_UNION:
                {
                    TypeSpec *ts = ds->m_type_spec.get();
                    name = ts->m_name;
                    int alignas_ = 0;
                    CR_TypedValue value;
                    if (ts->m_align_spec) {
                        switch (ts->m_align_spec->m_align_spec_type) {
                        case AlignSpec::TYPENAME:
                            alignas_ =
                                CrCalcAlignOfTypeName(
                                    namescope, 
                                        ts->m_align_spec->m_type_name.get());
                            break;

                        case AlignSpec::CONSTEXPR:
                            value = 
                                CrValueOnCondExpr(namescope,
                                    ts->m_align_spec->m_const_expr.get());
                            alignas_ = namescope.GetIntValue(value);
                            break;
                        }
                    }
                    if (ts->m_decl_list) {
                        tid = CrAnalyseUnionDeclList(
                            namescope, name, ts->m_decl_list.get(), ts->m_pack,
                            alignas_, ts->m_location);
                    } else {
                        tid = namescope.TypeIDFromName(name);
                        if (tid == cr_invalid_id) {
                            CR_LogStruct ls(false);
                            tid = namescope.AddUnionType(name, ls, alignas_, ts->m_location);
                        }
                    }
                }
                if (flags & TF_CONST) {
                    tid = namescope.AddConstType(tid);
                }
                return tid;

            case TF_ENUM:
                name = ds->m_type_spec->m_name;
                if (ds->m_type_spec->m_enumor_list) {
                    tid = CrAnalyseEnumorList(
                        namescope, name, ds->m_type_spec->m_enumor_list.get(),
                        ds->m_type_spec->m_location);
                } else {
                    CR_LogEnum le;
                    tid = namescope.AddEnumType(
                        name, le, ds->m_type_spec->m_location);
                }
                if (flags & TF_CONST) {
                    tid = namescope.AddConstType(tid);
                }
                return tid;

            case TF_ATOMIC:
                return CrAnalyseAtomic(namescope,
                    ds->m_type_spec->m_atomic_type_spec.get());

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
        }
        break;
    }

    flags = CrNormalizeTypeFlags(flags);
    tid = namescope.TypeIDFromFlags(flags & ~TF_CONST);
    if (flags & TF_CONST) {
        tid = namescope.AddConstType(tid);
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
    std::string ret = str;
    CrReplaceString(ret, "\"", "\"\"");
    if (ret.find_first_of("\" \t\f\v") != std::string::npos) {
        ret = "\"" + str + "\"";
    }
    return ret;
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
            error_info->add_error(
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
            std::string cmdline("gcc -E");
        #elif defined(__clang__)
            std::string cmdline("clang -E");
        #elif defined(_MSC_VER)
            std::string cmdline("cl /nologo /E");
        #else
            #error You lose.
        #endif

        int k = -1;
        for (; i < argc; i++) {
            if (k == -1 && ::GetFileAttributesA(argv[i]) != 0xFFFFFFFF) {
                k = i;
            }
            cmdline += ' ';
            cmdline += CrConvertCmdLineParam(argv[i]);
        }
        std::cerr << "CommandLine: " << cmdline << std::endl;

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
                            error_info->add_error(
                                std::string("cannot write to temporary file'") +
                                    cr_tmpfile + "'");
                            bOK = FALSE;
                            break;
                        }
                    } else {
                        DWORD dwError = ::GetLastError();
                        if (dwError != ERROR_MORE_DATA) {
                            error_info->add_error(
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
                error_info->add_error("failed to run C preprocessor");
                return cr_exit_cpp_error;   // failure
            }
        } else {
            error_info->add_error(
                "failed to start up C preprocessor (error code " +
                    std::to_string(::GetLastError()) + ")"
            );
            return cr_exit_cpp_error;   // failure
        }
    } else {
        error_info->add_error(
            std::string("unknown input file extension '") +
                pchDotExt + "'. please use .i or .h"
        );
        return cr_exit_wrong_ext;   // failure
    }
    return cr_exit_ok;   // success
}

////////////////////////////////////////////////////////////////////////////
// semantic analysis

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

bool CrLoadMacros(
    CR_NameScope& namescope, CR_MacroSet& macro_set,
    const std::string& prefix, const std::string& suffix,
    bool is_64bit = false)
{
    macro_set.clear();

    std::ifstream in1(prefix + "macros" + suffix);
    if (in1) {
        std::string line;
        for (; std::getline(in1, line); ) {
            CrChop(line);
            std::vector<std::string> fields;
            katahiromz::split(fields, line, "\t");

            std::string name = fields[0];
            int num_params = std::stoi(fields[1], NULL, 0);
            std::string params = fields[2];
            std::string contents = fields[3];
            std::string file = fields[4];
            int lineno = std::stol(fields[5], NULL, 0);

            CrTrimString(contents);

            CR_Location location(file, lineno);

            CR_Macro macro;
            macro.m_num_params = num_params;
            katahiromz::split(macro.m_params, params, ",");
            macro.m_contents = contents;
            macro.m_location = location;

            macro_set.emplace(name, macro);
        }
        return true;
    }

    namescope.ErrorInfo()->add_error("cannot load macros");
    return false;
}

std::string
CrExpandMacro(const CR_MacroSet& macros, std::string str) {
    // TODO:
    return str;
}

bool CrParseMacros(
    CR_NameScope& namescope,
    const std::string& prefix, const std::string& suffix,
    bool is_64bit = false)
{
    CR_MacroSet macro_set;
    CR_MacroSet checked;

    // load macros
    if (!CrLoadMacros(namescope, macro_set, prefix, suffix, is_64bit)) {
        return false;
    }

    // get type names
    auto type_names = make_shared<cparser::TypeNames>();
    for (auto it : namescope.MapNameToTypeID()) {
        type_names->emplace(it.first);
    }
    for (auto it : namescope.MapTypeIDToName()) {
        type_names->emplace(it.second);
    }
    type_names->erase("");

    // save error info
    auto error_info_saved = namescope.ErrorInfo();
    namescope.ErrorInfo() = make_shared<CR_ErrorInfo>();

    // for all macros
    auto end = macro_set.end();
    for (auto it = macro_set.begin(); it != end; ++it) {
        auto& name = it->first;
        auto& macro = it->second;

        // ignore macro functions
        if (macro.m_num_params > 0) {
            continue;
        }

        // expand macro
        auto expanded = CrExpandMacro(macro_set, macro.m_contents);
        CrTrimString(expanded);
        if (expanded.empty()) {
            continue;
        }

        // parse as expression
        shared_ptr<Expr> expr;
        shared_ptr<TypeName> type_name;
        if (parse_expression(namescope.ErrorInfo(), expr, 
                             expanded, type_names, namescope.Is64Bit()))
        {
            CR_LogVar var;
            CR_TypedValue typed_value = CrValueOnExpr(namescope, expr.get());
            var.m_location = macro.m_location;
            if (namescope.HasValue(typed_value)) {
                var.m_typed_value = typed_value;

                // add macro as variable
                auto vid = namescope.LogVars().insert(var);
                namescope.MapVarIDToName().emplace(vid, name);
                if (name.size()) {
                    namescope.MapNameToVarID().emplace(name, vid);
                }
            } else {
                // check the macro
                checked.emplace(name, macro);
            }
        } else if (parse_type(namescope.ErrorInfo(), type_name,
                              expanded, type_names, namescope.Is64Bit()))
        {
            // add type
            CR_TypeID tid;
            tid = CrGetTypeIDOfTypeName(namescope, type_name.get());
            if (tid != cr_invalid_id) {
                namescope.AddAliasType(name, tid, macro.m_location);
            }
            type_names->emplace(name);
        } else {
            // check the macro
            checked.emplace(name, macro);
        }
    }

    // retry checked macros
    auto end2 = checked.end();
    for (auto it2 = checked.begin(); it2 != end2; ++it2) {
        auto& name = it2->first;
        auto& macro = it2->second;

        // expand macro
        auto expanded = CrExpandMacro(macro_set, macro.m_contents);
        CrTrimString(expanded);
        if (expanded.empty()) {
            continue;
        }

        shared_ptr<TypeName> type_name;
        if (parse_type(namescope.ErrorInfo(), type_name,
                       expanded, type_names, namescope.Is64Bit()))
        {
            // add type
            CR_TypeID tid;
            tid = CrGetTypeIDOfTypeName(namescope, type_name.get());
            if (tid != cr_invalid_id) {
                namescope.AddAliasType(name, tid, macro.m_location);
            }
            type_names->emplace(name);
            continue;
        } else {
            // add variable mapping
            auto it3 = namescope.MapNameToVarID().find(expanded);
            if (it3 != namescope.MapNameToVarID().end()) {
                CR_Name2Name name2name;
                name2name.m_name1 = name;
                name2name.m_name2 = expanded;
                name2name.m_location = macro.m_location;
                namescope.MapNameToName().emplace(name, name2name);
                #if 0
                    std::cerr << name << "::: " << macro.m_contents << std::endl;
                #endif
                continue;
            } else {
                #if 0
                    std::cerr << name << ":: " << macro.m_contents << std::endl;
                #endif
            }
        }
        #if 0
            std::cerr << name << ": " << macro.m_contents << std::endl;
        #endif
    }

    namescope.ErrorInfo() = error_info_saved;
    return true;
}

int CrSemanticAnalysis(CR_NameScope& namescope, shared_ptr<TransUnit>& tu) {
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
                        if (decl->m_constant.size()) {
                            int vector_size = std::stoul(decl->m_constant, NULL, 0);
                            CrAnalyseTypedefDeclorListVector(namescope, tid,
                                decl->m_declor_list.get(), decl->m_align_spec.get(),
                                vector_size, decl->m_location);
                        } else {
                            CrAnalyseTypedefDeclorList(
                                namescope, tid, dl.get(),
                                decl->m_align_spec.get(),
                                decl->m_location);
                        }
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
    const std::string& strPrefix, const std::string& strSuffix)
{
    namescope.SaveToFiles(strPrefix, strSuffix);
}

////////////////////////////////////////////////////////////////////////////

void CrShowHelp(void) {
    std::cerr <<
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
    " Usage: cparser64 [options] [input-file.h [compiler_options]]\n"
#else
    " Usage: cparser [options] [input-file.h [compiler_options]]\n"
#endif
    "\n"
    "Options:\n"
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
    " -32               32-bit mode\n"
    " -64               64-bit mode (default)\n"
#else
    " -32               32-bit mode (default)\n"
    " -64               64-bit mode\n"
#endif
    "--nologo           Don't show logo.\n"
    "--version          Show version only.\n"
    "--prefix prefix    The prefix of output file names (default is empty).\n"
    "--suffix suffix    The suffix of output file names (default is '.dat')."
    << std::endl;
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

    #if 0
        printf("Hit [Enter] key!");
        getchar();
    #endif

    bool show_help = false;
    bool show_version = false;
    bool no_logo = false;
    std::string strPrefix, strSuffix = ".dat";
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
                    std::cerr << "ERROR: Invalid option '" <<
                        argv[i] << "'." << std::endl;
                    return cr_exit_invalid_option;
                } else {
                    std::cerr << "ERROR: File '" << argv[i] <<
                        "' doesn't exist." << std::endl;
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

    int ret = cr_exit_ok;

    shared_ptr<TransUnit> tu;
    auto error_info = make_shared<CR_ErrorInfo>();
    if (i < argc) {
        std::cerr << "Parsing headers..." << std::endl;
        // argv[i] == input-file
        // argv[i + 1] == compiler option #1
        // argv[i + 2] == compiler option #2
        // argv[i + 3] == ...
        ret = CrInputCSrc(error_info, tu, i, argc, argv, is_64bit);
    } else {
        error_info->add_error("no input files");
        ret = cr_exit_no_input;
    }

    if (ret) {
        error_info->emit_all();
        return ret;
    }

    if (tu) {
        std::cerr << "Semantic analysis..." << std::endl;
        if (is_64bit) {
            CR_NameScope namescope(error_info, true);

            ret = CrSemanticAnalysis(namescope, tu);
            tu = shared_ptr<TransUnit>();
            if (ret == 0) {
                std::cerr << "Parsing macros..." << std::endl;
                CrParseMacros(namescope, strPrefix, strSuffix, true);
                std::cerr << "Dumping..." << std::endl;
                CrDumpSemantic(namescope, strPrefix, strSuffix);
            }
        } else {
            CR_NameScope namescope(error_info, false);

            ret = CrSemanticAnalysis(namescope, tu);
            tu = shared_ptr<TransUnit>();
            if (ret == 0) {
                std::cerr << "Parsing macros..." << std::endl;
                CrParseMacros(namescope, strPrefix, strSuffix, false);
                std::cerr << "Dumping..." << std::endl;
                CrDumpSemantic(namescope, strPrefix, strSuffix);
            }
        }
    }

    error_info->emit_all();

    if (error_info->errors().empty()) {
        std::cerr << "Done." << std::endl;
    } else {
        if (error_info->warnings().size()) {
            std::cerr << error_info->errors().size() << " errors, " <<
                         error_info->warnings().size() << " warnings." << std::endl;
        } else {
            std::cerr << error_info->errors().size() << " errors." << std::endl;
        }
        return cr_exit_parse_error;
    }

    return ret;
}

////////////////////////////////////////////////////////////////////////////
