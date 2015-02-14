#include "stdafx.h"
#include "CParseHeader.h"

////////////////////////////////////////////////////////////////////////////

const char * const cr_logo =
    "///////////////////////////////////////////////\n"
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64)
# ifdef __GNUC__
    "// CParser sample 0.1.9 (64-bit) for gcc     //\n"
# elif defined(__clang__)
    "// CParser sample 0.1.9 (64-bit) for clang    //\n"
# elif defined(_MSC_VER)
    "// CParser sample 0.1.9 (64-bit) for cl      //\n"
# endif
#else   // !64-bit
# ifdef __GNUC__
    "// CParser sample 0.1.9 (32-bit) for gcc     //\n"
# elif defined(__clang__)
    "// CParser sample 0.1.9 (32-bit) for clang    //\n"
# elif defined(_MSC_VER)
    "// CParser sample 0.1.9 (32-bit) for cl      //\n"
# endif
#endif  // !64-bit
    "// public domain software                    //\n"
    "// by Katayama Hirofumi MZ (katahiromz)      //\n"
    "// katayama.hirofumi.mz@gmail.com            //\n"
    "///////////////////////////////////////////////\n";

using namespace std;

////////////////////////////////////////////////////////////////////////////

// temporary file
static char *cr_tmpfile = NULL;

