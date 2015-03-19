////////////////////////////////////////////////////////////////////////////
// TypeSystem.cpp
// Copyright (C) 2014-2015 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse. See file ReadMe.txt and License.txt.
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

CR_TypeFlags CrNormalizeTypeFlags(CR_TypeFlags flags) {
    if (flags & TF_INT) {
        // remove "int" if wordy
        if (flags & TF_SHORT)
            flags &= ~TF_INT;
        else if (flags & TF_LONG)
            flags &= ~TF_INT;
        else if (flags & TF_LONGLONG)
            flags &= ~TF_INT;
        else if (flags & TF_INT128)
            flags &= ~TF_INT;
    }
    if ((flags & TF_UNSIGNED) &&
        !(flags & (TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG |
                   TF_INT128 | TF_INT)))
    {
        // add "int" for single "unsigned"
        flags |= TF_INT;
    }
    // add "int" if no type specified
    if (flags == 0)
        flags = TF_INT;
    // remove storage class specifiers
    return flags & ~TF_INCOMPLETE;
} // CrNormalizeTypeFlags

void CrChop(std::string& str) {
    if (str.size() && str[str.size() - 1] == '\n') {
        str.resize(str.size() - 1);
    }
}

std::string CrEscapeString(const std::string& str) {
    std::string result;
    size_t count = 0;
    const size_t siz = str.size();
    for (size_t i = 0; i < siz; ++i) {
        char ch = str[i];
        switch (ch) {
        case '\'': case '\"': case '\?': case '\\':
            result += '\\';
            result += ch;
            count += 2;
            break;
        case '\a':
            result += '\\';
            result += 'a';
            count += 2;
            break;
        case '\b':
            result += '\\';
            result += 'b';
            count += 2;
            break;
        case '\f':
            result += '\\';
            result += 'f';
            count += 2;
            break;
        case '\r':
            result += '\\';
            result += 'r';
            count += 2;
            break;
        case '\t':
            result += '\\';
            result += 't';
            count += 2;
            break;
        case '\v':
            result += '\\';
            result += 'v';
            count += 2;
            break;
        default:
            if (ch < 0x20) {
                int n = static_cast<int>(ch);
                std::strstream ss;
                ss << "\\x" << std::hex <<
                    std::setfill('0') << std::setw(2) << n;
                result += ss.str();
                count += 4;
            } else {
                result += ch;
                count++;
            }
        }
    }
    result.resize(count);
    return "\"" + result + "\"";
}

bool CrUnscapeString(std::string& result, const std::string& str) {
    result.clear();
    size_t siz = str.size();
    bool inside = false, is_valid = true;
    for (size_t i = 0; i < siz; ++i) {
        char ch = str[i];
        if (ch == '\"') {
            if (inside) {
                if (++i < siz && str[i] == '\"') {
                    result += '\"';
                } else {
                    --i;
                    inside = false;
                }
            } else {
                inside = true;
            }
            continue;
        }
        if (!inside) {
            if (!isspace(ch)) {
                is_valid = false;
            }
            continue;
        }
        if (ch != '\\') {
            result += ch;
            continue;
        }
        if (++i >= siz) {
            return false;
        }
        ch = str[i];
        switch (ch) {
        case '\'': case '\"': case '\?': case '\\':
            result += ch;
            break;
        case 'a': result += '\a'; break;
        case 'b': result += '\b'; break;
        case 'f': result += '\f'; break;
        case 'n': result += '\n'; break;
        case 'r': result += '\r'; break;
        case 't': result += '\t'; break;
        case 'v': result += '\v'; break;
        case 'x':
            {
                std::string hex;
                if (++i < siz && isxdigit(str[i])) {
                    hex += str[i];
                    if (++i < siz && isxdigit(str[i])) {
                        hex += str[i];
                    } else {
                        --i;
                    }
                } else {
                    --i;
                    is_valid = false; // invalid escape sequence
                }
                auto n = std::stoul(hex, NULL, 16);
                result += static_cast<char>(n);
            }
            break;
        default:
            if ('0' <= ch && ch <= '7') {
                std::string oct;
                oct += ch;
                if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                    oct += str[i];
                    if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                        oct += str[i];
                    } else {
                        --i;
                    }
                } else {
                    --i;
                }
                auto n = std::stoul(oct, NULL, 8);
                result += static_cast<char>(n);
            }
        }
    }
    return is_valid;
}

////////////////////////////////////////////////////////////////////////////
// CR_TypedValue

CR_TypedValue::CR_TypedValue(const void *ptr, size_t size) :
    m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id), m_addr(0)
{
    assign(ptr, size);
}

CR_TypedValue::CR_TypedValue(const CR_TypedValue& value) :
    m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id), m_addr(0)
{
    m_type_id = value.m_type_id;
    m_text = value.m_text;
    m_extra = value.m_extra;
    m_addr = value.m_addr;
    assign(value.m_ptr, value.m_size);
}

CR_TypedValue& CR_TypedValue::operator=(const CR_TypedValue& value) {
    if (this != &value) {
        m_type_id = value.m_type_id;
        m_text = value.m_text;
        m_extra = value.m_extra;
        m_addr = value.m_addr;
        assign(value.m_ptr, value.m_size);
    }
    return *this;
}

CR_TypedValue::CR_TypedValue(CR_TypedValue&& value) : m_ptr(NULL), m_size(0) {
    if (this != &value) {
        std::swap(m_ptr, value.m_ptr);
        std::swap(m_size, value.m_size);
        m_type_id = value.m_type_id;
        m_addr = value.m_addr;
        std::swap(m_text, value.m_text);
        std::swap(m_extra, value.m_extra);
    }
}

CR_TypedValue& CR_TypedValue::operator=(CR_TypedValue&& value) {
    if (this != &value) {
        std::swap(m_ptr, value.m_ptr);
        std::swap(m_size, value.m_size);
        m_type_id = value.m_type_id;
        m_addr = value.m_addr;
        std::swap(m_text, value.m_text);
        std::swap(m_extra, value.m_extra);
    }
    return *this;
}

void CR_TypedValue::assign(const void *ptr, size_t size) {
    assert(ptr != m_ptr);
    if (ptr == NULL || size == 0) {
        free(m_ptr);
        m_ptr = NULL;
        m_size = size;
        return;
    }
    m_ptr = realloc(m_ptr, size);
    if (m_ptr) {
        memmove(m_ptr, ptr, size);
        m_size = size;
    } else {
        m_size = 0;
        throw std::bad_alloc();
    }
}

/*virtual*/ CR_TypedValue::~CR_TypedValue() {
    free(m_ptr);
    m_ptr = NULL;
    m_size = 0;
}

////////////////////////////////////////////////////////////////////////////
// CR_LogType

bool CR_LogType::operator==(const CR_LogType& type) const {
    return m_flags == type.m_flags &&
           m_sub_id == type.m_sub_id &&
           m_count == type.m_count &&
           m_alignas == type.m_alignas;
}

bool CR_LogType::operator!=(const CR_LogType& type) const {
    return m_flags != type.m_flags ||
           m_sub_id != type.m_sub_id ||
           m_count != type.m_count ||
           m_alignas != type.m_alignas;
}

////////////////////////////////////////////////////////////////////////////
// CR_StructMember

bool operator==(const CR_StructMember& mem1, const CR_StructMember& mem2) {
    return
        mem1.m_type_id      == mem2.m_type_id &&
        mem1.m_name         == mem2.m_name &&
        mem1.m_bit_offset   == mem2.m_bit_offset &&
        mem1.m_bits         == mem2.m_bits;

}

bool operator!=(const CR_StructMember& mem1, const CR_StructMember& mem2) {
    return
        mem1.m_type_id      != mem2.m_type_id ||
        mem1.m_name         != mem2.m_name ||
        mem1.m_bit_offset   != mem2.m_bit_offset ||
        mem1.m_bits         != mem2.m_bits;
}

////////////////////////////////////////////////////////////////////////////
// CR_LogStruct

bool CR_LogStruct::operator==(const CR_LogStruct& ls) const {
    return m_is_struct  == ls.m_is_struct &&
           m_pack       == ls.m_pack &&
           m_align      == ls.m_align &&
           m_members    == ls.m_members;
}

bool CR_LogStruct::operator!=(const CR_LogStruct& ls) const {
    return m_is_struct  != ls.m_is_struct ||
           m_pack       != ls.m_pack ||
           m_align      != ls.m_align ||
           m_members    != ls.m_members;
}

////////////////////////////////////////////////////////////////////////////
// CR_NameScope

CR_NameScope::CR_NameScope(const CR_NameScope& ns) :
    m_error_info(ns.m_error_info),
    m_is_64bit(ns.m_is_64bit),
    m_mNameToTypeID(ns.m_mNameToTypeID),
    m_mTypeIDToName(ns.m_mTypeIDToName),
    m_mNameToVarID(ns.m_mNameToVarID),
    m_mVarIDToName(ns.m_mVarIDToName),
    m_mNameToFuncTypeID(ns.m_mNameToFuncTypeID),
    m_types(ns.m_types),
    m_funcs(ns.m_funcs),
    m_structs(ns.m_structs),
    m_enums(ns.m_enums),
    m_vars(ns.m_vars),
    m_void_type(ns.m_void_type),
    m_char_type(ns.m_char_type),
    m_short_type(ns.m_short_type),
    m_long_type(ns.m_long_type),
    m_long_long_type(ns.m_long_long_type),
    m_int_type(ns.m_int_type),
    m_uchar_type(ns.m_uchar_type),
    m_ushort_type(ns.m_ushort_type),
    m_ulong_type(ns.m_ulong_type),
    m_ulong_long_type(ns.m_ulong_long_type),
    m_uint_type(ns.m_uint_type),
    m_float_type(ns.m_float_type),
    m_double_type(ns.m_double_type),
    m_long_double_type(ns.m_long_double_type) { }

CR_NameScope& CR_NameScope::operator=(const CR_NameScope& ns) {
    m_error_info = ns.m_error_info;
    m_is_64bit = ns.m_is_64bit;
    m_mNameToTypeID = ns.m_mNameToTypeID;
    m_mTypeIDToName = ns.m_mTypeIDToName;
    m_mNameToVarID = ns.m_mNameToVarID;
    m_mVarIDToName = ns.m_mVarIDToName;
    m_mNameToFuncTypeID = ns.m_mNameToFuncTypeID;
    m_types = ns.m_types;
    m_funcs = ns.m_funcs;
    m_structs = ns.m_structs;
    m_enums = ns.m_enums;
    m_vars = ns.m_vars;
    m_void_type = ns.m_void_type;
    m_char_type = ns.m_char_type;
    m_short_type = ns.m_short_type;
    m_long_type = ns.m_long_type;
    m_long_long_type = ns.m_long_long_type;
    m_int_type = ns.m_int_type;
    m_uchar_type = ns.m_uchar_type;
    m_ushort_type = ns.m_ushort_type;
    m_ulong_type = ns.m_ulong_type;
    m_ulong_long_type = ns.m_ulong_long_type;
    m_uint_type = ns.m_uint_type;
    m_float_type = ns.m_float_type;
    m_double_type = ns.m_double_type;
    m_long_double_type = ns.m_long_double_type;
    return *this;
}

