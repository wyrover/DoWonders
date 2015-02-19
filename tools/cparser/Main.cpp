#include "stdafx.h"
#include "CParseHeader.h"

////////////////////////////////////////////////////////////////////////////

const char * const cr_logo =
    "///////////////////////////////////////////////\n"
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
# ifdef __GNUC__
    "// CParser sample 0.2.2 (64-bit) for gcc     //\n"
# elif defined(__clang__)
    "// CParser sample 0.2.2 (64-bit) for clang    //\n"
# elif defined(_MSC_VER)
    "// CParser sample 0.2.2 (64-bit) for cl      //\n"
# endif
#else   // !64-bit
# ifdef __GNUC__
    "// CParser sample 0.2.2 (32-bit) for gcc     //\n"
# elif defined(__clang__)
    "// CParser sample 0.2.2 (32-bit) for clang    //\n"
# elif defined(_MSC_VER)
    "// CParser sample 0.2.2 (32-bit) for cl      //\n"
# endif
#endif  // !64-bit
    "// public domain software (PDS)              //\n"
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
// cparser::Scanner

void cparser::Scanner::resynth1(ScannerBase& base, token_container& c) {
    token_container     newc;

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
                CR_ErrorInfo::Type type = parse_pragma(base, it, end);
                add(type, base.location(), "unknown pragma found");
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

void cparser::Scanner::resynth2(token_container& c) {
    token_container newc;
    token_iterator it, it2, end = c.end();
    for (it = c.begin(); it != end; ++it) {
        if (it->m_token == T_R_PAREN && (it + 1)->m_token == T_ASM) {
            // int func() __asm__("..." "...");
            it = skip_asm_for_fn_decl(it + 1, end);
            if (it == end)
                break;
        }

        if (it->m_token == T_GNU_ATTRIBUTE) {
            it2 = it;
            it = skip_gnu_attribute(it, end);
            if (it != end) {
                ++it2;  // T_GNU_ATTRIBUTE
                ++it2;  // T_L_PAREN
                ++it2;  // T_L_PAREN
                switch (it2->m_token) {
                case T_CDECL: case T_STDCALL: case T_FASTCALL:
                    newc.push_back(*it2);
                    break;

                default:
                    break;
                }
            }
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

void cparser::Scanner::resynth3(token_iterator begin, token_iterator end) {
    m_type_names.clear();
    #ifdef __GNUC__
        m_type_names.insert("__builtin_va_list");   // fixup
    #else
        m_type_names.insert("va_list");
        m_type_names.insert("SOCKADDR_STORAGE");    // fixup
    #endif

    for (token_iterator it = begin; it != end; ++it) {
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

    for (token_iterator it = begin; it != end; ++it) {
        if (it->m_token == T_TYPEDEF) {
            it = resynth_typedef(++it, end);
        } else if (it->m_token == T_IDENTIFIER) {
            if (m_type_names.count(it->m_text)) {
                it->set_token(T_TYPEDEF_NAME);
            }
        }
    }
} // resynth3

cparser::token_iterator
cparser::Scanner::resynth_typedef(token_iterator begin, token_iterator end) {
    int paren_nest = 0, brace_nest = 0, bracket_nest = 0;
    token_iterator it;
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
            } else
                --it;
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

cparser::token_iterator
cparser::Scanner::resynth_parameter_list(
    token_iterator begin, token_iterator end)
{
    int paren_nest = 1;
    bool fresh = true;
    token_iterator it;
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

cparser::token_iterator
cparser::Scanner::skip_gnu_attribute(
    token_iterator begin, token_iterator end)
{
    token_iterator it = begin;
    if (it != end && it->m_token == T_GNU_ATTRIBUTE)
        ++it;

    if (it != end && it->m_token == T_L_PAREN) {
        ++it;
        int paren_nest = 1;
        for (; it != end; ++it) {
            if (it->m_token == T_L_PAREN) {
                paren_nest++;
            } else if (it->m_token == T_R_PAREN) {
                paren_nest--;
                if (paren_nest == 0)
                    break;
            }
        }
    }
    return it;
} // skip_gnu_attribute

cparser::token_iterator
cparser::Scanner::skip_asm_for_fn_decl(
    token_iterator begin, token_iterator end)
{
    token_iterator it = begin;
    if (it != end && it->m_token == T_ASM)
        ++it;

    if (it != end && it->m_token == T_L_PAREN) {
        ++it;
        int paren_nest = 1;
        for (; it != end; ++it) {
            if (it->m_token == T_L_PAREN) {
                paren_nest++;
            } else if (it->m_token == T_R_PAREN) {
                paren_nest--;
                if (paren_nest == 0)
                    break;
            }
        }
    }
    return it;
} // skip_asm_for_fn_decl

void cparser::Scanner::resynth4(token_container& c) {
    token_container newc;
    token_iterator it, it2, end = c.end();
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

void cparser::Scanner::resynth5(token_iterator begin, token_iterator end) {
    token_iterator it, paren_it, it2;
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

cparser::Token
cparser::Scanner::parse_identifier(const std::string& text) const {
    Token token = T_IDENTIFIER;
    char c, d;
    c = text[0];
    if (c == '_') {
        if (text.size() >= 2 && text[1] != '_') {
            d = text[1];
            if (d == 'A' && text == "_Alignas") token = T_ALIGNAS;
            else if (d == 'A' && text == "_Alignof") token = T_ALIGNOF;
            else if (d == 'A' && text == "_Atomic") token = T_ATOMIC;
            else if (d == 'B' && text == "_Bool") token = T_BOOL;
            else if (d == 'C' && text == "_Complex") token = T_COMPLEX;
            else if (d == 'G' && text == "_Generic") token = T_GENERIC;
            else if (d == 'I' && text == "_Imaginary") token = T_IMAGINARY;
            else if (d == 'N' && text == "_Noreturn") token = T_NORETURN;
            else if (d == 'S' && text == "_Static_assert") token = T_STATIC_ASSERT;
            else if (d == 'T' && text == "_Thread_local") token = T_THREAD_LOCAL;
        } else if (text.size() >= 3) {
            d = text[2];
            if (d == 'a' && text == "__asm") token = T_ASM;
            else if (d == 'a' && text == "__asm__") token = T_ASM;
            else if (d == 'a' && text == "__attribute__") token = T_GNU_ATTRIBUTE;
            else if (d == 'c' && text == "__cdecl") token = T_CDECL;
            else if (d == 'c' && text == "__cdecl__") token = T_CDECL;
            else if (d == 'c' && text == "__const__") token = T_CONST;
            else if (d == 'd' && text == "__declspec") token = T_DECLSPEC;
            else if (d == 'e' && text == "__extension__") token = T_GNU_EXTENSION;
            else if (d == 'f' && text == "__fastcall") token = T_FASTCALL;
            else if (d == 'f' && text == "__fastcall__") token = T_FASTCALL;
            else if (d == 'f' && text == "__forceinline") token = T_FORCEINLINE;
            else if (d == 'i' && text == "__inline") token = T_INLINE;
            else if (d == 'i' && text == "__inline__") token = T_INLINE;
            else if (d == 'i' && text == "__int32") token = T_INT32;
            else if (d == 'i' && text == "__int64") token = T_INT64;
            else if (d == 'i' && text == "__int128") token = T_INT128;
            else if (d == 'n' && text == "__noreturn__") token = T_NORETURN;
            else if (d == 'n' && text == "__nothrow__") token = T_NOTHROW;
            else if (d == 'p' && text == "__pragma") token = T_PRAGMA;
            else if (d == 'p' && text == "__ptr32") token = T_PTR32;
            else if (d == 'p' && text == "__ptr64") token = T_PTR64;
            else if (d == 'r' && text == "__restrict") token = T_RESTRICT;
            else if (d == 'r' && text == "__restrict__") token = T_RESTRICT;
            else if (d == 's' && text == "__signed__") token = T_SIGNED;
            else if (d == 's' && text == "__stdcall") token = T_STDCALL;
            else if (d == 's' && text == "__stdcall__") token = T_STDCALL;
            else if (d == 'u' && text == "__unaligned") token = T_UNALIGNED;
            else if (d == 'v' && text == "__volatile__") token = T_VOLATILE;
            else if (d == 'w' && text == "__w64") token = T_W64;
        }
    } else if (c < 'd') {
        if (c == 'a' && text == "asm") token = T_ASM;
        else if (c == 'a' && text == "auto") token = T_AUTO;
        else if (c == 'b' && text == "break") token = T_BREAK;
        else if (c == 'c' && text == "case") token = T_CASE;
        else if (c == 'c' && text == "char") token = T_CHAR;
        else if (c == 'c' && text == "const") token = T_CONST;
        else if (c == 'c' && text == "continue") token = T_CONTINUE;
    } else if (c < 'f') {
        if (c == 'd' && text == "default") token = T_DEFAULT;
        else if (c == 'd' && text == "do") token = T_DO;
        else if (c == 'd' && text == "double") token = T_DOUBLE;
        else if (c == 'e' && text == "else") token = T_ELSE;
        else if (c == 'e' && text == "enum") token = T_ENUM;
        else if (c == 'e' && text == "extern") token = T_EXTERN;
    } else if (c < 's') {
        if (c == 'f' && text == "float") token = T_FLOAT;
        else if (c == 'f' && text == "for") token = T_FOR;
        else if (c == 'g' && text == "goto") token = T_GOTO;
        else if (c == 'i' && text == "if") token = T_IF;
        else if (c == 'i' && text == "inline") token = T_INLINE;
        else if (c == 'i' && text == "int") token = T_INT;
        else if (c == 'l' && text == "long") token = T_LONG;
        else if (c == 'n' && text == "noreturn") token = T_NORETURN;
        else if (c == 'r' && text == "register") token = T_REGISTER;
        else if (c == 'r' && text == "restrict") token = T_RESTRICT;
        else if (c == 'r' && text == "return") token = T_RETURN;
    } else {
        if (c == 's' && text == "short") token = T_SHORT;
        else if (c == 's' && text == "signed") token = T_SIGNED;
        else if (c == 's' && text == "sizeof") token = T_SIZEOF;
        else if (c == 's' && text == "static") token = T_STATIC;
        else if (c == 's' && text == "struct") token = T_STRUCT;
        else if (c == 's' && text == "switch") token = T_SWITCH;
        else if (c == 't' && text == "typedef") token = T_TYPEDEF;
        else if (c == 'u' && text == "union") token = T_UNION;
        else if (c == 'u' && text == "unsigned") token = T_UNSIGNED;
        else if (c == 'v' && text == "void") token = T_VOID;
        else if (c == 'v' && text == "volatile") token = T_VOLATILE;
        else if (c == 'w' && text == "while") token = T_WHILE;
        else if (c == 'x' && text == "xsigned") token = T_XSIGNED;
    }
    return token;
} // parse_identifier

CR_ErrorInfo::Type
cparser::Scanner::parse_pragma(
    ScannerBase& base, token_iterator& it, token_iterator end)
{
    if (it == end) {
        return CR_ErrorInfo::NOTHING;
    }
    // #pragma name...
    auto name = it->m_text;
    ++it;
    if (name == "message") {
        // #pragma message("...")
        bool flag = token_pattern_match(it, end,
            {T_L_PAREN, T_STRING, T_R_PAREN}
        );
        if (flag) {
            message(base.location(), (it + 1)->m_text);
            it += 3;
        }
        return CR_ErrorInfo::NOTHING;
    }
    if (name == "pack") {
        // #pragma pack...
        bool flag = token_pattern_match(it, end,
            {T_L_PAREN, T_R_PAREN}
        );
        if (flag) {
            // #pragma pack()
            base.packing().set_pack(); // default value
            it += 2;
            return CR_ErrorInfo::NOTHING;
        }
        flag = token_pattern_match(it, end,
            {T_L_PAREN, eof, T_R_PAREN}
        );
        if (flag) {
            if ((it + 1)->m_text == "pop") {
                // #pragma pack(pop)
                base.packing().pop_pack();
            } else {
                // #pragma pack(#)
                int pack = std::strtol((it + 1)->m_text.data(), NULL, 0);
                base.packing().set_pack(pack);
            }
            it += 3;
            return CR_ErrorInfo::NOTHING;
        }
        flag = token_pattern_match(it, end,
            {T_L_PAREN, eof, T_COMMA, eof, T_R_PAREN}
        );
        if (flag) {
            // #pragma pack(push, #)
            if ((it + 1)->m_text == "push") {
                int pack = std::strtol((it + 3)->m_text.data(), NULL, 0);
                assert(pack != 0);
                base.packing().push_pack(pack);
                it += 5;
                return CR_ErrorInfo::NOTHING;
            }
        }
        flag = token_pattern_match(it, end,
            {T_L_PAREN, eof, T_COMMA, eof, T_COMMA, eof, T_R_PAREN}
        );
        if (flag) {
            // #pragma pack(push, #, #)
            if ((it + 1)->m_text == "push") {
                // TODO & FIXME
                return CR_ErrorInfo::NOTICE;
            }
        }
    }
    if (name == "comment") {
        bool flag = token_pattern_match(it, end,
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
    return CR_ErrorInfo::ERR;
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
                assert(ls.m_struct_or_union);   // must be struct
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
    shared_ptr<TransUnit>& tu,
    int i, int argc, char **argv, bool is_64bit)
{
    char *pchDotExt = strrchr(argv[i], '.');
    // if file extension is ".i",
    if (strcmp(pchDotExt, ".i") == 0) {
        // directly parse
        if (!cparser::parse_file(tu, argv[i], is_64bit)) {
            fprintf(stderr, "ERROR: Failed to parse file '%s'\n", argv[i]);
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
                            fprintf(stderr,
                                "ERROR: Cannot write to temporary file '%s'\n",
                                cr_tmpfile);
                            bOK = FALSE;
                            break;
                        }
                    } else {
                        DWORD dwError = ::GetLastError();
                        if (dwError != ERROR_MORE_DATA) {
                            fprintf(stderr, "ERROR: Cannot read output\n");
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
                if (!cparser::parse_file(tu, cr_tmpfile, is_64bit)) {
                    return cr_exit_parse_error;   // failure
                }
            } else {
                return cr_exit_cpp_error;   // failure
            }
        } else {
            return cr_exit_cpp_error;   // failure
        }
    } else {
        fprintf(stderr,
            "ERROR: Unknown input file extension '%s'. Please use .i or .h\n",
            pchDotExt);
        return cr_exit_wrong_ext;   // failure
    }
    return cr_exit_ok;   // success
}

////////////////////////////////////////////////////////////////////////////
// semantic analysis

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
        fprintf(fp, "(type_id)\t(name)\t(flags)\t(sub_id)\t(count)\t(size)\t(file)\t(line)\t(definition)\n");
        for (CR_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
            const auto& name = namescope.MapTypeIDToName()[tid];
            const auto& type = namescope.LogType(tid);
            if (namescope.IsCrExtendedType(tid)) {
                #ifdef _DEBUG
                    size_t size = namescope.SizeOfType(tid);
                    fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t(cr_extended)\t0\t(cr_extended)\n",
                        static_cast<int>(tid), name.data(), type.m_flags,
                        static_cast<int>(type.m_sub_id), 0, static_cast<int>(size));
                #endif
            } else if (namescope.IsPredefinedType(tid)) {
                size_t size = namescope.SizeOfType(tid);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t(predefined)\t0\t(predefined)\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_sub_id), 0, static_cast<int>(size));
            } else if (type.m_flags & (TF_STRUCT | TF_UNION | TF_ENUM)) {
                auto strDef = namescope.StringOfType(tid, "", true);
                const auto& location = type.location();
                size_t size = type.m_size;
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%s\t%d\t%s;\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_sub_id), 0, static_cast<int>(size),
                    location.m_file.data(), location.m_line, strDef.data());
            } else if (type.m_flags & (TF_POINTER | TF_FUNCTION | TF_ARRAY)) {
                const auto& location = type.location();
                size_t size = type.m_size;
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%s\t%d\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_sub_id),
                    static_cast<int>(type.m_count), static_cast<int>(size),
                    location.m_file.data(), location.m_line);
            } else {
                auto strDef = namescope.StringOfType(tid, name, true);
                const auto& location = type.location();
                size_t size = namescope.SizeOfType(tid);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%s\t%d\ttypedef %s;\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_sub_id), 0, static_cast<int>(size),
                    location.m_file.data(), location.m_line, strDef.data());
            }
        }
        fclose(fp);
    }

    fp = fopen((strPrefix + "structures" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(type_id)\t(name)\t(struct_id)\t(struct_or_union)\t(size)\t(count)\t(pack)\t(align)\t(file)\t(line)\t(definition)\t(item_1_name)\t(item_1_type_id)\t(item_1_offset)\t(item_1_bits)\t(item_2_type_id)\t...\n");
        for (CR_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
            const auto& type = namescope.LogType(tid);
            if (!(type.m_flags & (TF_STRUCT | TF_UNION))) {
                continue;
            }
            const auto& name = namescope.MapTypeIDToName()[tid];
            auto strDef = namescope.StringOfType(tid, "");
            assert(!strDef.empty());
            const auto& location = type.location();
            size_t size = namescope.SizeOfType(tid);
            auto sid = type.m_sub_id;
            const auto& ls = namescope.LogStruct(sid);
            assert(ls.m_type_list.size() == ls.m_name_list.size());
            fprintf(fp, "%d\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s;",
                static_cast<int>(tid), name.data(), 
                static_cast<int>(sid), static_cast<int>(ls.m_struct_or_union),
                static_cast<int>(size), static_cast<int>(ls.m_type_list.size()),
                static_cast<int>(ls.m_pack), static_cast<int>(ls.m_align),
                location.m_file.data(), location.m_line, strDef.data());
            if (type.m_flags & TF_UNION) {
                for (size_t i = 0; i < ls.m_type_list.size(); ++i) {
                    fprintf(fp, "\t%s\t%d\t0\t0",
                        ls.m_name_list[i].data(),
                        static_cast<int>(ls.m_type_list[i]));
                }
            } else {
                assert(ls.m_type_list.size() == ls.m_offset_list.size());
                for (size_t i = 0; i < ls.m_type_list.size(); ++i) {
                    fprintf(fp, "\t%s\t%d\t%d\t%d",
                        ls.m_name_list[i].data(),
                        static_cast<int>(ls.m_type_list[i]),
                        static_cast<int>(ls.m_offset_list[i]),
                        static_cast<int>(ls.m_bits_list[i]));
                }
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
        fprintf(fp, "(name)\t(enum_type_id)\t(value)\t(enum_name)\t(file)\t(line)\n");
        const auto& vars = namescope.Vars();
        for (CR_VarID i = 0; i < vars.size(); ++i) {
            const auto& var = vars[i];
            const auto& type = namescope.LogType(var.m_type_id);
            if (var.m_has_value && (type.m_flags & TF_ENUMITEM)) {
                const auto& name = namescope.MapVarIDToName()[i];
                const auto& enum_name = namescope.MapTypeIDToName()[var.m_enum_type_id];
                const auto& location = var.location();
                fprintf(fp, "%s\t%d\t%d\t%s\t%s\t%d\n",
                    name.data(), static_cast<int>(var.m_enum_type_id), var.m_int_value,
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

    fprintf(stderr, "Parsing...\n");
    shared_ptr<TransUnit> tu;
    if (i < argc) {
        // argv[i] == input-file
        // argv[i + 1] == compiler option #1
        // argv[i + 2] == compiler option #2
        // argv[i + 3] == ...
        int result = CrInputCSrc(tu, i, argc, argv, is_64bit);
        if (result)
            return result;
    } else {
        fprintf(stderr, "ERROR: No input files.\n");
        return cr_exit_no_input;
    }

    if (tu) {
        fprintf(stderr, "Semantic analysis...\n");
        if (is_64bit) {
            CR_NameScope namescope(true);

            int result = CrSemanticAnalysis(namescope, tu);
            if (result)
                return result;

            tu = shared_ptr<TransUnit>();
            CrDumpSemantic(namescope, strPrefix, strSuffix);
        } else {
            CR_NameScope namescope(false);

            int result = CrSemanticAnalysis(namescope, tu);
            if (result)
                return result;

            tu = shared_ptr<TransUnit>();
            CrDumpSemantic(namescope, strPrefix, strSuffix);
        }
    }

    fprintf(stderr, "Done.\n");

    return cr_exit_ok;
}

////////////////////////////////////////////////////////////////////////////
