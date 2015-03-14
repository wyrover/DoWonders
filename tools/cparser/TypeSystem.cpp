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

std::string
CrJoin(const std::vector<std::string>& array, const std::string sep) {
    std::string str;
    if (array.size()) {
        str = array[0];
        for (size_t i = 1; i < array.size(); ++i) {
            str += sep;
            str += array[i];
        }
    }
    return str;
}

void
CrSplit(std::vector<std::string>& v, const std::string& s, char separator)
{
    std::size_t i = 0, j = s.find(separator);
    v.clear();
    while(j != std::string::npos) {
        v.push_back(s.substr(i, j - i));
        i = j + 1;
        j = s.find(separator, i);
    }
    v.push_back(s.substr(i, -1));
}

void CrChop(std::string& str) {
    if (str.size() && str[str.size() - 1] == '\n') {
        str.resize(str.size() - 1);
    }
}

////////////////////////////////////////////////////////////////////////////
// CR_TypedValue

CR_TypedValue::CR_TypedValue(void *ptr, size_t size) :
    m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id)
{
    assign(ptr, size);
}

void CR_TypedValue::Copy(const CR_TypedValue& value) {
    if (this != &value) {
        m_type_id = value.m_type_id;
        m_string = value.m_string;
        m_extra = value.m_extra;
        assign(value.m_ptr, value.m_size);
    }
}

CR_TypedValue::CR_TypedValue(const CR_TypedValue& value) :
    m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id)
{
    Copy(value);
}

CR_TypedValue& CR_TypedValue::operator=(const CR_TypedValue& value) {
    if (this != &value) {
        Copy(value);
    }
    return *this;
}

CR_TypedValue::CR_TypedValue(CR_TypedValue&& value) : m_ptr(NULL), m_size(0) {
    std::swap(m_ptr, value.m_ptr);
    std::swap(m_size, value.m_size);
    m_type_id = value.m_type_id;
    std::swap(m_string, value.m_string);
    std::swap(m_extra, value.m_extra);
}