void CR_NameScope::Init() {
    CR_Location location("(predefined)", 0);

    m_void_type = AddType("void", TF_VOID, 0, location);

    m_char_type = AddType("char", TF_CHAR, sizeof(char), location);
    m_short_type = AddType("short", TF_SHORT, sizeof(short), location);
    m_long_type = AddType("long", TF_LONG, sizeof(long), location);
    m_long_long_type = AddType("long long", TF_LONGLONG, sizeof(long long), location);
    AddAliasType("__int64", m_long_long_type, location);

    m_int_type = AddType("int", TF_INT, sizeof(int), location);

    m_uchar_type = AddType("unsigned char", TF_UNSIGNED | TF_CHAR, sizeof(char), location);
    m_ushort_type = AddType("unsigned short", TF_UNSIGNED | TF_SHORT, sizeof(short), location);
    m_ulong_type = AddType("unsigned long", TF_UNSIGNED | TF_LONG, sizeof(long), location);
    m_ulong_long_type = AddType("unsigned long long", TF_UNSIGNED | TF_LONGLONG, sizeof(long long), location);
    AddAliasType("unsigned __int64", m_ulong_long_type, location);

    #ifdef __GNUC__
        AddType("__int128", TF_INT128, sizeof(__int128), location);
        AddType("unsigned __int128", TF_UNSIGNED | TF_INT128, sizeof(__int128), location);
    #endif

    m_uint_type = AddType("unsigned int", TF_UNSIGNED | TF_INT, sizeof(int), location);

    m_float_type = AddType("float", TF_FLOAT, sizeof(float), location);
    m_double_type = AddType("double", TF_DOUBLE, sizeof(double), location);

    #ifdef __GNUC__
        if (m_is_64bit)
            m_long_double_type = AddType("long double", TF_LONG | TF_DOUBLE, 16, 16, 16, location);
        else
            m_long_double_type = AddType("long double", TF_LONG | TF_DOUBLE, 12, 4, 4, location);
    #else
        m_long_double_type = AddType("long double", TF_LONG | TF_DOUBLE, sizeof(long double), location);
    #endif
}

CR_TypeID CR_NameScope::AddAliasType(
    const std::string& name, CR_TypeID tid, const CR_Location& location)
{
    assert(!name.empty());
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_ALIAS | TF_INCOMPLETE;
        type1.m_alignas = type2.m_alignas;
    } else {
        type1.m_flags = TF_ALIAS;
        type1.m_size = type2.m_size;
        type1.m_align = type2.m_align;
        type1.m_alignas = type2.m_alignas;
    }
    #ifdef __GNUC__
        if (type2.m_flags & TF_INACCURATE) {
            type1.m_flags |= TF_INACCURATE;
        }
    #endif
    if (type2.m_flags & TF_FUNCTION) {
        type1.m_flags |= TF_FUNCTION;
    }
    type1.m_sub_id = tid;
    type1.m_count = type2.m_count;
    type1.location() = location;
    tid = m_types.insert(type1);
    m_mNameToTypeID[name] = tid;
    m_mTypeIDToName[tid] = name;
    return tid;
}

CR_VarID CR_NameScope::AddVar(
    const std::string& name, CR_TypeID tid, const CR_Location& location)
{
    assert(tid != cr_invalid_id);
    CR_LogVar var;
    var.m_typed_value.m_type_id = tid;
    var.location() = location;
    auto vid = m_vars.insert(var);
    if (!name.empty()) {
        m_mNameToVarID[name] = vid;
        m_mVarIDToName[vid] = name;
    }
    return vid;
}

CR_VarID CR_NameScope::AddVar(
    const std::string& name, CR_TypeID tid, int value,
    const CR_Location& location)
{
    assert(tid != cr_invalid_id);
    CR_LogVar var;
    var.m_typed_value = CR_TypedValue(tid);
    var.m_typed_value.assign<int>(value);
    var.location() = location;
    auto vid = m_vars.insert(var);
    if (!name.empty()) {
        m_mNameToVarID[name] = vid;
        m_mVarIDToName[vid] = name;
    }
    return vid;
}

CR_TypeID CR_NameScope::AddConstType(CR_TypeID tid) {
    assert(tid != cr_invalid_id);
    CR_LogType type1;
    auto type2 = LogType(tid);
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_CONST | TF_INCOMPLETE;
    } else {
        type1.m_flags = TF_CONST;
        type1.m_size = type2.m_size;
        type1.m_align = type2.m_align;
    }
    if (type2.m_flags & TF_BITFIELD) {
        type1.m_flags |= TF_BITFIELD;
    }
    type1.m_sub_id = tid;
    auto tid2 = m_types.AddUnique(type1);
    auto name = NameFromTypeID(tid);
    if (name.size()) {
        name = std::string("const ") + name;
        m_mNameToTypeID[name] = tid2;
        m_mTypeIDToName[tid2] = name;
    }
    return tid2;
}

CR_TypeID CR_NameScope::AddPointerType(
    CR_TypeID tid, CR_TypeFlags flags, const CR_Location& location)
{
    assert(tid != cr_invalid_id);
    CR_LogType type1;
    type1.m_flags = TF_POINTER | flags;
    if (Is64Bit()) {
        type1.m_flags &= ~TF_PTR64;
    } else {
        type1.m_flags &= ~TF_PTR32;
    }
    type1.m_sub_id = tid;
    if (flags & TF_PTR64) {
        type1.m_align = type1.m_size = 8;
    } else if (flags & TF_PTR32) {
        type1.m_align = type1.m_size = 4;
    } else {
        type1.m_align = type1.m_size = (Is64Bit() ? 8 : 4);
    }
    type1.location() = location;
    auto tid2 = m_types.AddUnique(type1);
    auto type2 = LogType(tid);
    auto name = NameFromTypeID(tid);
    if (!name.empty()) {
        if ((type2.m_flags & (TF_ALIAS | TF_FUNCTION)) != TF_FUNCTION) {
            if (flags & TF_CONST) {
                if (flags & TF_PTR32) {
                    name += "* __ptr32 const ";
                } else if (flags & TF_PTR64) {
                    name += "* __ptr64 const ";
                } else {
                    name += "* const ";
                }
            } else {
                if (flags & TF_PTR32) {
                    name += "* __ptr32 ";
                } else if (flags & TF_PTR64) {
                    name += "* __ptr64 ";
                } else {
                    name += "* ";
                }
            }
        }
        m_mNameToTypeID[name] = tid2;
        m_mTypeIDToName[tid2] = name;
    }
    return tid2;
} // AddPointerType

CR_TypeID CR_NameScope::AddArrayType(
    CR_TypeID tid, int count, const CR_Location& location)
{
    assert(tid != cr_invalid_id);
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_ARRAY | TF_INCOMPLETE;
    } else {
        type1.m_flags = TF_ARRAY;
        type1.m_size = type2.m_size * count;
        type1.m_align = type2.m_align;
        if (type1.m_alignas < type2.m_alignas) {
            type1.m_alignas = type2.m_alignas;
            type1.m_align = type2.m_alignas;
        }
    }
    if (type2.m_flags & TF_BITFIELD) {
        type1.m_flags |= TF_BITFIELD;
    }
    type1.m_sub_id = tid;
    type1.m_count = count;
    type1.location() = location;
    tid = m_types.AddUnique(type1);
    m_mTypeIDToName[tid] = "";
    return tid;
} // AddArrayType

CR_TypeID CR_NameScope::AddVectorType(
    const std::string& name, CR_TypeID tid, int vector_size,
    const CR_Location& location)
{
    assert(tid != cr_invalid_id);
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_VECTOR | TF_INCOMPLETE;
    } else {
        type1.m_flags = TF_VECTOR;
        type1.m_count = vector_size / type2.m_size;
    }
    type1.m_size = vector_size;
    type1.m_align = vector_size;
    type1.m_alignas = vector_size;
    if (type2.m_flags & TF_BITFIELD) {
        type1.m_flags |= TF_BITFIELD;
    }
    type1.m_sub_id = tid;
    type1.location() = location;
    tid = m_types.AddUnique(type1);
    m_mNameToTypeID[name] = tid;
    m_mTypeIDToName[tid] = name;
    return tid;
} // AddVectorSize

CR_TypeID CR_NameScope::AddFuncType(
    const CR_LogFunc& lf, const CR_Location& location)
{
    CR_LogFunc func(lf);
    if (func.m_params.size() == 1 && func.m_params[0].m_type_id == 0) {
        // parameter list is void
        func.m_params.clear();
    }
    auto fid = m_funcs.insert(func);
    CR_LogType type1;
    type1.m_flags = TF_FUNCTION;
    type1.m_sub_id = fid;
    type1.m_size = 1;
    type1.m_align = 1;
    type1.location() = location;
    CR_TypeID tid1 = m_types.AddUnique(type1);
    m_mTypeIDToName[tid1] = "";
    return tid1;
} // AddFuncType

CR_TypeID CR_NameScope::AddStructType(
    const std::string& name, const CR_LogStruct& ls,
    int alignas_, const CR_Location& location)
{
    CR_LogType type1;
    if (name.empty()) {     // name is empty
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_STRUCT | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_members.size();
        type1.m_alignas = alignas_;
        type1.location() = location;
        CR_TypeID tid2 = m_types.AddUnique(type1);
        LogStruct(sid).m_tid = tid2;
        m_mTypeIDToName[tid2] = name;
        CompleteStructType(tid2, sid);
        return tid2;
    }
    auto it = m_mNameToTypeID.find("struct " + name);
    if (it == m_mNameToTypeID.end()) {  // name not found
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_STRUCT | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_members.size();
        type1.m_alignas = alignas_;
        type1.location() = location;
        CR_TypeID tid2 = m_types.AddUnique(type1);
        LogStruct(sid).m_tid = tid2;
        m_mNameToTypeID["struct " + name] = tid2;
        m_mTypeIDToName[tid2] = "struct " + name;
        CompleteStructType(tid2, sid);
        return tid2;
    } else {    // name was found
        CR_TypeID tid2 = it->second;
        assert(m_types[tid2].m_flags & TF_STRUCT);
        if (ls.m_members.size()) {
            // overwrite the definition if type list not empty
            auto& type1 = LogType(tid2);
            type1.m_count = ls.m_members.size();
            type1.m_alignas = alignas_;
            type1.location() = location;
            CR_StructID sid = type1.m_sub_id;
            LogStruct(sid) = ls;
            LogStruct(sid).m_tid = tid2;
            CompleteStructType(tid2, sid);
        }
        return tid2;
    }
} // AddStructType

CR_TypeID CR_NameScope::AddUnionType(
    const std::string& name, const CR_LogStruct& ls,
    int alignas_, const CR_Location& location)
{
    CR_LogType type1;
    if (name.empty()) { // name is empty
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_UNION | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_members.size();
        type1.m_alignas = alignas_;
        type1.location() = location;
        CR_TypeID tid1 = m_types.AddUnique(type1);
        LogStruct(sid).m_tid = tid1;
        m_mTypeIDToName[tid1] = name;
        CompleteUnionType(tid1, sid);
        return tid1;
    }
    auto it = m_mNameToTypeID.find("union " + name);
    if (it == m_mNameToTypeID.end()) {  // name not found
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_UNION | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_members.size();
        type1.m_alignas = alignas_;
        type1.location() = location;
        CR_TypeID tid1 = m_types.AddUnique(type1);
        LogStruct(sid).m_tid = tid1;
        m_mNameToTypeID["union " + name] = tid1;
        m_mTypeIDToName[tid1] = "union " + name;
        CompleteUnionType(tid1, sid);
        return tid1;
    } else {    // name was found
        CR_TypeID tid2 = it->second;
        assert(m_types[tid2].m_flags & TF_UNION);
        if (ls.m_members.size()) {
            // overwrite the definition if type list not empty
            auto& type1 = LogType(tid2);
            type1.m_count = ls.m_members.size();
            type1.m_alignas = alignas_;
            type1.location() = location;
            CR_StructID sid = type1.m_sub_id;
            LogStruct(sid) = ls;
            LogStruct(sid).m_tid = tid2;
            CompleteUnionType(tid2, sid);
        }
        return tid2;
    }
} // AddUnionType

