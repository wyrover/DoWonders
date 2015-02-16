#include "stdafx.h"
#include "CParseHeader.h"

////////////////////////////////////////////////////////////////////////////

const char * const cr_logo =
    "///////////////////////////////////////////////\n"
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
# ifdef __GNUC__
    "// CParser sample 0.2.1 (64-bit) for gcc     //\n"
# elif defined(__clang__)
    "// CParser sample 0.2.1 (64-bit) for clang    //\n"
# elif defined(_MSC_VER)
    "// CParser sample 0.2.1 (64-bit) for cl      //\n"
# endif
#else   // !64-bit
# ifdef __GNUC__
    "// CParser sample 0.2.1 (32-bit) for gcc     //\n"
# elif defined(__clang__)
    "// CParser sample 0.2.1 (32-bit) for clang    //\n"
# elif defined(_MSC_VER)
    "// CParser sample 0.2.1 (32-bit) for cl      //\n"
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

void CpDeleteTempFileAtExit(void) {
    if (cr_tmpfile) {
        std::remove(cr_tmpfile);
        cr_tmpfile = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////

using namespace cparser;

////////////////////////////////////////////////////////////////////////////
// CpCalcConstInt...Expr functions

int CpCalcConstIntPrimExpr(CP_NameScope& namescope, PrimExpr *pe);
int CpCalcConstIntPostfixExpr(CP_NameScope& namescope, PostfixExpr *pe);
int CpCalcConstIntUnaryExpr(CP_NameScope& namescope, UnaryExpr *ue);
int CpCalcConstIntCastExpr(CP_NameScope& namescope, CastExpr *ce);
int CpCalcConstIntMulExpr(CP_NameScope& namescope, MulExpr *me);
int CpCalcConstIntAddExpr(CP_NameScope& namescope, AddExpr *ae);
int CpCalcConstIntShiftExpr(CP_NameScope& namescope, ShiftExpr *se);
int CpCalcConstIntRelExpr(CP_NameScope& namescope, RelExpr *re);
int CpCalcConstIntEqualExpr(CP_NameScope& namescope, EqualExpr *ee);
int CpCalcConstIntAndExpr(CP_NameScope& namescope, AndExpr *ae);
int CpCalcConstIntExclOrExpr(CP_NameScope& namescope, ExclOrExpr *eoe);
int CpCalcConstIntInclOrExpr(CP_NameScope& namescope, InclOrExpr *ioe);
int CpCalcConstIntLogAndExpr(CP_NameScope& namescope, LogAndExpr *lae);
int CpCalcConstIntLogOrExpr(CP_NameScope& namescope, LogOrExpr *loe);
int CpCalcConstIntAssignExpr(CP_NameScope& namescope, AssignExpr *ae);
int CpCalcConstIntExpr(CP_NameScope& namescope, Expr *e);
int CpCalcConstIntCondExpr(CP_NameScope& namescope, CondExpr *ce);

int CpCalcConstIntPrimExpr(CP_NameScope& namescope, PrimExpr *pe) {
    int n;
    switch (pe->m_prim_type) {
    case PrimExpr::IDENTIFIER:
        n = namescope.GetIntValueFromVarName(pe->m_text);
        return n;

    case PrimExpr::F_CONSTANT:
        return 0;

    case PrimExpr::I_CONSTANT:
        n = std::atoi(pe->m_text.data());
        return n;

    case PrimExpr::STRING:
        return 1;

    case PrimExpr::PAREN:
        n = CpCalcConstIntExpr(namescope, pe->m_expr.get());
        return n;

    case PrimExpr::SELECTION:
        // TODO:
        break;

    default:
        assert(0);
    }
    return 0;
}

int CpCalcConstIntPostfixExpr(CP_NameScope& namescope, PostfixExpr *pe) {
    int n;
    switch (pe->m_postfix_type) {
    case PostfixExpr::SINGLE:
        n = CpCalcConstIntPrimExpr(namescope, pe->m_prim_expr.get());
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
        n = CpCalcConstIntPostfixExpr(namescope, pe->m_postfix_expr.get());
        return n;

    case PostfixExpr::DEC:
        n = CpCalcConstIntPostfixExpr(namescope, pe->m_postfix_expr.get());
        return n;

    default:
        assert(0);
    }
    return 0;
}

int CpCalcSizeOfUnaryExpr(CP_NameScope& namescope, UnaryExpr *ue) {
    return 0;
}

CP_TypeID CpAnalyseDeclSpecs(CP_NameScope& namescope, DeclSpecs *ds);

size_t CpCalcSizeOfTypeName(CP_NameScope& namescope, TypeName *tn) {
    CP_TypeID tid = CpAnalyseDeclSpecs(namescope, tn->m_decl_specs.get());
    if (tn->m_declor) {
        switch (tn->m_declor->m_declor_type) {
        case Declor::POINTERS:
        case Declor::FUNCTION:
            return (namescope.Is64Bit() ? 8 : 4);

        case Declor::ARRAY:
            {
                int count = CpCalcConstIntCondExpr(
                    namescope, tn->m_declor->m_const_expr.get());
                return namescope.GetSizeofType(tid) * count;
            }

        case Declor::BITS:
            return 0;

        default:
            break;
        }
    }
    return namescope.GetSizeofType(tid);
}

int CpCalcConstIntUnaryExpr(CP_NameScope& namescope, UnaryExpr *ue) {
    int n;
    switch (ue->m_unary_type) {
    case UnaryExpr::SINGLE:
        n = CpCalcConstIntPostfixExpr(namescope, ue->m_postfix_expr.get());
        return n;

    case UnaryExpr::INC:
        n = CpCalcConstIntUnaryExpr(namescope, ue->m_unary_expr.get());
        return ++n;

    case UnaryExpr::DEC:
        n = CpCalcConstIntUnaryExpr(namescope, ue->m_unary_expr.get());
        return --n;

    case UnaryExpr::AND:
        return 0;

    case UnaryExpr::ASTERISK:
        return 0;

    case UnaryExpr::PLUS:
        n = CpCalcConstIntCastExpr(namescope, ue->m_cast_expr.get());
        return n;

    case UnaryExpr::MINUS:
        n = CpCalcConstIntCastExpr(namescope, ue->m_cast_expr.get());
        return n;

    case UnaryExpr::BITWISE_NOT:
        n = CpCalcConstIntCastExpr(namescope, ue->m_cast_expr.get());
        return ~n;

    case UnaryExpr::NOT:
        n = CpCalcConstIntCastExpr(namescope, ue->m_cast_expr.get());
        return !n;

    case UnaryExpr::SIZEOF1:
        n = static_cast<int>(CpCalcSizeOfUnaryExpr(namescope, ue->m_unary_expr.get()));
        return n;

    case UnaryExpr::SIZEOF2:
        n = static_cast<int>(CpCalcSizeOfTypeName(namescope, ue->m_type_name.get()));
        return n;

    default:
        assert(0);
    }
    return 0;
}

int CpCalcConstIntCastExpr(CP_NameScope& namescope, CastExpr *ce) {
    int result = 0;
    switch (ce->m_cast_type) {
    case CastExpr::UNARY:
        result = CpCalcConstIntUnaryExpr(namescope, ce->m_unary_expr.get());
        break;
    
    case CastExpr::INITERLIST:
        // TODO:
        //ce->m_type_name
        //ce->m_initer_list
        break;

    case CastExpr::CAST:
        //ce->m_type_name
        result = CpCalcConstIntCastExpr(namescope, ce->m_cast_expr.get());
        break;

    default:
        assert(0);
    }
    return result;
}

int CpCalcConstIntMulExpr(CP_NameScope& namescope, MulExpr *me) {
    int n1, n2, result = 0;
    switch (me->m_mul_type) {
    case MulExpr::SINGLE:
        result = CpCalcConstIntCastExpr(namescope, me->m_cast_expr.get());
        break;

    case MulExpr::ASTERISK:
        n1 = CpCalcConstIntMulExpr(namescope, me->m_mul_expr.get());
        n2 = CpCalcConstIntCastExpr(namescope, me->m_cast_expr.get());
        result = (n1 * n2);
        break;

    case MulExpr::SLASH:
        n1 = CpCalcConstIntMulExpr(namescope, me->m_mul_expr.get());
        n2 = CpCalcConstIntCastExpr(namescope, me->m_cast_expr.get());
        result = (n1 / n2);
        break;

    case MulExpr::PERCENT:
        n1 = CpCalcConstIntMulExpr(namescope, me->m_mul_expr.get());
        n2 = CpCalcConstIntCastExpr(namescope, me->m_cast_expr.get());
        result = (n1 % n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CpCalcConstIntAddExpr(CP_NameScope& namescope, AddExpr *ae) {
    int n1, n2, result = 0;
    switch (ae->m_add_type) {
    case AddExpr::SINGLE:
        result = CpCalcConstIntMulExpr(namescope, ae->m_mul_expr.get());
        break;

    case AddExpr::PLUS:
        n1 = CpCalcConstIntAddExpr(namescope, ae->m_add_expr.get());
        n2 = CpCalcConstIntMulExpr(namescope, ae->m_mul_expr.get());
        result = (n1 + n2);
        break;

    case AddExpr::MINUS:
        n1 = CpCalcConstIntAddExpr(namescope, ae->m_add_expr.get());
        n2 = CpCalcConstIntMulExpr(namescope, ae->m_mul_expr.get());
        result = (n1 - n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CpCalcConstIntShiftExpr(CP_NameScope& namescope, ShiftExpr *se) {
    int n1, n2, result = 0;
    switch (se->m_shift_type) {
    case ShiftExpr::SINGLE:
        result = CpCalcConstIntAddExpr(namescope, se->m_add_expr.get());
        break;

    case ShiftExpr::L_SHIFT:
        n1 = CpCalcConstIntShiftExpr(namescope, se->m_shift_expr.get());
        n2 = CpCalcConstIntAddExpr(namescope, se->m_add_expr.get());
        result = (n1 << n2);
        break;

    case ShiftExpr::R_SHIFT:
        n1 = CpCalcConstIntShiftExpr(namescope, se->m_shift_expr.get());
        n2 = CpCalcConstIntAddExpr(namescope, se->m_add_expr.get());
        result = (n1 >> n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CpCalcConstIntRelExpr(CP_NameScope& namescope, RelExpr *re) {
    int n1, n2, result = 0;
    switch (re->m_rel_type) {
    case RelExpr::SINGLE:
        result = CpCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        break;

    case RelExpr::LT:
        n1 = CpCalcConstIntRelExpr(namescope, re->m_rel_expr.get());
        n2 = CpCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        result = (n1 < n2);
        break;

    case RelExpr::GT:
        n1 = CpCalcConstIntRelExpr(namescope, re->m_rel_expr.get());
        n2 = CpCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        result = (n1 > n2);
        break;

    case RelExpr::LE:
        n1 = CpCalcConstIntRelExpr(namescope, re->m_rel_expr.get());
        n2 = CpCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        result = (n1 <= n2);
        break;

    case RelExpr::GE:
        n1 = CpCalcConstIntRelExpr(namescope, re->m_rel_expr.get());
        n2 = CpCalcConstIntShiftExpr(namescope, re->m_shift_expr.get());
        result = (n1 >= n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CpCalcConstIntEqualExpr(CP_NameScope& namescope, EqualExpr *ee) {
    int n1, n2, result = 0;
    switch (ee->m_equal_type) {
    case EqualExpr::SINGLE:
        result = CpCalcConstIntRelExpr(namescope, ee->m_rel_expr.get());
        break;

    case EqualExpr::EQUAL:
        n1 = CpCalcConstIntEqualExpr(namescope, ee->m_equal_expr.get());
        n2 = CpCalcConstIntRelExpr(namescope, ee->m_rel_expr.get());
        result = (n1 == n2);
        break;

    case EqualExpr::NE:
        n1 = CpCalcConstIntEqualExpr(namescope, ee->m_equal_expr.get());
        n2 = CpCalcConstIntRelExpr(namescope, ee->m_rel_expr.get());
        result = (n1 != n2);
        break;

    default:
        assert(0);
    }
    return result;
}

int CpCalcConstIntAndExpr(CP_NameScope& namescope, AndExpr *ae) {
    int result = CpCalcConstIntEqualExpr(namescope, (*ae)[0].get());
    for (std::size_t i = 1; i < ae->size(); ++i) {
        result &= CpCalcConstIntEqualExpr(namescope, (*ae)[i].get());
    }
    return result;
}

int CpCalcConstIntExclOrExpr(CP_NameScope& namescope, ExclOrExpr *eoe) {
    int result = 0;
    for (auto& ae : *eoe) {
        result ^= CpCalcConstIntAndExpr(namescope, ae.get());
    }
    return result;
}

int CpCalcConstIntInclOrExpr(CP_NameScope& namescope, InclOrExpr *ioe) {
    int result = 0;
    for (auto& eoe : *ioe) {
        result |= CpCalcConstIntExclOrExpr(namescope, eoe.get());
    }
    return result;
}

int CpCalcConstIntLogAndExpr(CP_NameScope& namescope, LogAndExpr *lae) {
    int result = 1;
    if (lae->size() == 1) {
        result = CpCalcConstIntInclOrExpr(namescope, (*lae)[0].get());
    } else {
        for (auto& ioe : *lae) {
            result = result && CpCalcConstIntInclOrExpr(namescope, ioe.get());
            if (!result) {
                break;
            }
        }
    }
    return result;
}

int CpCalcConstIntLogOrExpr(CP_NameScope& namescope, LogOrExpr *loe) {
    int result = 0;
    if (loe->size() == 1) {
        result = CpCalcConstIntLogAndExpr(namescope, (*loe)[0].get());
    } else {
        for (auto& lae : *loe) {
            result = CpCalcConstIntLogAndExpr(namescope, lae.get());
            if (result) {
                result = 1;
                break;
            }
        }
    }
    return result;
}

int CpCalcConstIntAssignExpr(CP_NameScope& namescope, AssignExpr *ae) {
    int n1, n2;
    switch (ae->m_assign_type) {
    case AssignExpr::COND:
        n1 = CpCalcConstIntCondExpr(namescope, ae->m_cond_expr.get());
        return n1;

    case AssignExpr::SINGLE:
        n1 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        return n1;

    case AssignExpr::MUL:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 *= n2;
        return n1;

    case AssignExpr::DIV:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 /= n2;
        return n1;

    case AssignExpr::MOD:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 %= n2;
        return n1;

    case AssignExpr::ADD:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 += n2;
        return n1;

    case AssignExpr::SUB:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 -= n2;
        return n1;

    case AssignExpr::L_SHIFT:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 <<= n2;
        return n1;

    case AssignExpr::R_SHIFT:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 >>= n2;
        return n1;

    case AssignExpr::AND:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 &= n2;
        return n1;

    case AssignExpr::XOR:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 ^= n2;
        return n1;

    case AssignExpr::OR:
        n1 = CpCalcConstIntUnaryExpr(namescope, ae->m_unary_expr.get());
        n2 = CpCalcConstIntAssignExpr(namescope, ae->m_assign_expr.get());
        n1 |= n2;
        return n1;

    default:
        assert(0);
    }
    return 0;
}

int CpCalcConstIntExpr(CP_NameScope& namescope, Expr *e) {
    int result = 0;
    for (auto& ae : *e) {
        result = CpCalcConstIntAssignExpr(namescope, ae.get());
    }
    return result;
}

int CpCalcConstIntCondExpr(CP_NameScope& namescope, CondExpr *ce) {
    int result = 0;
    switch (ce->m_cond_type) {
    case CondExpr::SINGLE:
        result = CpCalcConstIntLogOrExpr(namescope, ce->m_log_or_expr.get());
        break;

    case CondExpr::QUESTION:
        if (CpCalcConstIntLogOrExpr(namescope, ce->m_log_or_expr.get())) {
            result = CpCalcConstIntExpr(namescope, ce->m_expr.get());
        } else {
            result = CpCalcConstIntCondExpr(namescope, ce->m_cond_expr.get());
        }
        break;

    default:
        assert(0);
        break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////
// CpAnalyse... functions

CP_TypeID CpAnalysePointer(CP_NameScope& namescope, Pointers *pointers,
                           CP_TypeID tid);
void CpAnalyseTypedefDeclorList(CP_NameScope& namescope, CP_TypeID tid,
                                DeclorList *dl, const CP_Location& location);
void CpAnalyseDeclorList(CP_NameScope& namescope, CP_TypeID tid,
                         DeclorList *dl);
void CpAnalyseStructDeclorList(CP_NameScope& namescope, CP_TypeID tid,
                               DeclorList *dl, CP_LogStruct& ls);
void CpAnalyseDeclList(CP_NameScope& namescope, DeclList *dl);
void CpAnalyseParamList(CP_NameScope& namescope, CP_LogFunc& func,
                        ParamList *pl);
void CpAnalyseFunc(CP_NameScope& namescope, CP_TypeID return_type,
                   Declor *declor, DeclList *decl_list);
CP_TypeID CpAnalyseStructDeclList(CP_NameScope& namescope,
                                  const CP_String& name, DeclList *dl,
                                  int pack, const CP_Location& location);
CP_TypeID CpAnalyseUnionDeclList(CP_NameScope& namescope,
                                 const CP_String& name, DeclList *dl,
                                 const CP_Location& location);
CP_TypeID CpAnalyseEnumorList(CP_NameScope& namescope,
                              const CP_String& name, EnumorList *el);
CP_TypeID CpAnalyseAtomic(CP_NameScope& namescope, AtomicTypeSpec *ats);
CP_TypeID CpAnalyseDeclSpecs(CP_NameScope& namescope, DeclSpecs *ds);

////////////////////////////////////////////////////////////////////////////

CP_TypeID CpAnalysePointers(CP_NameScope& namescope, Pointers *pointers,
                            CP_TypeID tid, const CP_Location& location)
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

void CpAnalyseTypedefDeclorList(CP_NameScope& namescope, CP_TypeID tid,
                                DeclorList *dl, const CP_Location& location)
{
    assert(dl);
    for (auto& declor : *dl) {
        CP_TypeID tid2 = tid;

        int value;
        Declor *d = declor.get();
        while (d) {
            CP_String name;
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
                tid2 = CpAnalysePointers(namescope, d->m_pointers.get(), tid2, location);
                d = d->m_declor.get();
                continue;

            case Declor::ARRAY:
                if (d->m_const_expr)
                    value = CpCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                else
                    value = 0;
                tid2 = namescope.AddArrayType(tid2, value, location);
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CP_LogFunc func;
                    func.m_return_type = tid2;
                    if (d->m_param_list) {
                        CpAnalyseParamList(namescope, func, d->m_param_list.get());
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

void CpAnalyseDeclorList(CP_NameScope& namescope, CP_TypeID tid,
                         DeclorList *dl)
{
    assert(dl);
    for (auto& declor : *dl) {
        CP_TypeID tid2 = tid;

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
                tid2 = CpAnalysePointers(namescope, d->m_pointers.get(), tid2, d->location());
                d = d->m_declor.get();
                break;

            case Declor::ARRAY:
                if (d->m_const_expr)
                    value = CpCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                else
                    value = 0;
                tid2 = namescope.AddArrayType(tid2, value, d->location());
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CP_LogFunc lf;
                    lf.m_return_type = tid2;
                    if (d->m_param_list) {
                        CpAnalyseParamList(namescope, lf, d->m_param_list.get());
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

void CpAnalyseStructDeclorList(CP_NameScope& namescope, CP_TypeID tid,
                               DeclorList *dl, CP_LogStruct& ls)
{
    assert(dl);
    for (auto& declor : *dl) {
        CP_TypeID tid2 = tid;

        int value, bits = 0;
        CP_String name;
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
                tid2 = CpAnalysePointers(namescope, d->m_pointers.get(), tid2,
                                         d->location());
                d = d->m_declor.get();
                continue;

            case Declor::ARRAY:
                if (d->m_const_expr)
                    value = CpCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                else
                    value = 0;
                tid2 = namescope.AddArrayType(tid2, value, d->location());
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CP_LogFunc lf;
                    if (d->m_param_list) {
                        CpAnalyseParamList(namescope, lf, d->m_param_list.get());
                    }
                    tid2 = namescope.AddFuncType(lf, d->location());
                    d = d->m_declor.get();
                }
                continue;

            case Declor::BITS:
                assert(ls.m_struct_or_union);   // must be struct
                assert(d->m_const_expr);
                bits = CpCalcConstIntCondExpr(namescope, d->m_const_expr.get());
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
        ls.m_bitfield.push_back(bits);
    }
}

void CpAnalyseDeclList(CP_NameScope& namescope, DeclList *dl) {
    assert(dl);
    for (auto& decl : *dl) {
        CP_TypeID tid = CpAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
        switch (decl->m_decl_type) {
        case Decl::TYPEDEF:
            if (decl->m_declor_list.get()) {
                CpAnalyseTypedefDeclorList(namescope, tid,
                    decl->m_declor_list.get(), decl->location());
            }
            break;

        case Decl::DECLORLIST:
            CpAnalyseDeclorList(namescope, tid, decl->m_declor_list.get());
            break;

        case Decl::STATIC_ASSERT:
            {
                shared_ptr<CondExpr> const_expr =
                    decl->m_static_assert_decl->m_const_expr;
                if (CpCalcConstIntCondExpr(namescope, const_expr.get()) == 0)
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

void CpAnalyseParamList(CP_NameScope& namescope, CP_LogFunc& func,
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
        CP_TypeID tid;
        tid = CpAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());

        #ifdef DEEPDEBUG
            printf("ParamList##%s\n", namescope.StringOfType(tid, "").data());
        #endif

        CP_TypeID tid2 = tid;
        int value;
        CP_String name;
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
                tid2 = CpAnalysePointers(namescope, d->m_pointers.get(), tid2,
                                         d->location());
                d = d->m_declor.get();
                continue;

            case Declor::ARRAY:
                if (d->m_const_expr)
                    value = CpCalcConstIntCondExpr(namescope, d->m_const_expr.get());
                else
                    value = 0;
                tid2 = namescope.AddArrayType(tid2, value, d->location());
                d = d->m_declor.get();
                continue;

            case Declor::FUNCTION:
                {
                    CP_LogFunc lf;
                    lf.m_return_type = tid2;
                    if (d->m_param_list) {
                        CpAnalyseParamList(namescope, lf, d->m_param_list.get());
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

void CpAnalyseFunc(CP_NameScope& namescope, CP_TypeID return_type,
                   Declor *declor, DeclList *decl_list)
{
    CP_LogFunc func;
    assert(declor);

    if (declor->m_declor_type == Declor::FUNCTION) {
        if (!declor->m_name.empty()) {
            if (decl_list) {
                CpAnalyseDeclList(namescope, decl_list);
                if (declor->m_param_list) {
                    CpAnalyseParamList(namescope, func, declor->m_param_list.get());
                    namescope.AddFuncType(func, declor->location());
                } else {
                    assert(0);
                }
            } else {
                assert(declor->m_param_list);
                if (declor->m_param_list) {
                    CpAnalyseParamList(namescope, func, declor->m_param_list.get());
                    namescope.AddFuncType(func, declor->location());
                }
            }
        }
    }
}

CP_TypeID CpAnalyseStructDeclList(CP_NameScope& namescope,
                                  const CP_String& name, DeclList *dl, int pack,
                                  const CP_Location& location)
{
    CP_LogStruct ls(true);  // struct
    ls.m_pack = pack;
    assert(pack >= 1);

    CP_TypeID tid;
    assert(dl);
    for (auto& decl : *dl) {
        switch (decl->m_decl_type) {
        case Decl::DECLORLIST:
            tid = CpAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
            CpAnalyseStructDeclorList(namescope, tid, decl->m_declor_list.get(), ls);
            break;

        case Decl::SINGLE:
            tid = CpAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
            if (tid != cr_invalid_id) {
                ls.m_type_list.push_back(tid);
                ls.m_name_list.push_back("");
                ls.m_bitfield.push_back(0);
            }
            break;

        case Decl::STATIC_ASSERT:
            {
                shared_ptr<CondExpr> const_expr =
                    decl->m_static_assert_decl->m_const_expr;
                if (CpCalcConstIntCondExpr(namescope, const_expr.get()) == 0) {
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

CP_TypeID CpAnalyseUnionDeclList(CP_NameScope& namescope,
                                 const CP_String& name, DeclList *dl,
                                 const CP_Location& location)
{
    CP_LogStruct ls(false);     // union
    ls.m_pack = 1;
    assert(dl);

    CP_TypeID tid;
    assert(dl);
    for (auto& decl : *dl) {
        switch (decl->m_decl_type) {
        case Decl::DECLORLIST:
            tid = CpAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
            CpAnalyseStructDeclorList(namescope, tid, decl->m_declor_list.get(), ls);
            break;

        case Decl::SINGLE:
            tid = CpAnalyseDeclSpecs(namescope, decl->m_decl_specs.get());
            if (tid != cr_invalid_id) {
                ls.m_type_list.push_back(tid);
                ls.m_name_list.push_back("");
                ls.m_bitfield.push_back(0);
            }
            break;

        case Decl::STATIC_ASSERT:
            {
                shared_ptr<CondExpr> const_expr =
                    decl->m_static_assert_decl->m_const_expr;
                if (CpCalcConstIntCondExpr(namescope, const_expr.get()) == 0) {
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

CP_TypeID CpAnalyseEnumorList(CP_NameScope& namescope,
                              const CP_String& name, EnumorList *el,
                              const CP_Location& location)
{
    CP_LogEnum le;

    int value, next_value = 0;
    assert(el);
    CP_TypeID tid_enumitem = namescope.TypeIDFromName("enumitem");
    CP_IDSet vids;
    for (auto& e : *el) {
        if (e->m_const_expr)
            value = CpCalcConstIntCondExpr(namescope, e->m_const_expr.get());
        else
            value = next_value;

        le.MapNameToValue()[e->m_name.data()] = value;
        le.MapValueToName()[value] = e->m_name.data();
        CP_VarID vid = namescope.AddVar(e->m_name, tid_enumitem, location);
        namescope.LogVar(vid).m_has_value = true;
        namescope.LogVar(vid).m_int_value = value;
        vids.push_back(vid);
        next_value = value + 1;
    }

    CP_TypeID tid = namescope.AddEnumType(name, le, location);
    for (const auto& vid : vids) {
        namescope.LogVar(vid).m_enum_type_id = tid;
    }
    return tid;
}

CP_TypeID CpAnalyseAtomic(CP_NameScope& namescope, AtomicTypeSpec *ats) {
    // TODO: TF_ATOMIC
    assert(0);
    return 0;
}

CP_TypeID CpAnalyseDeclSpecs(CP_NameScope& namescope, DeclSpecs *ds) {
    CP_TypeID tid;
    CP_TypeFlags flag, flags = 0;
    if (ds == NULL)
        return namescope.TypeIDFromName("int");

    while (ds) {
        CP_String name;
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
                        tid = CpAnalyseStructDeclList(
                            namescope, name, ts->m_decl_list.get(), ts->m_pack,
                            ts->location());
                    } else {
                        CP_LogStruct ls;
                        ls.m_struct_or_union = true;
                        tid = namescope.AddStructType(name, ls, ts->location());
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
                        tid = CpAnalyseUnionDeclList(
                            namescope, name, ts->m_decl_list.get(),
                            ts->location());
                    } else {
                        CP_LogStruct ls;
                        ls.m_struct_or_union = false;
                        tid = namescope.AddStructType(name, ls, ts->location());
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
                    tid = CpAnalyseEnumorList(
                        namescope, name, ds->m_type_spec->m_enumor_list.get(),
                        ds->m_type_spec->location());
                } else {
                    CP_LogEnum le;
                    tid = namescope.AddEnumType(
                        name, le, ds->m_type_spec->location());
                }
                if (flags & TF_CONST) {
                    tid = namescope.AddConstType(tid);
                }
                assert(tid != cr_invalid_id);
                return tid;

            case TF_ATOMIC:
                return CpAnalyseAtomic(namescope,
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

    flags = CpNormalizeTypeFlags(flags);
    tid = namescope.TypeIDFromFlags(flags & ~TF_CONST);
    assert(tid != cr_invalid_id);
    if (flags & TF_CONST) {
        tid = namescope.AddConstType(tid);
        assert(tid != cr_invalid_id);
    }
    return tid;
}

////////////////////////////////////////////////////////////////////////////

enum CP_ExitCode {
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
void CpReplaceString(
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

std::string CpConvertCmdLineParam(const std::string& str) {
    std::string result = str;
    CpReplaceString(result, "\"", "\"\"");
    if (result.find_first_of("\" \t\f\v") != std::string::npos) {
        result = "\"" + str + "\"";
    }
    return result;
}

// do input
int CpInputCSrc(
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
        atexit(CpDeleteTempFileAtExit);

        // build command line
        #ifdef __GNUC__
            CP_String cmdline("gcc -E");
        #elif defined(__clang__)
            CP_String cmdline("clang -E");
        #elif defined(_MSC_VER)
            CP_String cmdline("cl /nologo /E");
        #else
            #error You lose.
        #endif

        for (; i < argc; i++) {
            cmdline += " ";
            cmdline += CpConvertCmdLineParam(argv[i]);
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
                    fprintf(stderr, "ERROR: Failed to parse file '%s'\n",
                            argv[i]);
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

int CpSemanticAnalysis(CP_NameScope& namescope, shared_ptr<TransUnit>& tu) {
    assert(tu.get());
    for (shared_ptr<Decl>& decl : *tu.get()) {
        switch (decl->m_decl_type) {
        case Decl::FUNCTION: {
                fflush(stderr);
                shared_ptr<DeclSpecs>& ds = decl->m_decl_specs;
                CP_TypeID tid = CpAnalyseDeclSpecs(namescope, ds.get());
                shared_ptr<DeclorList>& dl = decl->m_declor_list;
                assert(dl.get());
                auto& declor = (*dl.get())[0];
                CpAnalyseFunc(namescope, tid, declor.get(),
                              decl->m_decl_list.get());
            }
            break;

        case Decl::TYPEDEF:
        case Decl::DECLORLIST: {
                shared_ptr<DeclSpecs>& ds = decl->m_decl_specs;
                shared_ptr<DeclorList>& dl = decl->m_declor_list;
                CP_TypeID tid = CpAnalyseDeclSpecs(namescope, ds.get());
                if (decl->m_decl_type == Decl::TYPEDEF) {
                    fflush(stderr);
                    if (dl.get()) {
                        CpAnalyseTypedefDeclorList(namescope, tid, dl.get(),
                                                   decl->location());
                    }
                } else {
                    fflush(stderr);
                    CpAnalyseDeclorList(namescope, tid, dl.get());
                }
            }
            break;

        default:
            break;
        }
    }

    return cr_exit_ok;   // success
}

////////////////////////////////////////////////////////////////////////////

void CpDumpSemantic(
    CP_NameScope& namescope,
    const CP_String& strPrefix,
    const CP_String& strSuffix)
{
    FILE *fp;

    fp = fopen((strPrefix + "types" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(type_id)\t(name)\t(flags)\t(sub_id)\t(count)\t(size)\t(file)\t(line)\t(definition)\n");
        for (CP_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
            const auto& name = namescope.MapTypeIDToName()[tid];
            const auto& type = namescope.LogType(tid);
            if (namescope.IsExtendedType(tid)) {
                size_t size = namescope.GetSizeofType(tid);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t(cr_extended)\t0\t(cr_extended)\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_id), 0, static_cast<int>(size));
            } else if (namescope.IsPredefinedType(tid)) {
                size_t size = namescope.GetSizeofType(tid);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t(predefined)\t0\t(predefined)\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_id), 0, static_cast<int>(size));
            } else if (type.m_flags & (TF_STRUCT | TF_UNION | TF_ENUM)) {
                auto strDef = namescope.StringOfType(tid, "", true);
                const auto& location = type.location();
                size_t size = namescope.GetSizeofType(type.m_id);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%s\t%d\t%s;\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_id), 0, static_cast<int>(size),
                    location.m_file.data(), location.m_line, strDef.data());
            } else if (type.m_flags & (TF_POINTER | TF_FUNCTION | TF_ARRAY)) {
                const auto& location = type.location();
                size_t size = namescope.GetSizeofType(type.m_id);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%s\t%d\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_id),
                    static_cast<int>(type.m_count), static_cast<int>(size),
                    location.m_file.data(), location.m_line);
            } else {
                auto strDef = namescope.StringOfType(tid, name, true);
                const auto& location = type.location();
                size_t size = namescope.GetSizeofType(tid);
                fprintf(fp, "%d\t%s\t0x%08lX\t%d\t%d\t%d\t%s\t%d\ttypedef %s;\n",
                    static_cast<int>(tid), name.data(), type.m_flags,
                    static_cast<int>(type.m_id), 0, static_cast<int>(size),
                    location.m_file.data(), location.m_line, strDef.data());
            }
        }
        fclose(fp);
    }

    fp = fopen((strPrefix + "structures" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(type_id)\t(name)\t(struct_id)\t(struct_or_union)\t(size)\t(count)\t(pack)\t(file)\t(line)\t(definition)\t(item_1_type_id)\t(item_1_name)\t(item_1_offset)\t(item_1_bits)\t(item_2_type_id)\t...\n");
        for (CP_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
            const auto& type = namescope.LogType(tid);
            if (!(type.m_flags & (TF_STRUCT | TF_UNION))) {
                continue;
            }
            const auto& name = namescope.MapTypeIDToName()[tid];
            auto strDef = namescope.StringOfType(tid, "");
            assert(!strDef.empty());
            const auto& location = type.location();
            size_t size = namescope.GetSizeofType(tid);
            auto sid = type.m_id;
            const auto& ls = namescope.LogStruct(sid);
            assert(ls.m_type_list.size() == ls.m_name_list.size());
            fprintf(fp, "%d\t%s\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s;",
                static_cast<int>(tid), name.data(), 
                static_cast<int>(sid), static_cast<int>(ls.m_struct_or_union),
                static_cast<int>(size), static_cast<int>(ls.m_type_list.size()),
                static_cast<int>(ls.m_pack),
                location.m_file.data(), location.m_line, strDef.data());
            if (type.m_flags & TF_UNION) {
                for (size_t i = 0; i < ls.m_type_list.size(); ++i) {
                    fprintf(fp, "\t%d\t%s\t0\t0",
                        static_cast<int>(ls.m_type_list[i]), ls.m_name_list[i].data());
                }
            } else {                
                for (size_t i = 0; i < ls.m_type_list.size(); ++i) {
                    fprintf(fp, "\t%d\t%s\t%d\t%d",
                        static_cast<int>(ls.m_type_list[i]), ls.m_name_list[i].data(),
                        static_cast<int>(ls.m_field_offset[i]),
                        static_cast<int>(ls.m_bitfield[i]));
                }
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }

    fp = fopen((strPrefix + "enums" + strSuffix).data(), "w");
    if (fp) {
        fprintf(fp, "(type_id)\t(name)\t(num_items)\t(file)\t(line)\t(item_name_1)\t(item_value_1)\t(item_name_2)\t...\n");
        for (CP_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
            const auto& type = namescope.LogType(tid);
            if (type.m_flags & TF_ENUM) {
                const auto& name = namescope.MapTypeIDToName()[tid];
                const auto& le = namescope.LogEnum(type.m_id);
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
        for (CP_VarID i = 0; i < vars.size(); ++i) {
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
        for (CP_VarID i = 0; i < vars.size(); ++i) {
            const auto& var = vars[i];
            CP_TypeID tid = namescope.ResolveAlias(var.m_type_id);
            if (namescope.IsFuncType(tid)) {
                const auto& name = namescope.MapVarIDToName()[i];
                const auto& type = namescope.LogType(tid);
                const auto& lf = namescope.LogFunc(type.m_id);
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

void CpShowHelp(void) {
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

    #if 0
        printf("Press [Enter] key.");
        getchar();
    #endif

    #if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
        bool is_64bit = true;
    #else
        bool is_64bit = false;
    #endif

    bool show_help = false;
    bool show_version = false;
    bool no_logo = false;
    CP_String strPrefix, strSuffix = ".dat";
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
        CpShowHelp();
        return cr_exit_ok;
    }

    fprintf(stderr, "Parsing...\n");
    shared_ptr<TransUnit> tu;
    if (i < argc) {
        // argv[i] == input-file
        // argv[i + 1] == compiler option #1
        // argv[i + 2] == compiler option #2
        // argv[i + 3] == ...
        int result = CpInputCSrc(tu, i, argc, argv, is_64bit);
        if (result)
            return result;
    } else {
        fprintf(stderr, "ERROR: No input files.\n");
        return cr_exit_no_input;
    }

    if (tu) {
        fprintf(stderr, "Semantic analysis...\n");
        if (is_64bit) {
            CP_NameScope namescope(true);

            int result = CpSemanticAnalysis(namescope, tu);
            if (result)
                return result;

            tu = shared_ptr<TransUnit>();
            CpDumpSemantic(namescope, strPrefix, strSuffix);
        } else {
            CP_NameScope namescope(false);

            int result = CpSemanticAnalysis(namescope, tu);
            if (result)
                return result;

            tu = shared_ptr<TransUnit>();
            CpDumpSemantic(namescope, strPrefix, strSuffix);
        }
    }

    fprintf(stderr, "Done.\n");

    return cr_exit_ok;
}

////////////////////////////////////////////////////////////////////////////