void CrDeleteTempFileAtExit(void)
{
    if (cr_tmpfile) {
        std::remove(cr_tmpfile);
        cr_tmpfile = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////

using namespace cparser;

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

int CrCalcConstIntPrimExpr(CR_NameScope& namescope, PrimExpr *pe)
{
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

int CrCalcConstIntPostfixExpr(CR_NameScope& namescope, PostfixExpr *pe)
{
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

int CrCalcSizeOfUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue)
{
    return 0;
}

CR_TypeID CrAnalyseDeclSpecs(CR_NameScope& namescope, DeclSpecs *ds);

size_t CrCalcSizeOfTypeName(CR_NameScope& namescope, TypeName *tn)
{
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

int CrCalcConstIntUnaryExpr(CR_NameScope& namescope, UnaryExpr *ue)
{
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

int CrCalcConstIntCastExpr(CR_NameScope& namescope, CastExpr *ce)
{
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

int CrCalcConstIntMulExpr(CR_NameScope& namescope, MulExpr *me)
{
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

int CrCalcConstIntAddExpr(CR_NameScope& namescope, AddExpr *ae)
{
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

int CrCalcConstIntShiftExpr(CR_NameScope& namescope, ShiftExpr *se)
{
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

int CrCalcConstIntRelExpr(CR_NameScope& namescope, RelExpr *re)
{
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

int CrCalcConstIntEqualExpr(CR_NameScope& namescope, EqualExpr *ee)
{
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

int CrCalcConstIntLogAndExpr(CR_NameScope& namescope, LogAndExpr *lae)
{
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

int CrCalcConstIntLogOrExpr(CR_NameScope& namescope, LogOrExpr *loe)
{
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

int CrCalcConstIntAssignExpr(CR_NameScope& namescope, AssignExpr *ae)
{
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
        ls.m_bitfield.push_back(bits);
    }
}

void CrAnalyseDeclList(CR_NameScope& namescope, DeclList *dl)
{
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
                ls.m_bitfield.push_back(0);
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
                ls.m_bitfield.push_back(0);
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

CR_TypeID CrAnalyseAtomic(CR_NameScope& namescope, AtomicTypeSpec *ats)
{
    // TODO: TF_ATOMIC
    assert(0);
    return 0;
}

CR_TypeID CrAnalyseDeclSpecs(CR_NameScope& namescope, DeclSpecs *ds)
{
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
                assert(tid != cr_invalid_id);
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
                        CR_LogStruct ls;
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
                        tid = CrAnalyseUnionDeclList(
                            namescope, name, ts->m_decl_list.get(),
                            ts->location());
                    } else {
                        CR_LogStruct ls;
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

// do input
int CrInputCSrc(shared_ptr<TransUnit>& tu, int argc, char **args, bool is_64bit)
{
    char *pchDotExt = strrchr(args[0], '.');
    // if file extension is ".i",
    if (strcmp(pchDotExt, ".i") == 0) {
        // directly parse
        if (!cparser::parse_file(tu, args[0], is_64bit)) {
            fprintf(stderr, "ERROR: Failed to parse file '%s'\n", args[0]);
            return cr_exit_parse_error;   // failure
        }
    } else if (strcmp(pchDotExt, ".h") == 0 ||
               strcmp(pchDotExt, ".c") == 0)
    {
        // if file extension is ".h",
        static char filename[] = "cparser~.tmp";
        cr_tmpfile = filename;
        atexit(CrDeleteTempFileAtExit);

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

        for (int i = 0; i < argc; i++) {
            cmdline += " \"";
            cmdline += args[i];
            cmdline += "\"";
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
                if (!cparser::parse_file(tu, cr_tmpfile, is_64bit))
                {
                    fprintf(stderr, "ERROR: Failed to parse file '%s'\n",
                            args[0]);
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

int CrSemanticAnalysis(CR_NameScope& namescope, shared_ptr<TransUnit>& tu)
{
    assert(tu.get());
    for (shared_ptr<Decl>& decl : *tu.get()) {
        switch (decl->m_decl_type) {
        case Decl::FUNCTION:
            {
                fflush(stderr);
                shared_ptr<DeclSpecs>& ds = decl->m_decl_specs;
                CR_TypeID tid = CrAnalyseDeclSpecs(namescope, ds.get());
                shared_ptr<DeclorList>& dl = decl->m_declor_list;
                assert(dl.get());
                auto& declor = (*dl.get())[0];
                CrAnalyseFunc(namescope, tid, declor.get(),
                              decl->m_decl_list.get());
            }
            break;

        case Decl::TYPEDEF:
        case Decl::DECLORLIST:
            {
                shared_ptr<DeclSpecs>& ds = decl->m_decl_specs;
                shared_ptr<DeclorList>& dl = decl->m_declor_list;
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
        fprintf(fp, "(type_id)\t(name)\t(struct_id)\t(struct_or_union)\t(size)\t(count)\t(pack)\t(file)\t(line)\t(definition)\t(item_1_type_id)\t(item_1_name)\t(item_1_bits)\t(item_2_type_id)\t...\n");
        for (CR_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
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
                    fprintf(fp, "\t%d\t%s\t0",
                        static_cast<int>(ls.m_type_list[i]), ls.m_name_list[i].data());
                }
            } else {                
                for (size_t i = 0; i < ls.m_type_list.size(); ++i) {
                    fprintf(fp, "\t%d\t%s\t%d",
                        static_cast<int>(ls.m_type_list[i]), ls.m_name_list[i].data(),
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
        for (CR_TypeID tid = 0; tid < namescope.LogTypes().size(); ++tid) {
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
            auto& var = vars[i];
            if (namescope.IsFuncType(var.m_type_id)) {
                auto& name = namescope.MapVarIDToName()[i];
                const auto& type = namescope.LogType(var.m_type_id);
                const auto& lf = namescope.LogFunc(type.m_id);
                const auto& location = var.location();
                fprintf(fp, "%d\t%s\t%d\t%s\t%d\t%s;\t%d",
                    static_cast<int>(var.m_type_id), name.data(), 
                    static_cast<int>(lf.m_type_list.size()),
                    location.m_file.data(), location.m_line,
                    namescope.StringOfType(var.m_type_id, name).data(),
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

void CrShowHelp(void)
{
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
int main(int argc, char **argv)
{
    char **args = argv + 1;
    --argc;

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
    CR_String strPrefix, strSuffix = ".dat";
    for (; argc > 0; --argc, ++args) {
        if (::lstrcmpiA(args[0], "/?") == 0 ||
            ::lstrcmpiA(args[0], "--help") == 0)
        {
            show_help = true;
        }
        else if (::lstrcmpiA(args[0], "/nologo") == 0 ||
                 ::lstrcmpiA(args[0], "--nologo") == 0)
        {
            no_logo = true;
        } else if (::lstrcmpiA(args[0], "--version") == 0 ||
                   ::lstrcmpiA(args[0], "/version") == 0)
        {
            show_version = true;
        } else if (::lstrcmpiA(args[0], "-32") == 0) {
            is_64bit = false;
        } else if (::lstrcmpiA(args[0], "-64") == 0) {
            is_64bit = true;
        } else if (::lstrcmpiA(args[0], "--prefix") == 0 ||
                   ::lstrcmpiA(args[0], "-p") == 0)
        {
            --argc, ++args;
            strPrefix = args[0];
        } else if (::lstrcmpiA(args[0], "--suffix") == 0 ||
                   ::lstrcmpiA(args[0], "-s") == 0)
        {
            --argc, ++args;
            strSuffix = args[0];
        } else {
            if (::GetFileAttributesA(args[0]) == 0xFFFFFFFF) {
                if (args[0][0] == '-' || args[0][0] == '/') {
                    fprintf(stderr, "ERROR: Invalid option '%s'.\n", args[0]);
                    return cr_exit_invalid_option;
                } else {
                    fprintf(stderr, "ERROR: File '%s' doesn't exist.\n", args[0]);
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
    if (argc >= 1) {
        // args[0] == input-file
        // ...compiler_options...
        int result = CrInputCSrc(tu, argc, args, is_64bit);
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