CR_TypeID CR_NameScope::AddEnumType(
    const std::string& name, const CR_LogEnum& le, const CR_Location& location)
{
    CR_LogType type1;
    if (name.empty()) {     // name is empty
        CR_EnumID eid = m_enums.insert(le);
        type1.m_flags = TF_ENUM;
        #ifdef __GNUC__
            type1.m_flags |= TF_INACCURATE;
        #endif
        type1.m_sub_id = eid;
        type1.m_size = type1.m_align = 4;
        type1.location() = location;
        CR_TypeID tid1 = m_types.AddUnique(type1);
        m_mTypeIDToName[tid1] = name;
        return tid1;
    }
    auto it = m_mNameToTypeID.find("enum " + name);
    if (it == m_mNameToTypeID.end()) {  // name not found
        CR_EnumID eid = m_enums.insert(le);
        type1.m_flags = TF_ENUM;
        #ifdef __GNUC__
            type1.m_flags |= TF_INACCURATE;
        #endif
        type1.m_sub_id = eid;
        type1.m_size = type1.m_align = 4;
        type1.location() = location;
        CR_TypeID tid1 = m_types.AddUnique(type1);
        m_mNameToTypeID["enum " + name] = tid1;
        m_mTypeIDToName[tid1] = "enum " + name;
        return tid1;
    } else {    // name was found
        CR_TypeID tid1 = it->second;
        auto& type1 = LogType(tid1);
        assert(type1.m_flags & TF_ENUM);
        CR_EnumID eid = type1.m_sub_id;
        if (!le.empty()) {
            // overwrite the definition if not empty
            m_enums[eid] = le;
            type1.m_size = type1.m_align = 4;
        }
        return tid1;
    }
} // AddEnumType

bool CR_NameScope::CompleteStructType(CR_TypeID tid, CR_StructID sid) {
    const int bits_of_one_byte = 8;

    auto& ls = LogStruct(sid);
    auto& type1 = LogType(tid);
    if ((type1.m_flags & TF_INCOMPLETE) == 0) {
        ls.m_is_complete = true;
        return true;
    }

    // check completeness for each field
    const size_t siz = ls.m_members.size();
    bool is_complete = true;
    for (std::size_t i = 0; i < siz; ++i) {
        auto tid2 = ls.m_members[i].m_type_id;
        auto& type2 = LogType(tid2);
        if (type2.m_flags & TF_INCOMPLETE) {
            if (!CompleteType(tid2, type2)) {
                auto& name = ls.m_members[i].m_name;
                m_error_info->add_warning(
                    type2.location(), "'" + name + "' has incomplete type");
                is_complete = false;
            }
        }
        if (ls.m_members[i].m_bits != -1 || (type2.m_flags & TF_BITFIELD)) {
            type1.m_flags |= TF_BITFIELD;
        }
        #ifdef __GNUC__
            if ((type1.m_flags & TF_BITFIELD) ||
                (type2.m_flags & TF_INACCURATE))
            {
                type1.m_flags |= TF_INACCURATE;
            }
        #endif
    }

    // calculate alignment and size
    int byte_offset = 0, prev_item_size = 0;
    int bits_remain = 0, max_align = 1;
    if (is_complete) {
        for (std::size_t i = 0; i < siz; ++i) {
            auto tid2 = ls.m_members[i].m_type_id;
            auto& type2 = LogType(tid2);
            int item_size = type2.m_size;           // size of type
            int item_align = type2.m_align;         // alignment requirement
            if (type1.m_alignas < type2.m_alignas) {
                type1.m_alignas = type2.m_alignas;
            }
            int bits = ls.m_members[i].m_bits;      // bits of bitfield
            if (bits != -1) {
                // the bits specified as bitfield
                assert(bits <= item_size * bits_of_one_byte);
                if (ls.m_pack < item_align) {
                    item_align = ls.m_pack;
                }
                if (prev_item_size == item_size || bits_remain == 0) {
                    // bitfield continuous
                    ls.m_members[i].m_bit_offset =
                        byte_offset * bits_of_one_byte + bits_remain;
                    bits_remain += bits;
                } else {
                    // bitfield discontinuous
                    int bytes =
                        (bits_remain + bits_of_one_byte - 1) / bits_of_one_byte;
                    byte_offset += bytes;
                    if (type2.m_alignas) {
                        int alignas_ = type2.m_alignas;
                        byte_offset = (byte_offset + alignas_ - 1) / alignas_ * alignas_;
                    } else if (ls.m_pack < item_align) {
                        assert(ls.m_pack);
                        byte_offset = (byte_offset + ls.m_pack - 1) / ls.m_pack * ls.m_pack;
                    } else {
                        assert(item_align);
                        byte_offset = (byte_offset + item_align - 1) / item_align * item_align;
                    }
                    ls.m_members[i].m_bit_offset =
                        byte_offset * bits_of_one_byte;
                    bits_remain = bits;
                }
            } else {
                // not bitfield
                if (bits_remain) {
                    // the previous was bitfield
                    int prev_size_bits = prev_item_size * bits_of_one_byte;
                    assert(prev_size_bits);
                    byte_offset += ((bits_remain + prev_size_bits - 1)
                                    / prev_size_bits * prev_size_bits) / bits_of_one_byte;
                    bits_remain = 0;
                }
                if (prev_item_size) {
                    // add padding
                    if (type2.m_alignas) {
                        int alignas_ = type2.m_alignas;
                        byte_offset = (byte_offset + alignas_ - 1) / alignas_ * alignas_;
                    } else if (ls.m_pack < item_align) {
                        assert(ls.m_pack);
                        byte_offset = (byte_offset + ls.m_pack - 1) / ls.m_pack * ls.m_pack;
                    } else {
                        assert(item_align);
                        byte_offset = (byte_offset + item_align - 1) / item_align * item_align;
                    }
                }
                ls.m_members[i].m_bit_offset = byte_offset * bits_of_one_byte;
                byte_offset += item_size;
            }
            if (max_align < item_align) {
                max_align = item_align;
            }
            prev_item_size = item_size;
        }
    }

    // alignment requirement and tail padding
    if (bits_remain) {
        int prev_size_bits = prev_item_size * bits_of_one_byte;
        assert(prev_size_bits);
        byte_offset += ((bits_remain + prev_size_bits - 1)
                        / prev_size_bits * prev_size_bits) / bits_of_one_byte;
    }
    if (type1.m_alignas) {
        int alignas_;
        if (type1.m_alignas >= max_align) {
            alignas_ = type1.m_alignas;
        } else {
            alignas_ = max_align;
        }
        ls.m_align = alignas_;
        type1.m_align = alignas_;
        byte_offset = (byte_offset + alignas_ - 1) / alignas_ * alignas_;
    } else if (ls.m_pack > max_align) {
        ls.m_align = max_align;
        type1.m_align = max_align;
        assert(ls.m_pack);
        byte_offset = (byte_offset + max_align - 1) / max_align * max_align;
    } else {
        ls.m_align = ls.m_pack;
        type1.m_align = ls.m_pack;
        assert(max_align);
        byte_offset = (byte_offset + ls.m_pack - 1) / ls.m_pack * ls.m_pack;
    }

    // total size
    type1.m_size = byte_offset;

    // complete
    if (is_complete && ls.m_members.size()) {
        type1.m_flags &= ~TF_INCOMPLETE;
        ls.m_is_complete = true;
    }
    return is_complete;
} // CompleteStructType

bool CR_NameScope::CompleteUnionType(CR_TypeID tid, CR_StructID sid) {
    auto& ls = LogStruct(sid);
    auto& type1 = LogType(tid);

    if ((type1.m_flags & TF_INCOMPLETE) == 0) {
        ls.m_is_complete = true;
        return true;
    }

    // check completeness for each field
    const size_t siz = ls.m_members.size();
    bool is_complete = true;
    for (std::size_t i = 0; i < siz; ++i) {
        auto tid2 = ls.m_members[i].m_type_id;
        auto& type2 = LogType(tid2);
        if (type2.m_flags & TF_INCOMPLETE) {
            if (!CompleteType(tid2, type2)) {
                auto& name = ls.m_members[i].m_name;
                m_error_info->add_warning(
                    type2.location(), "'" + name + "' has incomplete type");
                is_complete = false;
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type1.m_flags |= TF_INACCURATE;
                }
            #endif
        }
    }

    // calculate alignment and size
    int item_size, item_align, max_size = 0, max_align = 1;
    for (std::size_t i = 0; i < siz; ++i) {
        auto tid2 = ls.m_members[i].m_type_id;
        auto& type2 = LogType(tid2);
        item_size = type2.m_size;
        item_align = type2.m_align;
        if (type1.m_alignas < type2.m_alignas) {
            type1.m_alignas = type2.m_alignas;
        }
        if (max_size < item_size) {
            max_size = item_size;
        }
        if (max_align < item_align) {
            max_align = item_align;
        }
    }

    // alignment requirement
    if (type1.m_alignas) {
        int alignas_;
        if (type1.m_alignas >= max_align) {
            alignas_ = type1.m_alignas;
        } else {
            alignas_ = max_align;
        }
        ls.m_align = alignas_;
        type1.m_align = alignas_;
    } else if (ls.m_pack > max_align) {
        ls.m_align = max_align;
        type1.m_align = max_align;
    } else {
        ls.m_align = ls.m_pack;
        type1.m_align = ls.m_pack;
    }

    type1.m_size = max_size;

    // complete
    if (is_complete && ls.m_members.size()) {
        type1.m_flags &= ~TF_INCOMPLETE;
        ls.m_is_complete = true;
    }
    return is_complete;
} // CompleteUnionType

bool CR_NameScope::CompleteType(CR_TypeID tid, CR_LogType& type) {
    if ((type.m_flags & TF_INCOMPLETE) == 0)
        return true;
    if (type.m_flags & TF_ALIAS) {
        if (CompleteType(type.m_sub_id)) {
            auto& type2 = LogType(type.m_sub_id);
            type.m_size = type2.m_size;
            type.m_align = type2.m_align;
            type.m_alignas = type2.m_alignas;
            if (type2.m_alignas) {
                type.m_align = type2.m_alignas;
                if (type.m_alignas < type2.m_alignas) {
                    type.m_alignas = type2.m_alignas;
                }
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type.m_flags |= TF_INACCURATE;
                }
            #endif
            if (type2.m_flags & TF_FUNCTION) {
                type.m_flags |= TF_FUNCTION;
            }
            type.m_flags &= ~TF_INCOMPLETE;
            return true;
        }
        return false;
    }
    if (type.m_flags & TF_ARRAY) {
        if (CompleteType(type.m_sub_id)) {
            auto& type2 = LogType(type.m_sub_id);
            type.m_size = type2.m_size * static_cast<int>(type.m_count);
            type.m_align = type2.m_align;
            if (type.m_alignas < type2.m_alignas) {
                type.m_alignas = type2.m_alignas;
                type.m_align = type2.m_alignas;
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type.m_flags |= TF_INACCURATE;
                }
            #endif
            type.m_flags &= ~TF_INCOMPLETE;
            return true;
        }
        return false;
    }
    if (type.m_flags & TF_VECTOR) {
        if (CompleteType(type.m_sub_id)) {
            auto& type2 = LogType(type.m_sub_id);
            if (type2.m_size) {
                type.m_count = type.m_size / type2.m_size;
            }
            if (type.m_alignas < type2.m_alignas) {
                type.m_alignas = type2.m_alignas;
                type.m_align = type2.m_alignas;
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type.m_flags |= TF_INACCURATE;
                }
            #endif
            type.m_flags &= ~TF_INCOMPLETE;
            return true;
        }
        return false;
    }
    if (type.m_flags & TF_CONST) {
        if (CompleteType(type.m_sub_id)) {
            auto& type2 = LogType(type.m_sub_id);
            type.m_size = type2.m_size;
            type.m_align = type2.m_align;
            type.m_alignas = type2.m_alignas;
            if (type2.m_alignas) {
                type.m_align = type2.m_alignas;
                if (type.m_alignas < type2.m_alignas) {
                    type.m_alignas = type2.m_alignas;
                }
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type.m_flags |= TF_INACCURATE;
                }
            #endif
            type.m_flags &= ~TF_INCOMPLETE;
            return true;
        }
    }
    if (type.m_flags & (TF_STRUCT | TF_UNION)) {
        auto& ls = LogStruct(type.m_sub_id);
        for (auto mem : ls.m_members) {
            CompleteType(mem.m_type_id);
        }
        if (type.m_flags & TF_STRUCT) {
            return CompleteStructType(tid, type.m_sub_id);
        } else if (type.m_flags & TF_UNION) {
            return CompleteUnionType(tid, type.m_sub_id);
        }
    }
    return false;
} // CompleteType