CR_TypedValue& CR_TypedValue::operator=(CR_TypedValue&& value) {
    if (this != &value) {
        std::swap(m_ptr, value.m_ptr);
        std::swap(m_size, value.m_size);
        m_type_id = value.m_type_id;
        std::swap(m_string, value.m_string);
        std::swap(m_extra, value.m_extra);
    }
    return *this;
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

void CR_NameScope::Init() {
    AddType("void", TF_VOID, 0);

    AddType("char", TF_CHAR, 1);
    AddType("short", TF_SHORT, 2);
    AddType("long", TF_LONG, 4);
    AddType("__int64", TF_LONGLONG, 8);
    AddType("long long", TF_LONGLONG, 8);
    #ifdef __GNUC__
        AddType("__int128", TF_INT128, 16);
    #endif
    AddType("int", TF_INT, 4);

    AddType("unsigned char", TF_UNSIGNED | TF_CHAR, 1);
    AddType("unsigned short", TF_UNSIGNED | TF_SHORT, 2);
    AddType("unsigned long", TF_UNSIGNED | TF_LONG, 4);
    AddType("unsigned __int64", TF_UNSIGNED | TF_LONGLONG, 8);
    AddType("unsigned long long", TF_UNSIGNED | TF_LONGLONG, 8);
    #ifdef __GNUC__
        AddType("unsigned __int128", TF_UNSIGNED | TF_INT128, 16);
    #endif
    AddType("unsigned int", TF_UNSIGNED | TF_INT, 4);

    AddType("float", TF_FLOAT, 4);
    AddType("double", TF_DOUBLE, 8);

    // long double may differ between environments...
    #ifdef __GNUC__
        if (m_is_64bit)
            AddType("long double", TF_LONG | TF_DOUBLE, 16, 16, 16);
        else
            AddType("long double", TF_LONG | TF_DOUBLE, 12, 4, 4);
    #else
        AddType("long double", TF_LONG | TF_DOUBLE, 8);
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
    var.m_type_id = tid;
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
    var.m_type_id = tid;
    var.m_value = CR_TypedValue(tid);
    var.m_value.assign<int>(value);
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

CR_TypeID CR_NameScope::AddPtrType(
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
} // AddPtrType

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
                m_error_info.get()->add_warning(
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
                assert(bits <= item_size * 8);
                if (ls.m_pack < item_align) {
                    item_align = ls.m_pack;
                }
                if (prev_item_size == item_size || bits_remain == 0) {
                    // bitfield continuous
                    ls.m_members[i].m_bit_offset = byte_offset * 8 + bits_remain;
                    bits_remain += bits;
                } else {
                    // bitfield discontinuous
                    int bytes = (bits_remain + 7) / 8;
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
                    ls.m_members[i].m_bit_offset = byte_offset * 8;
                    bits_remain = bits;
                }
            } else {
                // not bitfield
                if (bits_remain) {
                    // the previous was bitfield
                    int prev_size_bits = prev_item_size * 8;
                    assert(prev_size_bits);
                    byte_offset += ((bits_remain + prev_size_bits - 1)
                                    / prev_size_bits * prev_size_bits) / 8;
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
                ls.m_members[i].m_bit_offset = byte_offset * 8;
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
        int prev_size_bits = prev_item_size * 8;
        assert(prev_size_bits);
        byte_offset += ((bits_remain + prev_size_bits - 1)
                        / prev_size_bits * prev_size_bits) / 8;
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
                m_error_info.get()->add_warning(
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
        str += CrJoin(array, ", ");
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
    const CR_TypeFlags flags =
        (TF_ALIAS | TF_FUNCTION | TF_STRUCT | TF_ENUM |
         TF_UNION | TF_VECTOR);
    if (type.m_flags & flags) {
        return false;
    }
    return true;
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

CR_TypeID CR_NameScope::_ResolveAliasRecurse(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    while (m_types[tid].m_flags & TF_ALIAS)
        tid = m_types[tid].m_sub_id;
    return tid;
}

CR_TypeID CR_NameScope::ResolveAliasAndCV(CR_TypeID tid) const {
    tid = ResolveAlias(tid);
    if (tid == cr_invalid_id)
        return tid;
    auto& type = LogType(tid);
    if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
        return ResolveAliasAndCV(type.m_sub_id);
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

bool CR_NameScope::GetVarIntValue(int& int_value, const std::string& name) const {
    auto it = MapNameToVarID().find(name);
    if (it != MapNameToVarID().end()) {
        CR_VarID vid = it->second;
        auto& var = LogVar(vid);
        auto& value = var.m_value;
        int size = SizeOfType(value.m_type_id);
        if (size >= sizeof(int) && !value.empty()) {
            int_value = value.get<int>();
            return true;
        }
    }
    return false;
}

CR_TypeID CR_NameScope::GetConstIntType() {
    auto tid = TypeIDFromFlags(TF_INT);
    return AddConstType(tid);
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
            CrSplit(fields, line, '\t');

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
            CrSplit(fields, line, '\t');

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
            CrSplit(fields, line, '\t');

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
            CrSplit(fields, line, '\t');

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
        for (; std::getline(in4, line);) {
            CrChop(line);
            std::vector<std::string> fields;
            CrSplit(fields, line, '\t');

            //CR_VarID vid = std::stol(fields[0], NULL, 0);
            std::string name = fields[1];
            int type_id = std::stol(fields[2], NULL, 0);
            std::string int_value = fields[3];
            std::string file = fields[4];
            int lineno = std::stol(fields[5], NULL, 0);

            CR_LogVar var;
            var.m_type_id = type_id;
            if (int_value.size()) {
                var.m_value = CR_TypedValue(type_id, std::stol(int_value, NULL, 0));
            } else {
                var.m_value = CR_TypedValue(type_id);
            }
            var.m_location.set(file, lineno);

            auto vid = m_vars.insert(var);
            m_mVarIDToName[vid] = name;
            m_mNameToVarID[name] = vid;
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
                location.m_file << "\t" <<
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
        out5 << "(var_id)\t(name)\t(type_id)\t(int_value)\t(file)\t(line)" << std::endl;
        for (size_t vid = 0; vid < LogVars().size(); ++vid) {
            auto& var = LogVar(vid);
            std::string name;
            auto it = MapVarIDToName().find(vid);
            if (it != MapVarIDToName().end()) {
                name = it->second;
            }
            auto& location = var.location();

            int int_value = 0;
            std::string value;
            if (GetVarIntValue(int_value, name)) {
                value = std::to_string(int_value);
            }

            out5 <<
                vid << "\t" <<
                name << "\t" <<
                var.m_type_id << "\t" <<
                value << "\t" <<
                location.m_file << "\t" <<
                location.m_line << std::endl;
        }
    } else {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