void CR_NameScope::CompleteTypeInfo() {
    for (CR_TypeID tid = 0; tid < m_types.size(); ++tid) {
        CompleteType(tid);
    }
}

std::string CR_NameScope::StringOfEnum(
    const std::string& name, CR_EnumID eid) const
{
    assert(eid != cr_invalid_id);
    if (eid == cr_invalid_id) {
        return "";  // invalid ID
    }
    auto& e = m_enums[eid];
    std::string str = StringOfEnumTag(name);
    if (!e.empty()) {
        str += "{ ";
        std::vector<std::string> array;
        for (auto it : e.m_mNameToValue) {
            array.emplace_back(it.first + " = " + std::to_string(it.second));
        }
        str += katahiromz::join(array, ", ");
        str += "} ";
    }
    return str;
} // StringOfEnum

std::string
CR_NameScope::StringOfStructTag(
    const std::string& name, const CR_LogStruct& s) const
{
    std::string str;

    if (s.m_is_struct)
        str += "struct ";
    else
        str += "union ";

    auto& type = LogType(s.m_tid);
    if (type.m_alignas && type.m_alignas_explicit) {
        str += "_Alignas(";
        str += std::to_string(type.m_alignas);
        str += ") ";
    }

    if (name.size()) {
        if (s.m_is_struct) {
            assert(name.find("struct ") == 0);
            str += name.substr(7);
        } else {
            assert(name.find("union ") == 0);
            str += name.substr(6);
        }
        str += ' ';
    }
    return str;
} // StringOfStructTag

std::string CR_NameScope::StringOfStruct(
    const std::string& name, CR_StructID sid) const
{
    auto& s = LogStruct(sid);
    std::string str = StringOfStructTag(name, s);
    if (!s.empty()) {
        str += "{ ";
        const std::size_t siz = s.m_members.size();
        for (std::size_t i = 0; i < siz; i++) {
            str += StringOfType(s.m_members[i].m_type_id, s.m_members[i].m_name, false);
            if (s.m_members[i].m_bits != -1) {
                str += " : ";
                str += std::to_string(s.m_members[i].m_bits);
            }
            str += "; ";
        }
        str += "} ";
    }
    return str;
} // StringOfStruct

std::string CR_NameScope::StringOfType(
    CR_TypeID tid, const std::string& name,
    bool expand/* = true*/, bool no_convension/* = false*/) const
{
    auto& type = LogType(tid);
    auto type_name = NameFromTypeID(tid);
    if (type.m_flags & TF_ALIAS) {
        // if type was alias
        if (expand || type_name.empty()) {
            return StringOfType(type.m_sub_id, name, false);
        } else {
            return type_name + " " + name;
        }
    }
    if (type.m_flags & (TF_STRUCT | TF_UNION)) {
        // if type was struct or union
        if (expand || type_name.empty()) {
            return StringOfStruct(type_name, type.m_sub_id) + name;
        } else {
            return type_name + " " + name;
        }
    }
    if (type.m_flags & TF_ENUM) {
        // if type was enum
        if (expand || type_name.empty()) {
            return StringOfEnum(type_name, type.m_sub_id) + name;
        } else {
            return type_name + " " + name;
        }
    }
    if (type.m_flags & (TF_ARRAY | TF_VECTOR)) {
        // if type was array or vector
        if (type.m_count) {
            std::string s = "[";
            s += std::to_string(type.m_count);
            s += ']';
            return StringOfType(type.m_sub_id, name + s, false);
        } else {
            return StringOfType(type.m_sub_id, name + "[]", false);
        }
    }
    if (type.m_flags & TF_FUNCTION) {
        // if type was function
        auto& func = LogFunc(type.m_sub_id);
        auto rettype = StringOfType(func.m_return_type, "", false);
        auto paramlist = StringOfParamList(func.m_params);
        std::string convension;
        if (!no_convension) {
            if (type.m_flags & TF_STDCALL)
                convension = "__stdcall ";
            else if (type.m_flags & TF_FASTCALL)
                convension = "__fastcall ";
            else
                convension = "__cdecl ";
        }
        if (func.m_ellipsis)
            paramlist += ", ...";
        return rettype + convension + name + "(" + paramlist + ")";
    }
    if (type.m_flags & TF_POINTER) {
        // if type was pointer
        auto sub_id = type.m_sub_id;
        auto& type2 = LogType(sub_id);
        if (type2.m_flags & TF_FUNCTION) {
            // function pointer
            if (type.m_flags & TF_CONST) {
                // const function pointer
                if (type.m_flags & TF_STDCALL)
                    return StringOfType(sub_id, "(__stdcall * const " + name + ")", false, true);
                else if (type.m_flags & TF_FASTCALL)
                    return StringOfType(sub_id, "(__fastcall * const " + name + ")", false, true);
                else
                    return StringOfType(sub_id, "(__cdecl * const " + name + ")", false, true);
            } else {
                // non-const function pointer
                if (type.m_flags & TF_STDCALL)
                    return StringOfType(sub_id, "(__stdcall *" + name + ")", false, true);
                else if (type.m_flags & TF_FASTCALL)
                    return StringOfType(sub_id, "(__fastcall *" + name + ")", false, true);
                else
                    return StringOfType(sub_id, "(__cdecl *" + name + ")", false, true);
            }
        } else if (type2.m_flags & TF_POINTER) {
            // pointer to pointer
            if (type.m_flags & TF_CONST) {
                return StringOfType(sub_id, "(* const " + name + ")", false);
            } else {
                return StringOfType(sub_id, "(*" + name + ")", false);
            }
        } else if (type2.m_flags & (TF_ARRAY | TF_VECTOR)) {
            // pointer to array
            if (type.m_flags & TF_CONST) {
                // pointer to const array
                if (type2.m_count) {
                    std::string s = "[";
                    s += std::to_string(type2.m_count);
                    s += ']';
                    return StringOfType(sub_id, "(* const " + name + s + ")", false);
                } else {
                    return StringOfType(sub_id, "(* const " + name + "[])", false);
                }
            } else {
                // pointer to non-const array
                if (type2.m_count) {
                    std::string s = "[";
                    s += std::to_string(type2.m_count);
                    s += ']';
                    return StringOfType(sub_id, "(*" + name + s + ")", false);
                } else {
                    return StringOfType(sub_id, "(*" + name + "[])", false);
                }
            }
        } else {
            // otherwise
            if (type.m_flags & TF_CONST) {
                return StringOfType(sub_id, "", false) + "* const " + name;
            } else {
                return StringOfType(sub_id, "", false) + "*" + name;
            }
        }
    }
    if (type.m_flags & TF_CONST) {
        // if type is const
        return "const " + StringOfType(type.m_sub_id, name, false);
    }
    if (type_name.size()) {
        // if there was type name
        return type_name + " " + name;
    }
    return "";  // no name
} // StringOfType

std::string CR_NameScope::StringOfParamList(
    const std::vector<CR_FuncParam>& params) const
{
    std::size_t i, size = params.size();
    std::string str;
    if (size > 0) {
        str += StringOfType(params[0].m_type_id, params[0].m_name, false);
        for (i = 1; i < size; i++) {
            str += ", ";
            str += StringOfType(params[i].m_type_id, params[i].m_name, false);
        }
    } else {
        str += "void";
    }
    return str;
} // StringOfParamList

bool CR_NameScope::IsFuncType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    if (LogType(tid).m_flags & TF_FUNCTION)
        return true;
    return false;
} // IsFuncType

bool CR_NameScope::IsPredefinedType(CR_TypeID tid) const {
    auto& type = LogType(tid);
    if (type.m_flags & (TF_POINTER | TF_ARRAY | TF_CONST)) {
        return IsPredefinedType(type.m_sub_id);
    }
    return (type.location().m_file == "(predefined)");
} // IsPredefinedType

bool CR_NameScope::IsIntegralType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    auto& type = LogType(tid);
    const CR_TypeFlags not_flags =
        (TF_DOUBLE | TF_FLOAT | TF_POINTER | TF_ARRAY | TF_VECTOR |
         TF_FUNCTION | TF_STRUCT | TF_UNION | TF_ENUM);
    if (type.m_flags & not_flags)
        return false;
    const CR_TypeFlags flags =
        (TF_INT | TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG);
    if (type.m_flags & flags)
        return true;
    if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST)
        return IsIntegralType(type.m_sub_id);
    return false;
} // IsIntegralType

bool CR_NameScope::IsFloatingType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    auto& type = LogType(tid);
    if (type.m_flags & (TF_DOUBLE | TF_FLOAT))
        return true;
    if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST)
        return IsFloatingType(type.m_sub_id);
    return false;
} // IsFloatingType

bool CR_NameScope::IsUnsignedType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    auto& type = LogType(tid);
    if (type.m_flags & TF_UNSIGNED)
        return true;
    if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST)
        return IsUnsignedType(type.m_sub_id);
    return false;
} // IsUnsignedType

bool CR_NameScope::IsPointerType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    auto& type = LogType(tid);
    if (type.m_flags & TF_POINTER)
        return true;
    if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST)
        return IsUnsignedType(type.m_sub_id);
    return false;
}

bool CR_NameScope::IsFunctionType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    auto& type = LogType(tid);
    if (type.m_flags & TF_POINTER)
        return true;
    if ((type.m_flags & (TF_ALIAS | TF_FUNCTION)) == TF_FUNCTION)
        return IsFunctionType(type.m_sub_id);
    return false;
}

CR_TypeID CR_NameScope::ResolveAlias(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        auto& type = LogType(tid);
        if (type.m_flags & TF_ALIAS) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return tid;
}

CR_TypeID CR_NameScope::ResolveAliasAndCV(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        auto& type = LogType(tid);
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            if (!(type.m_flags & TF_ALIAS)) {
                break;
            }
        }
    }
    return tid;
}

CR_TypeID CR_NameScope::TypeIDFromFlags(CR_TypeFlags flags) const {
    const size_t siz = m_types.size();
    for (size_t i = 0; i < siz; ++i) {
        if (LogType(i).m_flags == flags)
            return i;
    }
    return cr_invalid_id;
}

CR_TypeID CR_NameScope::TypeIDFromName(const std::string& name) const {
    auto it = m_mNameToTypeID.find(name);
    if (it != m_mNameToTypeID.end())
        return it->second;
    else
        return cr_invalid_id;
}

std::string CR_NameScope::NameFromTypeID(CR_TypeID tid) const {
    auto it = m_mTypeIDToName.find(tid);
    if (it != m_mTypeIDToName.end())
        return it->second;
    else
        return "";
}

void
CR_NameScope::GetStructMemberList(
    CR_StructID sid, std::vector<CR_StructMember>& members) const
{
    members.clear();
    auto& ls = LogStruct(sid);
    for (auto& mem : ls.m_members) {
        if (mem.m_name.size()) {
            members.emplace_back(mem);
        } else {
            CR_TypeID tid = ResolveAlias(mem.m_type_id);
            auto& type = LogType(tid);
            if (type.m_flags & (TF_STRUCT | TF_UNION)) {
                std::vector<CR_StructMember> children;
                GetStructMemberList(type.m_sub_id, children);
                for (auto& child : children) {
                    child.m_bit_offset += mem.m_bit_offset;
                }
                members.insert(members.end(),
                    children.begin(), children.end());
            }
        }
    }
}

CR_TypeID CR_NameScope::AddConstCharType() {
    return AddConstType(m_char_type);
}

CR_TypeID CR_NameScope::AddConstUCharType() {
    return AddConstType(m_uchar_type);
}

CR_TypeID CR_NameScope::AddConstShortType() {
    return AddConstType(m_short_type);
}

CR_TypeID CR_NameScope::AddConstUShortType() {
    return AddConstType(m_ushort_type);
}

CR_TypeID CR_NameScope::AddConstIntType() {
    return AddConstType(m_int_type);
}

CR_TypeID CR_NameScope::AddConstUIntType() {
    return AddConstType(m_uint_type);
}

CR_TypeID CR_NameScope::AddConstLongType() {
    return AddConstType(m_long_type);
}

CR_TypeID CR_NameScope::AddConstULongType() {
    return AddConstType(m_ulong_type);
}

CR_TypeID CR_NameScope::AddConstLongLongType() {
    return AddConstType(m_long_long_type);
}

CR_TypeID CR_NameScope::AddConstULongLongType() {
    return AddConstType(m_ulong_long_type);
}

CR_TypeID CR_NameScope::AddConstFloatType() {
    return AddConstType(m_float_type);
}

CR_TypeID CR_NameScope::AddConstDoubleType() {
    return AddConstType(m_double_type);
}

CR_TypeID CR_NameScope::AddConstLongDoubleType() {
    return AddConstType(m_long_double_type);
}

CR_TypeID CR_NameScope::AddConstStringType() {
    auto tid = m_char_type;
    auto& type = LogType(tid);
    tid = AddConstType(tid);
    tid = AddPointerType(tid, TF_CONST, type.location());
    return tid;
}

CR_TypeID CR_NameScope::AddConstWStringType() {
    CR_TypeID tid;
    auto it = MapNameToTypeID().find("wchar_t");
    if (it != MapNameToTypeID().end()) {
        tid = it->second;
    } else {
        tid = AddAliasType("wchar_t", m_ushort_type,
                           CR_Location("(predefined)", 0));
    }
    auto& type = LogType(tid);
    tid = AddConstType(tid);
    tid = AddPointerType(tid, TF_CONST, type.location());
    return tid;
}

CR_TypeID CR_NameScope::IsStringType(CR_TypeID tid) const {
    tid = ResolveAliasAndCV(tid);
    auto& type1 = LogType(tid);
    if (type1.m_flags & (TF_POINTER | TF_ARRAY)) {
        auto tid2 = ResolveAliasAndCV(type1.m_sub_id);
        auto& type2 = LogType(tid2);
        if ((type2.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            auto tid3 = ResolveAlias(type2.m_sub_id);
            auto& type3 = LogType(tid3);
            return (type3.m_flags & TF_CHAR) != 0;
        } else {
            return (type2.m_flags & TF_CHAR) != 0;
        }
    }
    return false;
}

CR_TypeID CR_NameScope::IsWStringType(CR_TypeID tid) const {
    tid = ResolveAliasAndCV(tid);
    auto& type1 = LogType(tid);
    if (type1.m_flags & (TF_POINTER | TF_ARRAY)) {
        auto tid2 = ResolveAliasAndCV(type1.m_sub_id);
        auto& type2 = LogType(tid2);
        if ((type2.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            auto tid3 = ResolveAlias(type2.m_sub_id);
            auto& type3 = LogType(tid3);
            return (type3.m_flags & TF_SHORT) != 0;
        } else if (type2.m_flags & TF_SHORT) {
            return true;
        }
    }
    return false;
}

long long CR_NameScope::GetLongLongValue(const CR_TypedValue& value) const {
    long long result = 0;
    auto& type = LogType(value.m_type_id);
    if (type.m_size != value.m_size) {
        return 0;
    }
    if (type.m_size == sizeof(int)) {
        result = static_cast<long long>(value.get<int>());
    } else if (type.m_size == sizeof(char)) {
        result = static_cast<long long>(value.get<char>());
    } else if (type.m_size == sizeof(short)) {
        result = static_cast<long long>(value.get<short>());
    } else if (type.m_size == sizeof(long)) {
        result = static_cast<long long>(value.get<long>());
    } else if (type.m_size == sizeof(long long)) {
        result = value.get<long long>();
    }
    return result;
}

unsigned long long CR_NameScope::GetULongLongValue(const CR_TypedValue& value) const {
    unsigned long long result = 0;
    auto& type = LogType(value.m_type_id);
    if (type.m_size != value.m_size) {
        return 0;
    }
    if (type.m_size == sizeof(unsigned int)) {
        result = static_cast<unsigned long long>(value.get<unsigned int>());
    } else if (type.m_size == sizeof(unsigned char)) {
        result = static_cast<unsigned long long>(value.get<unsigned char>());
    } else if (type.m_size == sizeof(unsigned short)) {
        result = static_cast<unsigned long long>(value.get<unsigned short>());
    } else if (type.m_size == sizeof(unsigned long)) {
        result = static_cast<unsigned long long>(value.get<unsigned long>());
    } else if (type.m_size == sizeof(unsigned long long)) {
        result = value.get<unsigned long long>();
    }
    return result;
}

long double CR_NameScope::GetLongDoubleValue(const CR_TypedValue& value) const {
    long double result = 0;
    auto& type = LogType(value.m_type_id);
    if (type.m_size != value.m_size) {
        return 0;
    }
    if (type.m_size == sizeof(float)) {
        result = value.get<float>();
    } else if (type.m_size == sizeof(double)) {
        result = value.get<double>();
    } else if (type.m_size == sizeof(long double)) {
        result = value.get<long double>();
    } else {
        assert(0);
    }
    return result;
}

CR_TypedValue CR_NameScope::StaticCast(
    CR_TypeID tid, const CR_TypedValue& value) const
{
    if (tid == value.m_type_id) {
        return value;
    }

    CR_TypedValue result;
    result.m_type_id = tid;
    result.m_size = SizeOfType(tid);
    auto& type = LogType(tid);
    if (HasValue(value)) {
        auto tid2 = value.m_type_id;
        auto& type2 = LogType(tid2);
        if (IsIntegralType(tid2)) {
            if (IsUnsignedType(tid2)) {
                auto u2 = GetULongLongValue(value);
                SetULongLongValue(result, u2);
            } else {
                auto n2 = GetLongLongValue(value);
                SetLongLongValue(result, n2);
            }
        } else if (IsFloatingType(tid2)) {
            auto ld2 = GetLongDoubleValue(value);
            SetLongDoubleValue(result, ld2);
        } else if (IsPointerType(tid2)) {
            auto u2 = GetULongLongValue(value);
            SetULongLongValue(result, u2);
        }
    }

    return result;
}

CR_TypedValue CR_NameScope::ReinterpretCast(
    CR_TypeID tid, const CR_TypedValue& value) const
{
    if (value.m_type_id == tid) {
        return value;
    }
    CR_TypedValue result(value);
    result.m_type_id = tid;
    return result;
}

CR_TypedValue CR_NameScope::Cast(
    CR_TypeID tid, const CR_TypedValue& value) const
{
    CR_TypedValue result;
    auto& type1 = LogType(tid);
    auto& type2 = LogType(value.m_type_id);
    if (IsPointerType(tid)) {
        int pointer_size = type1.m_size;
        if (IsPointerType(value.m_type_id)) {
            result = ReinterpretCast(tid, value);
        } else {
            if (pointer_size == 8) {
                result = StaticCast(m_ulong_long_type, value);
                result.m_type_id = tid;
            } else if (pointer_size == 4) {
                result = StaticCast(m_uint_type, value);
                result.m_type_id = tid;
            }
        }
    } else {
        if (type1.m_size == type2.m_size) {
            result = value;
            result.m_type_id = tid;
        } else if (IsPointerType(value.m_type_id)) {
            if (type1.m_size == sizeof(int)) {
                result = StaticCast(m_int_type, value);
            } else if (type1.m_size == sizeof(char)) {
                result = StaticCast(m_char_type, value);
            } else if (type1.m_size == sizeof(short)) {
                result = StaticCast(m_short_type, value);
            } else if (type1.m_size == sizeof(long)) {
                result = StaticCast(m_long_type, value);
            } else if (type1.m_size == sizeof(long long)) {
                result = StaticCast(m_long_long_type, value);
            }
            result.m_type_id = tid;
        }
    }
    return result;
}

void CR_NameScope::SetAlignas(CR_TypeID tid, int alignas_) {
    auto& type = LogType(tid);
    type.m_align = alignas_;
    type.m_alignas = alignas_;
    type.m_alignas_explicit = true;
    if (type.m_flags & (TF_STRUCT | TF_UNION)) {
        LogStruct(type.m_sub_id).m_align = alignas_;
        LogStruct(type.m_sub_id).m_alignas = alignas_;
        LogStruct(type.m_sub_id).m_alignas_explicit = true;
    }
}

CR_TypedValue
CR_NameScope::ArrayItem(const CR_TypedValue& array, size_t index) const {
    CR_TypedValue result;
    auto& array_type = LogType(array.m_type_id);
    auto item_tid = array_type.m_sub_id;
    auto& item_type = LogType(item_tid);
    if (0 <= index && index < array_type.m_count) {
    } else {
        return result;
    }

    result.m_addr = array.m_addr + index * item_type.m_size;
    const char *ptr = array.get_at<char>(
        index * item_type.m_size, item_type.m_size);
    if (ptr) {
        SetValue(result, item_tid, ptr, item_type.m_size);
    }
    return result;
}

CR_TypedValue CR_NameScope::Dot(
    const CR_TypedValue& struct_value, const std::string& name) const
{
    const int bits_of_one_byte = 8;
    CR_TypedValue result;
    auto& struct_type = LogType(struct_value.m_type_id);
    std::vector<CR_StructMember> children;
    GetStructMemberList(struct_type.m_sub_id, children);
    for (auto& child : children) {
        if (child.m_name == name) {
            if (child.m_bit_offset % bits_of_one_byte) {
                // bitfield not supported yet
                break;
            }
            result.m_type_id = child.m_type_id;
            result.m_addr = struct_value.m_addr +
                            child.m_bit_offset / bits_of_one_byte;
            result.m_size = SizeOfType(child.m_type_id);
            const char *ptr =
                struct_value.get_at<char>(
                    child.m_bit_offset / bits_of_one_byte, result.m_size);
            if (ptr) {
                SetValue(result, child.m_type_id, ptr, result.m_size);
            }
            break;
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Asterisk(const CR_TypedValue& pointer_value) const {
    CR_TypedValue result;
    auto tid = ResolveAlias(pointer_value.m_type_id);
    if (IsPointerType(tid)) {
        auto& type = LogType(tid);
        auto tid2 = type.m_sub_id;
        auto& type2 = LogType(tid2);
        if (HasValue(pointer_value)) {
            auto addr = GetULongLongValue(pointer_value);
            const void *ptr = GetAddressPointer(addr, type2.m_size);
            SetValue(result, tid2, ptr, type2.m_size);
        } else {
            result.m_type_id = tid2;
            result.m_size = type2.m_size;
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Address(const CR_TypedValue& value) const {
    CR_TypedValue result;
    if (Is64Bit()) {
        result.m_type_id = m_ulong_long_type;
        result.assign<unsigned long long>(value.m_addr);
    } else {
        result.m_type_id = m_uint_type;
        result.assign<unsigned int>(
            static_cast<unsigned int>(value.m_addr));
    }
    return result;
}

CR_TypedValue CR_NameScope::Arrow(
    const CR_TypedValue& pointer_value, const std::string& name) const
{
    CR_TypedValue result = Asterisk(pointer_value);
    result = Dot(result, name);
    return result;
}

void CR_NameScope::SetLongLongValue(CR_TypedValue& value, long long n) const {
    if (value.m_type_id == cr_invalid_id || value.m_size == 0) {
        return;
    }
    if (IsIntegralType(value.m_type_id) || IsPointerType(value.m_type_id)) {
        if (value.m_size == sizeof(int)) {
            value.assign<int>(static_cast<int>(n));
        } else if (value.m_size == sizeof(char)) {
            value.assign<char>(static_cast<char>(n));
        } else if (value.m_size == sizeof(short)) {
            value.assign<short>(static_cast<short>(n));
        } else if (value.m_size == sizeof(long)) {
            value.assign<long>(static_cast<long>(n));
        } else if (value.m_size == sizeof(long long)) {
            value.assign<long long>(n);
        }
    } else if (IsFloatingType(value.m_type_id)) {
        if (value.m_size == sizeof(float)) {
            value.assign<float>(static_cast<float>(n));
        } else if (value.m_size == sizeof(double)) {
            value.assign<double>(static_cast<double>(n));
        } else if (value.m_size == sizeof(long double)) {
            value.assign<long double>(static_cast<long double>(n));
        }
    }
}

void CR_NameScope::SetULongLongValue(CR_TypedValue& value, unsigned long long u) const {
	if (value.m_type_id == cr_invalid_id || value.m_size == 0) {
		return;
	}
	if (IsIntegralType(value.m_type_id) || IsPointerType(value.m_type_id)) {
        if (value.m_size == sizeof(int)) {
            value.assign<unsigned int>(static_cast<unsigned int>(u));
        } else if (value.m_size == sizeof(char)) {
            value.assign<unsigned char>(static_cast<unsigned char>(u));
        } else if (value.m_size == sizeof(short)) {
            value.assign<unsigned short>(static_cast<unsigned short>(u));
        } else if (value.m_size == sizeof(long)) {
            value.assign<unsigned long>(static_cast<unsigned long>(u));
        } else if (value.m_size == sizeof(long long)) {
            value.assign<unsigned long long>(u);
        }
    } else if (IsFloatingType(value.m_type_id)) {
        if (value.m_size == sizeof(float)) {
            value.assign<float>(static_cast<float>(u));
        } else if (value.m_size == sizeof(double)) {
            value.assign<double>(static_cast<double>(u));
        } else if (value.m_size == sizeof(long double)) {
            value.assign<long double>(static_cast<long double>(u));
        }
    }
}

void CR_NameScope::SetLongDoubleValue(CR_TypedValue& value, long double ld) const {
	if (value.m_type_id == cr_invalid_id || value.m_size == 0) {
		return;
	}
	if (IsIntegralType(value.m_type_id)) {
        if (value.m_size == sizeof(int)) {
            value.assign<int>(static_cast<char>(ld));
        } else if (value.m_size == sizeof(char)) {
            value.assign<char>(static_cast<char>(ld));
        } else if (value.m_size == sizeof(short)) {
            value.assign<short>(static_cast<short>(ld));
        } else if (value.m_size == sizeof(long)) {
            value.assign<long>(static_cast<long>(ld));
        } else if (value.m_size == sizeof(long long)) {
            value.assign<long long>(static_cast<long long>(ld));
        }
    } else if (IsFloatingType(value.m_type_id)) {
        if (value.m_size == sizeof(float)) {
            value.assign<float>(static_cast<float>(ld));
        } else if (value.m_size == sizeof(double)) {
            value.assign<double>(static_cast<double>(ld));
        } else if (value.m_size == sizeof(long double)) {
            value.assign<long double>(ld);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// calculations

void CR_NameScope::IntZero(CR_TypedValue& value1) const {
    value1.m_type_id = m_int_type;
    int n = 0;
    value1.assign<int>(n);
}

void CR_NameScope::IntOne(CR_TypedValue& value1) const {
    value1.m_type_id = m_int_type;
    int n = 1;
    value1.assign<int>(n);
}

bool CR_NameScope::IsZero(const CR_TypedValue& value1) const {
    if (!HasValue(value1)) {
        return false;
    }
    const char *p = reinterpret_cast<const char *>(value1.m_ptr);
    for (size_t i = 0; i < value1.m_size; ++i) {
        if (*p) {
            return false;
        }
    }
    return true;
}

bool CR_NameScope::IsNonZero(const CR_TypedValue& value1) const {
    return !IsZero(value1);
}

CR_TypedValue CR_NameScope::BiOp(CR_TypedValue& v1, CR_TypedValue& v2) const {
    CR_TypedValue result;
    if (IsIntegralType(v1.m_type_id)) {
        if (IsIntegralType(v2.m_type_id)) {
            int size1 = SizeOfType(v1.m_type_id);
            if (size1 < 4) {
                v1 = StaticCast(m_int_type, v1);
                size1 = 4;
            }
            int size2 = SizeOfType(v2.m_type_id);
            if (size2 < 4) {
                v2 = StaticCast(m_int_type, v2);
                size2 = 4;
            }
            if (size1 >= size2) {
                result.m_type_id = v1.m_type_id;
                result.m_size = size1;
            } else {
                result.m_type_id = v2.m_type_id;
                result.m_size = size2;
            }
        } else if (IsFloatingType(v2.m_type_id)) {
            v1 = StaticCast(v2.m_type_id, v1);
            result.m_type_id = v2.m_type_id;
            result.m_size = SizeOfType(v2.m_type_id);
        }
    } else if (IsFloatingType(v1.m_type_id)) {
        if (IsIntegralType(v2.m_type_id)) {
            v2 = StaticCast(v1.m_type_id, v2);
            result.m_type_id = v1.m_type_id;
            result.m_size = SizeOfType(v1.m_type_id);
        } else if (IsFloatingType(v2.m_type_id)) {
            int size1 = SizeOfType(v1.m_type_id);
            int size2 = SizeOfType(v2.m_type_id);
            if (size1 >= size2) {
                result.m_type_id = v1.m_type_id;
                result.m_size = size1;
            } else {
                result.m_type_id = v2.m_type_id;
                result.m_size = size2;
            }
        }
    }
    return result;
}

int CR_NameScope::CompareValue(
    const CR_TypedValue& v1, const CR_TypedValue& v2) const
{
    if (!HasValue(v1) || !HasValue(v2)) {
        return -2;
    }
    bool is_floating = false;
    if (IsIntegralType(v1.m_type_id)) {
        if (IsIntegralType(v2.m_type_id)) {
            bool un1 = IsUnsignedType(v1.m_type_id);
            bool un2 = IsUnsignedType(v2.m_type_id);
            if (un1) {
                if (un2) {
                    auto u1 = GetULongLongValue(v1);
                    auto u2 = GetULongLongValue(v2);
                    if (u1 < u2) return -1;
                    if (u1 > u2) return 1;
                } else {
                    auto u1 = GetULongLongValue(v1);
                    auto n2 = GetLongLongValue(v2);
                    auto n1 = static_cast<long long>(u1);
                    if (n1 < n2) return -1;
                    if (n1 > n2) return 1;
                }
            } else {
                if (un2) {
                    auto n1 = GetLongLongValue(v1);
                    auto u2 = GetULongLongValue(v2);
                    auto n2 = static_cast<long long>(u2);
                    if (n1 < n2) return -1;
                    if (n1 > n2) return 1;
                } else {
                    auto n1 = GetLongLongValue(v1);
                    auto n2 = GetLongLongValue(v2);
                    if (n1 < n2) return -1;
                    if (n1 > n2) return 1;
                }
            }
        } else if (IsFloatingType(v2.m_type_id)) {
            is_floating = true;
        }
    } else if (IsFloatingType(v1.m_type_id)) {
        is_floating = true;
    } else if (IsPointerType(v1.m_type_id)) {
        if (IsPointerType(v2.m_type_id)) {
            auto u1 = GetULongLongValue(v1);
            auto u2 = GetULongLongValue(v2);
            if (u1 < u2) return -1;
            if (u1 > u2) return 1;
        }
    }
    if (is_floating) {
        auto ld1 = GetLongDoubleValue(v1);
        auto ld2 = GetLongDoubleValue(v2);
        if (ld1 < ld2) return -1;
        if (ld1 > ld2) return 1;
    }
    return 0;
}

CR_TypedValue
CR_NameScope::BiOpInt(CR_TypedValue& v1, CR_TypedValue& v2) const {
    CR_TypedValue result;
    if (IsIntegralType(v1.m_type_id) && IsIntegralType(v2.m_type_id)) {
        int size1 = SizeOfType(v1.m_type_id);
        if (size1 < 4) {
            v1 = StaticCast(m_int_type, v1);
            size1 = 4;
        }
        int size2 = SizeOfType(v2.m_type_id);
        if (size2 < 4) {
            v2 = StaticCast(m_int_type, v2);
            size2 = 4;
        }
        if (size1 >= size2) {
            result.m_type_id = v1.m_type_id;
            result.m_size = size1;
        } else {
            result.m_type_id = v2.m_type_id;
            result.m_size = size2;
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Add(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOp(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsFloatingType(result.m_type_id)) {
                auto ld1 = GetLongDoubleValue(v1);
                auto ld2 = GetLongDoubleValue(v2);
                SetLongDoubleValue(result, ld1 + ld2);
            } else if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 + n2);
                } else {
                    SetLongLongValue(result, n1 + n2);
                }
            }
        }
    } else {
        if (IsIntegralType(v1.m_type_id) && IsPointerType(v2.m_type_id)) {
            std::swap(v1, v2);
        }
        if (IsPointerType(v1.m_type_id) && IsIntegralType(v2.m_type_id)) {
            v1.m_type_id = ResolveAlias(v1.m_type_id);
            auto& type1 = LogType(v1.m_type_id);
            auto& type2 = LogType(type1.m_sub_id);
            result.m_type_id = v1.m_type_id;
            result.m_size = v1.m_size;
            if (HasValue(v1) && HasValue(v2)) {
                if (IsUnsignedType(v2.m_type_id)) {
                    auto u1 = GetULongLongValue(v1);
                    auto u2 = GetULongLongValue(v2);
                    SetULongLongValue(result, u1 + u2 * type2.m_size);
                } else {
                    auto u1 = GetULongLongValue(v1);
                    auto n2 = GetLongLongValue(v2);
                    SetULongLongValue(result, u1 + n2 * type2.m_size);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Sub(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOp(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsFloatingType(result.m_type_id)) {
                auto ld1 = GetLongDoubleValue(v1);
                auto ld2 = GetLongDoubleValue(v2);
                SetLongDoubleValue(result, ld1 - ld2);
            } else if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 - n2);
                } else {
                    SetLongLongValue(result, n1 - n2);
                }
            }
        }
    } else {
        if (IsPointerType(v1.m_type_id) && IsPointerType(v2.m_type_id)) {
            v1.m_type_id = ResolveAlias(v1.m_type_id);
            auto& type1_1 = LogType(v1.m_type_id);
            auto& type1_2 = LogType(type1_1.m_sub_id);
            v2.m_type_id = ResolveAlias(v2.m_type_id);
            auto& type2_1 = LogType(v2.m_type_id);
            auto& type2_2 = LogType(type2_1.m_sub_id);
            if (type1_2.m_size == type2_2.m_size) {
                result.m_type_id = v1.m_type_id;
                result.m_size = v1.m_size;
                if (HasValue(v1) && HasValue(v2)) {
                    auto u1 = GetULongLongValue(v1);
                    auto u2 = GetULongLongValue(v2);
                    SetULongLongValue(result, (u1 - u2) / type1_2.m_size);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Mul(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOp(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsFloatingType(result.m_type_id)) {
                auto ld1 = GetLongDoubleValue(v1);
                auto ld2 = GetLongDoubleValue(v2);
                SetLongDoubleValue(result, ld1 * ld2);
            } else if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 * n2);
                } else {
                    SetLongLongValue(result, n1 * n2);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Div(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOp(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsFloatingType(result.m_type_id)) {
                auto ld1 = GetLongDoubleValue(v1);
                auto ld2 = GetLongDoubleValue(v2);
                SetLongDoubleValue(result, ld1 / ld2);
            } else if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 / n2);
                } else {
                    SetLongLongValue(result, n1 / n2);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Mod(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOpInt(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 / n2);
                } else {
                    SetLongLongValue(result, n1 / n2);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Not(const CR_TypedValue& value1) const {
    CR_TypedValue result = value1;
    if (IsIntegralType(result.m_type_id) && HasValue(result)) {
        char *p = reinterpret_cast<char *>(result.m_ptr);
        for (size_t i = 0; i < result.m_size; ++i) {
            *p = ~*p;
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Minus(const CR_TypedValue& value1) const
{
    CR_TypedValue result = value1;
    if (HasValue(result)) {
        if (IsIntegralType(value1.m_type_id)) {
            if (IsUnsignedType(value1.m_type_id)) {
                auto u = GetULongLongValue(value1);
                SetULongLongValue(result, u);
            } else {
                auto n = GetLongLongValue(value1);
                SetLongLongValue(result, n);
            }
        } else if (IsFloatingType(value1.m_type_id)) {
            auto ld = GetLongDoubleValue(value1);
            SetLongDoubleValue(result, -ld);
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::And(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOpInt(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(result)) {
            if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 & n2);
                } else {
                    SetLongLongValue(result, n1 & n2);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Or(const CR_TypedValue& value1,
                 const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOpInt(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(result)) {
            if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 | n2);
                } else {
                    SetLongLongValue(result, n1 | n2);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Xor(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOpInt(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(result)) {
            if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 ^ n2);
                } else {
                    SetLongLongValue(result, n1 ^ n2);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Eq(const CR_TypedValue& value1,
                 const CR_TypedValue& value2) const
{
    CR_TypedValue result;
    int compare = CompareValue(value1, value2);
    if (compare == 0) {
        IntOne(result);
    } else {
        IntZero(result);
    }
    return result;
}

CR_TypedValue
CR_NameScope::Ne(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue result;
    int compare = CompareValue(value1, value2);
    if (compare != 0) {
        IntOne(result);
    } else {
        IntZero(result);
    }
    return result;
}

CR_TypedValue
CR_NameScope::Gt(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue result;
    int compare = CompareValue(value1, value2);
    if (compare > 0) {
        IntOne(result);
    } else {
        IntZero(result);
    }
    return result;
}

CR_TypedValue
CR_NameScope::Lt(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue result;
    int compare = CompareValue(value1, value2);
    if (compare < 0) {
        IntOne(result);
    } else {
        IntZero(result);
    }
    return result;
}

CR_TypedValue
CR_NameScope::Ge(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue result;
    int compare = CompareValue(value1, value2);
    if (compare >= 0) {
        IntOne(result);
    } else {
        IntZero(result);
    }
    return result;
}

CR_TypedValue
CR_NameScope::Le(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue result;
    int compare = CompareValue(value1, value2);
    if (compare <= 0) {
        IntOne(result);
    } else {
        IntZero(result);
    }
    return result;
}

CR_TypedValue
CR_NameScope::Shl(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOpInt(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(result)) {
            if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 << n2);
                } else {
                    SetLongLongValue(result, n1 << n2);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::Shr(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto result = BiOpInt(v1, v2);
    if (result.m_type_id != cr_invalid_id) {
        if (HasValue(result)) {
            if (IsIntegralType(result.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(result.m_type_id)) {
                    SetULongLongValue(result, n1 >> n2);
                } else {
                    SetLongLongValue(result, n1 >> n2);
                }
            }
        }
    }
    return result;
}

CR_TypedValue
CR_NameScope::LNot(const CR_TypedValue& value1) const {
    CR_TypedValue result;
    if (IsZero(value1)) {
        IntOne(result);
    } else {
        IntZero(result);
    }
    return result;
}

CR_TypedValue
CR_NameScope::LAnd(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue result;
    if (IsZero(value1) || IsZero(value2)) {
        IntZero(result);
    } else {
        IntOne(result);
    }
    return result;
}

CR_TypedValue
CR_NameScope::LOr(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue result;
    if (IsNonZero(value1) || IsNonZero(value2)) {
        IntOne(result);
    } else {
        IntZero(result);
    }
    return result;
}

int CR_NameScope::GetIntValue(const CR_TypedValue& value) const {
    if (HasValue(value)) {
        auto int_value = StaticCast(m_int_type, value);
        if (HasValue(int_value)) {
            return int_value.get<int>();
        }
    }
    return 0;
}

void CR_NameScope::SetIntValue(CR_TypedValue& value, int n) const {
    value.m_type_id = m_int_type;
    value.assign<int>(n);
}

bool CR_NameScope::HasValue(const CR_TypedValue& value) const {
    if (value.m_ptr == NULL) {
        return false;
    }
    auto& type = LogType(value.m_type_id);
    return static_cast<int>(value.m_size) >= type.m_size;
}

void CR_NameScope::SetValue(
    CR_TypedValue& value, CR_TypeID tid, const void *ptr, size_t size) const
{
    value.m_type_id = tid;
    value.m_size = size;
    if (ptr == NULL) {
        value.clear();
        return;
    }
    if (IsIntegralType(tid)) {
        if (IsUnsignedType(tid)) {
            if (size == sizeof(int)) {
                value.assign<unsigned int>(
                    *reinterpret_cast<const unsigned int *>(ptr));
            } else if (size == sizeof(char)) {
                value.assign<unsigned char>(
                    *reinterpret_cast<const unsigned char *>(ptr));
            } else if (size == sizeof(short)) {
                value.assign<unsigned short>(
                    *reinterpret_cast<const unsigned short *>(ptr));
            } else if (size == sizeof(long)) {
                value.assign<unsigned long>(
                    *reinterpret_cast<const unsigned long *>(ptr));
            } else if (size == sizeof(long long)) {
                value.assign<unsigned long long>(
                    *reinterpret_cast<const unsigned long long *>(ptr));
            }
        } else {
            if (size == sizeof(int)) {
                value.assign<int>(
                    *reinterpret_cast<const int *>(ptr));
            } else if (size == sizeof(char)) {
                value.assign<char>(
                    *reinterpret_cast<const char *>(ptr));
            } else if (size == sizeof(short)) {
                value.assign<short>(
                    *reinterpret_cast<const short *>(ptr));
            } else if (size == sizeof(long)) {
                value.assign<long>(
                    *reinterpret_cast<const long *>(ptr));
            } else if (size == sizeof(long long)) {
                value.assign<long long>(
                    *reinterpret_cast<const long long *>(ptr));
            }
        }
    } else if (IsFloatingType(tid)){
        if (size == sizeof(float)) {
            value.assign<float>(*reinterpret_cast<const float *>(ptr));
        } else if (size == sizeof(double)) {
            value.assign<double>(*reinterpret_cast<const double *>(ptr));
        } else if (size == sizeof(long double)) {
            value.assign<long double>(*reinterpret_cast<const long double *>(ptr));
        }
    } else if (IsPointerType(tid)) {
        size_t n = reinterpret_cast<size_t>(ptr);
        value.assign<size_t>(n);
    } else {
        value.assign(ptr, size);
        value.m_text.clear();
    }
}

void *CR_NameScope::GetAddressPointer(unsigned long long addr, size_t size) {
    return NULL;
}

const void *
CR_NameScope::GetAddressPointer(unsigned long long addr, size_t size) const {
    return NULL;
}

////////////////////////////////////////////////////////////////////////////

bool CR_NameScope::LoadFromFiles(
    const std::string& prefix/* = ""*/,
    const std::string& suffix/* = ".dat"*/)
{
    m_types.clear();
    m_structs.clear();
    m_enums.clear();
    m_funcs.clear();
    m_mNameToTypeID.clear();
    m_mTypeIDToName.clear();
    m_mNameToVarID.clear();
    m_mVarIDToName.clear();

    std::ifstream in1(prefix + "types" + suffix);
    if (in1) {
        std::string line;
        std::getline(in1, line); // skip header
        for (; std::getline(in1, line); ) {
            CrChop(line);
            std::vector<std::string> fields;
            katahiromz::split(fields, line, "\t");

            CR_TypeID type_id = std::stol(fields[0], NULL, 0);
            std::string name = fields[1];
            CR_TypeFlags flags = std::stoul(fields[2], NULL, 0);
            CR_TypeID sub_id = std::stol(fields[3], NULL, 0);
            int count = std::stol(fields[4], NULL, 0);
            int size = std::stol(fields[5], NULL, 0);
            int align = std::stol(fields[6], NULL, 0);
            int alignas_ = std::stol(fields[7], NULL, 0);
            bool alignas_explicit = !!std::stol(fields[8], NULL, 0);
            std::string file = fields[9];
            int lineno = std::stol(fields[10], NULL, 0);

            if (name.size()) {
                MapNameToTypeID()[name] = type_id;
            }
            MapTypeIDToName()[type_id] = name;

            CR_LogType type;
            type.m_flags = flags;
            type.m_sub_id = sub_id;
            type.m_count = count;
            type.m_size = size;
            type.m_align = align;
            type.m_alignas = alignas_;
            type.m_alignas_explicit = alignas_explicit;
            type.m_location.set(file, lineno);
            m_types.emplace_back(type);
        }
    } else {
        return false;
    }

    std::ifstream in2(prefix + "structures" + suffix);
    if (in2) {
        std::string line;
        std::getline(in2, line); // skip header
        for (; std::getline(in2, line);) {
            CrChop(line);
            std::vector<std::string> fields;
            katahiromz::split(fields, line, "\t");

            //CR_StructID struct_id = std::stol(fields[0], NULL, 0);
            std::string name = fields[1];
            CR_TypeID type_id = std::stol(fields[2], NULL, 0);
            CR_TypeFlags flags = std::stoul(fields[3], NULL, 0);
            bool is_struct = !!std::stol(fields[4], NULL, 0);
            //int size = std::stol(fields[5], NULL, 0);
            int count = std::stol(fields[6], NULL, 0);
            int pack = std::stol(fields[7], NULL, 0);
            int align = std::stol(fields[8], NULL, 0);
            int alignas_ = std::stol(fields[9], NULL, 0);
            bool alignas_explicit = !!std::stol(fields[10], NULL, 0);
            //std::string file = fields[11];
            //int lineno = std::stol(fields[12], NULL, 0);

            CR_LogStruct ls;
            ls.m_tid = type_id;
            ls.m_is_struct = is_struct;
            ls.m_pack = pack;
            ls.m_align = align;
            ls.m_alignas = alignas_;
            ls.m_alignas_explicit = alignas_explicit;
            ls.m_is_complete = !(flags & TF_INCOMPLETE);

            for (int i = 0; i < count; ++i) {
                int j = 13 + 4 * i;
                auto& name = fields[j + 0];
                auto tid = std::stol(fields[j + 1], NULL, 0);
                auto bit_offset = std::stol(fields[j + 2], NULL, 0);
                auto bits = std::stol(fields[j + 3], NULL, 0);
                ls.m_members.emplace_back(tid, name, bit_offset, bits);
            }
            m_structs.emplace_back(ls);
        }
    } else {
        return false;
    }

    std::ifstream in3(prefix + "enums" + suffix);
    if (in3) {
        std::string line;
        std::getline(in3, line); // skip header
        for (; std::getline(in3, line);) {
            CrChop(line);
            std::vector<std::string> fields;
            katahiromz::split(fields, line, "\t");

            //CR_EnumID eid = std::stol(fields[0], NULL, 0);
            int num_items = std::stol(fields[1], NULL, 0);

            CR_LogEnum le;
            for (int i = 0; i < num_items; ++i) {
                int j = 2 + i * 2;
                std::string name = fields[j + 0];
                int value = std::stol(fields[j + 1], NULL, 0);
                le.m_mNameToValue[name] = value;
                le.m_mValueToName[value] = name;
            }
            m_enums.emplace_back(le);
        }
    } else {
        return false;
    }

    std::ifstream in4(prefix + "func_types" + suffix);
    if (in4) {
        std::string line;
        std::getline(in4, line); // skip header
        for (; std::getline(in4, line);) {
            CrChop(line);
            std::vector<std::string> fields;
            katahiromz::split(fields, line, "\t");

            //CR_FuncID fid = std::stol(fields[0], NULL, 0);
            int return_type = std::stol(fields[1], NULL, 0);
            int func_type = std::stol(fields[2], NULL, 0);
            bool ellipsis = !!std::stol(fields[3], NULL, 0);
            int param_count = std::stol(fields[4], NULL, 0);

            CR_LogFunc func;
            func.m_ellipsis = ellipsis;
            func.m_return_type = return_type;
            switch (func_type) {
            case 0: func.m_convention = CR_LogFunc::FT_CDECL; break;
            case 1: func.m_convention = CR_LogFunc::FT_STDCALL; break;
            case 2: func.m_convention = CR_LogFunc::FT_FASTCALL; break;
            default: assert(0);
            }

            for (int i = 0; i < param_count; ++i) {
                int j = 5 + i * 2;
                CR_TypeID tid = std::stol(fields[j + 0], NULL, 0);
                std::string name = fields[j + 1];
                func.m_params.emplace_back(tid, name);
            }
            m_funcs.emplace_back(func);
        }
    } else {
        return false;
    }

    std::ifstream in5(prefix + "vars" + suffix);
    if (in5) {
        std::string line;
        std::getline(in5, line); // skip header
        for (; std::getline(in5, line);) {
            CrChop(line);
            std::vector<std::string> fields;
            katahiromz::split(fields, line, "\t");

            //CR_VarID vid = std::stol(fields[0], NULL, 0);
            std::string name = fields[1];
            int type_id = std::stol(fields[2], NULL, 0);
            std::string text = fields[3];
            std::string extra = fields[4];
            std::string value_type = fields[5];
            std::string file = fields[6];
            int lineno = std::stol(fields[7], NULL, 0);

            CR_LogVar var;
            var.m_typed_value.m_type_id = type_id;
            var.m_typed_value.m_text = text;
            if (text.size() && value_type == "i" && IsIntegralType(type_id)) {
                bool is_unsigned = false;
                if (extra.find("u") != std::string::npos ||
                    extra.find("U") != std::string::npos)
                {
                    is_unsigned = true;
                }
                int sign = 1;
                if (text.size() && text[0] == '-') {
                    text = text.substr(1);
                    sign = -1;
                }
                if (extra.find("ll") != std::string::npos ||
                    extra.find("LL") != std::string::npos)
                {
                    if (is_unsigned) {
                        var.m_typed_value.assign<long long>(
                            sign * std::stoull(text, NULL, 0));
                    } else {
                        var.m_typed_value.assign<long long>(
                            sign * std::stoull(text, NULL, 0));
                    }
                } else if (extra.find('l') != std::string::npos ||
                           extra.find('L') != std::string::npos)
                {
                    if (is_unsigned) {
                        var.m_typed_value.assign<long>(
                            static_cast<long>(
                                sign * std::stoull(text, NULL, 0)));
                    } else {
                        var.m_typed_value.assign<long>(
                            static_cast<long>(
                                sign * std::stoull(text, NULL, 0)));
                    }
                } else {
                    if (is_unsigned) {
                        var.m_typed_value.assign<int>(
                            static_cast<int>(
                                sign * std::stoull(text, NULL, 0)));
                    } else {
                        var.m_typed_value.assign<int>(
                            static_cast<int>(
                                sign * std::stoull(text, NULL, 0)));
                    }
                }
            } else if (text.size() && value_type == "f" && IsFloatingType(type_id)) {
                if (extra.empty()) {
                    var.m_typed_value.assign<double>(std::stod(text));
                } else if (extra == "l" || extra == "L") {
                    var.m_typed_value.assign<long double>(std::stold(text));
                } else if (extra == "f" || extra == "F") {
                    var.m_typed_value.assign<float>(std::stof(text));
                }
            } else if (text.size() && value_type == "s" && IsStringType(type_id)) {
                std::string unescaped;
                if (CrUnscapeString(unescaped, text)) {
                    text = unescaped;
                    var.m_typed_value.assign(text.data(), text.size() + 1);
                    var.m_typed_value.m_text = text;
                } else {
                    ErrorInfo()->add_error("invalid string: '" + text + "'");
                }
            } else if (text.size() && value_type == "S" && IsWStringType(type_id)) {
                std::string unescaped;
                if (CrUnscapeString(unescaped, text)) {
                    std::wstring wstr = MAnsiToWide(unescaped.data());
                    var.m_typed_value.assign(wstr.data(), (wstr.size() + 1) * sizeof(WCHAR));
                    var.m_typed_value.m_text = text;
                } else {
                    ErrorInfo()->add_error("invalid string: '" + text + "'");
                }
            }
            var.m_typed_value.m_extra = extra;
            var.m_location.set(file, lineno);

            auto vid = m_vars.insert(var);
            m_mVarIDToName[vid] = name;
            if (name.size()) {
                m_mNameToVarID[name] = vid;
            }
        }
    } else {
        return false;
    }

    return true;
}

bool CR_NameScope::SaveToFiles(
    const std::string& prefix/* = ""*/,
    const std::string& suffix/* = ".dat"*/) const
{
    std::ofstream out1(prefix + "types" + suffix);
    if (out1) {
        out1 << "(type_id)\t(name)\t(flags)\t(sub_id)\t(count)\t(size)\t(align)\t(alignas)\t(alignas_explicit)\t(file)\t(line)" <<
            std::endl;
        for (CR_TypeID tid = 0; tid < LogTypes().size(); ++tid) {
            auto& type = LogType(tid);
            auto& location = type.location();
            std::string name;
            auto it = MapTypeIDToName().find(tid);
            if (it != MapTypeIDToName().end()) {
                name = it->second;
            }

            std::string file = location.m_file;
            int lineno = location.m_line;
            if (IsPredefinedType(tid)) {
                file = "(predefined)";
                lineno = 0;
            }
            for (auto& ch : file) {
                if (ch == '\\') {
                    ch = '/';
                }
            }

            out1 <<
                tid << "\t" <<
                name << "\t0x" <<
                std::hex << type.m_flags << std::dec << "\t" <<
                type.m_sub_id << "\t" <<
                type.m_count << "\t" <<
                type.m_size << "\t" <<
                type.m_align << "\t" <<
                type.m_alignas << "\t" <<
                type.m_alignas_explicit << "\t" <<
                file << "\t" <<
                lineno << std::endl;
        }
    } else {
        return false;
    }

    std::ofstream out2(prefix + "structures" + suffix);
    if (out2) {
        out2 << "(struct_id)\t(name)\t(type_id)\t(flags)\t(is_struct)\t(size)\t(count)\t(pack)\t(align)\t(alignas)\t(alignas_explicit)\t(file)\t(line)\t(item_1_name)\t(item_1_type_id)\t(item_1_bit_offset)\t(item_1_bits)\t(item_2_type_id)\t..." <<
            std::endl;
        for (CR_TypeID sid = 0; sid < LogStructs().size(); ++sid) {
            auto& ls = LogStruct(sid);
            auto tid = ls.m_tid;
            auto& type = LogType(tid);
            auto& location = type.location();
            std::string name;
            auto it = MapTypeIDToName().find(tid);
            if (it != MapTypeIDToName().end()) {
                name = it->second;
            }

            std::string file = location.m_file;
            for (auto& ch : file) {
                if (ch == '\\') {
                    ch = '/';
                }
            }

            out2 <<
                sid << "\t" <<
                name << "\t" <<
                tid << "\t0x" <<
                std::hex << type.m_flags << std::dec << "\t" <<
                ls.m_is_struct << "\t" <<
                type.m_size << "\t" <<
                ls.m_members.size() << "\t" <<
                ls.m_pack << "\t" <<
                ls.m_align << "\t" <<
                ls.m_alignas << "\t" <<
                ls.m_alignas_explicit << "\t" <<
                file << "\t" <<
                location.m_line;

            const size_t siz = ls.m_members.size();
            for (size_t i = 0; i < siz; ++i) {
                out2 << "\t" <<
                    ls.m_members[i].m_name << "\t" <<
                    ls.m_members[i].m_type_id << "\t" <<
                    ls.m_members[i].m_bit_offset << "\t" <<
                    ls.m_members[i].m_bits;
            }
            out2 << std::endl;
        }
    } else {
        return false;
    }

    std::ofstream out3(prefix + "enums" + suffix);
    if (out3) {
        out3 << "(enum_id)\t(num_items)\t(item_name_1)\t(item_value_1)\t(item_name_2)\t..." <<
            std::endl;
        for (CR_EnumID eid = 0; eid < LogEnums().size(); ++eid) {
            auto& le = LogEnum(eid);
            size_t num_items = le.m_mNameToValue.size();

            out3 <<
                eid << "\t" <<
                num_items;
            for (auto& item : le.m_mNameToValue) {
                out3 << "\t" <<
                    item.first << "\t" <<
                    item.second;
            }
            out3 << std::endl;
        }
    } else {
        return false;
    }

    std::ofstream out4(prefix + "func_types" + suffix);
    if (out4) {
        out4 << "(func_id)\t(return_type)\t(convention)\t(ellipsis)\t(param_count)\t(param_1_typeid)\t(param_1_name)\t(param_2_typeid)\t..." <<
            std::endl;
        for (size_t fid = 0; fid < LogFuncs().size(); ++fid) {
            auto& func = LogFunc(fid);

            out4 <<
                fid << "\t" <<
                func.m_return_type << "\t" <<
                func.m_convention << "\t" <<
                func.m_ellipsis << "\t" <<
                func.m_params.size();
            const size_t siz = func.m_params.size();
            for (size_t j = 0; j < siz; ++j) {
                out4 << "\t" <<
                    func.m_params[j].m_type_id << "\t" <<
                    func.m_params[j].m_name;
            }
            out4 << std::endl;
        }
    } else {
        return false;
    }

    std::ofstream out5(prefix + "vars" + suffix);
    if (out5) {
        out5 << "(var_id)\t(name)\t(type_id)\t(text)\t(extra)\t(value_type)\t(file)\t(line)" <<
            std::endl;
        for (size_t vid = 0; vid < LogVars().size(); ++vid) {
            auto& var = LogVar(vid);
            std::string name;
            auto it = MapVarIDToName().find(vid);
            if (it != MapVarIDToName().end()) {
                name = it->second;
            }
            auto& location = var.location();

            std::string value_type;
            auto tid = var.m_typed_value.m_type_id;
            if (IsIntegralType(tid)) {
                value_type = "i";
            } else if (IsFloatingType(tid)) {
                value_type = "f";
            } else if (IsStringType(tid)) {
                value_type = "s";
            } else if (IsWStringType(tid)) {
                value_type = "S";
            }

            std::string text;
            if (value_type == "s" || value_type == "S") {
                text = CrEscapeString(var.m_typed_value.m_text);
            } else {
                text = var.m_typed_value.m_text;
            }

            std::string file = location.m_file;
            for (auto& ch : file) {
                if (ch == '\\') {
                    ch = '/';
                }
            }

            out5 <<
                vid << "\t" <<
                name << "\t" <<
                tid << "\t" <<
                text << "\t" <<
                var.m_typed_value.m_extra << "\t" <<
                value_type << "\t" <<
                file << "\t" <<
                location.m_line << std::endl;
        }
    } else {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
