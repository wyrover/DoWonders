////////////////////////////////////////////////////////////////////////////
// TypeSystem.cpp
// Copyright (C) 2014-2015 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse. See file ReadMe.txt and License.txt.
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

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
// CR_LogStruct

bool CR_LogStruct::operator==(const CR_LogStruct& ls) const {
    return m_is_struct == ls.m_is_struct &&
           m_type_list == ls.m_type_list &&
           m_name_list == ls.m_name_list &&
           m_bit_offset_list == ls.m_bit_offset_list &&
           m_bits_list == ls.m_bits_list &&
           m_pack == ls.m_pack &&
           m_align == ls.m_align;
}

bool CR_LogStruct::operator!=(const CR_LogStruct& ls) const {
    return m_is_struct != ls.m_is_struct ||
           m_type_list != ls.m_type_list ||
           m_name_list != ls.m_name_list ||
           m_bit_offset_list != ls.m_bit_offset_list ||
           m_bits_list != ls.m_bits_list ||
           m_pack != ls.m_pack ||
           m_align != ls.m_align;
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
    AddType("__int128", TF_INT128, 16);
    AddType("int", TF_INT, 4);

    AddType("unsigned char", TF_UNSIGNED | TF_CHAR, 1);
    AddType("unsigned short", TF_UNSIGNED | TF_SHORT, 2);
    AddType("unsigned long", TF_UNSIGNED | TF_LONG, 4);
    AddType("unsigned __int64", TF_UNSIGNED | TF_LONGLONG, 8);
    AddType("unsigned long long", TF_UNSIGNED | TF_LONGLONG, 8);
    AddType("unsigned __int128", TF_UNSIGNED | TF_INT128, 16);
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

    AddType("va_list", TF_VA_LIST, (Is64Bit() ? 8 : 4));

    // CodeReverse extension
    AddType("enumitem", TF_ENUMITEM, 4);
}

CR_TypeID CR_NameScope::AddAliasType(
    const CR_String& name, CR_TypeID tid, const CR_Location& location)
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
    const CR_String& name, CR_TypeID tid, const CR_Location& location) {
    assert(tid != cr_invalid_id);
    CR_LogVar var;
    var.m_type_id = tid;
    var.m_int_value = 0;
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
        name = CR_String("const ") + name;
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
    if (func.m_type_list.size() == 1 && func.m_type_list[0] == 0) {
        // parameter list is void
        func.m_type_list.clear();
        func.m_name_list.clear();
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
    const CR_String& name, const CR_LogStruct& ls,
    int alignas_, const CR_Location& location)
{
    CR_LogType type1;
    if (name.empty()) {     // name is empty
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_STRUCT | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_type_list.size();
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
        type1.m_count = ls.m_type_list.size();
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
        if (ls.m_type_list.size()) {
            // overwrite the definition if type list not empty
            auto& type1 = LogType(tid2);
            type1.m_count = ls.m_type_list.size();
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
    const CR_String& name, const CR_LogStruct& ls,
    int alignas_, const CR_Location& location)
{
    CR_LogType type1;
    if (name.empty()) { // name is empty
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_UNION | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_type_list.size();
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
        type1.m_count = ls.m_type_list.size();
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
        if (ls.m_type_list.size()) {
            // overwrite the definition if type list not empty
            auto& type1 = LogType(tid2);
            type1.m_count = ls.m_type_list.size();
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
    const CR_String& name, const CR_LogEnum& le, const CR_Location& location)
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
    const size_t siz = ls.m_type_list.size();
    bool is_complete = true;
    for (std::size_t i = 0; i < siz; ++i) {
        auto tid2 = ls.m_type_list[i];
        auto& type2 = LogType(tid2);
        if (type2.m_flags & TF_INCOMPLETE) {
            if (!CompleteType(tid2, type2)) {
                auto& name = ls.m_name_list[i];
                m_error_info.get()->add_warning(
                    type2.location(), "'" +
                        name + "' has incomplete type");
                is_complete = false;
            }
        }
        if (ls.m_bits_list[i] != -1 || (type2.m_flags & TF_BITFIELD)) {
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
        ls.m_bit_offset_list.clear();
        for (std::size_t i = 0; i < siz; ++i) {
            auto tid2 = ls.m_type_list[i];
            auto& type2 = LogType(tid2);
            int item_size = type2.m_size;            // size of type
            int item_align = type2.m_align;          // alignment requirement
            if (type1.m_alignas < type2.m_alignas) {
                type1.m_alignas = type2.m_alignas;
            }
            int bits = ls.m_bits_list[i];            // bits of bitfield
            if (bits != -1) {
                // the bits specified as bitfield
                assert(bits <= item_size * 8);
                if (ls.m_pack < item_align) {
                    item_align = ls.m_pack;
                }
                if (prev_item_size == item_size || bits_remain == 0) {
                    // bitfield continuous
                    ls.m_bit_offset_list.push_back(byte_offset * 8 + bits_remain);
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
                    ls.m_bit_offset_list.push_back(byte_offset * 8);
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
                ls.m_bit_offset_list.push_back(byte_offset * 8);
                byte_offset += item_size;
            }
            if (max_align < item_align) {
                max_align = item_align;
            }
            prev_item_size = item_size;
        }
    } else {
        ls.m_bit_offset_list.assign(siz, -1);
    }
    assert(ls.m_bit_offset_list.size() == ls.m_type_list.size());

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
    if (is_complete && ls.m_type_list.size()) {
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

    // every field offset of union is zero
    ls.m_bit_offset_list.assign(ls.m_type_list.size(), 0);
    assert(ls.m_bit_offset_list.size() == ls.m_type_list.size());

    // check completeness for each field
    const size_t siz = ls.m_type_list.size();
    bool is_complete = true;
    for (std::size_t i = 0; i < siz; ++i) {
        auto tid2 = ls.m_type_list[i];
        auto& type2 = LogType(tid2);
        if (type2.m_flags & TF_INCOMPLETE) {
            if (!CompleteType(tid2, type2)) {
                auto& name = ls.m_name_list[i];
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
        auto tid2 = ls.m_type_list[i];
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
    if (is_complete && ls.m_type_list.size()) {
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
            const auto& type2 = LogType(type.m_sub_id);
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
            const auto& type2 = LogType(type.m_sub_id);
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
            const auto& type2 = LogType(type.m_sub_id);
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
            const auto& type2 = LogType(type.m_sub_id);
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
        for (auto tid : ls.m_type_list) {
            CompleteType(tid);
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

CR_String CR_NameScope::StringOfEnum(
    const CR_String& name, CR_EnumID eid) const
{
    assert(eid != cr_invalid_id);
    if (eid == cr_invalid_id) {
        return "";  // invalid ID
    }
    const auto& e = m_enums[eid];
    CR_String str = StringOfEnumTag(name);
    if (!e.empty()) {
        str += "{ ";
        for (auto it : e.MapNameToValue()) {
            str += it.first;
            str += " = ";
            str += std::to_string(it.second);
            str += ", ";
        }
        str += "} ";
    }
    return str;
} // StringOfEnum

CR_String
CR_NameScope::StringOfStructTag(
    const CR_String& name, const CR_LogStruct& s) const
{
    CR_String str;

    if (s.m_is_struct)
        str += "struct ";
    else
        str += "union ";

    const auto& type = LogType(s.m_tid);
    if (type.m_alignas) {
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

CR_String CR_NameScope::StringOfStruct(
    const CR_String& name, CR_StructID sid) const
{
    const auto& s = LogStruct(sid);
    CR_String str = StringOfStructTag(name, s);
    if (!s.empty()) {
        str += "{ ";
        const std::size_t siz = s.m_type_list.size();
        for (std::size_t i = 0; i < siz; i++) {
            str += StringOfType(s.m_type_list[i], s.m_name_list[i], false);
            if (s.m_bits_list[i] != -1) {
                str += " : ";
                str += std::to_string(s.m_bits_list[i]);
            }
            str += "; ";
        }
        str += "} ";
    }
    return str;
} // StringOfStruct

CR_String CR_NameScope::StringOfType(
    CR_TypeID tid, const CR_String& name,
    bool expand/* = true*/, bool no_convension/* = false*/) const
{
    const auto& type = LogType(tid);
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
    if (type.m_flags & TF_ENUMITEM) {
        // if type was enumitem (CodeReverse extension)
        return "enumitem " + type_name + " = " + name;
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
        assert(type.m_sub_id < m_funcs.size());
        const auto& func = m_funcs[type.m_sub_id];
        auto rettype = StringOfType(func.m_return_type, "", false);
        auto paramlist = StringOfParamList(func.m_type_list, func.m_name_list);
        CR_String convension;
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
        const auto& type2 = LogType(sub_id);
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

CR_String CR_NameScope::StringOfParamList(
    const CR_TypeSet& type_list,
    const CR_StringSet& name_list) const
{
    assert(type_list.size() == name_list.size());
    std::size_t i, size = type_list.size();
    CR_String str;
    if (size > 0) {
        assert(type_list[0] != cr_invalid_id);
        str += StringOfType(type_list[0], name_list[0], false);
        for (i = 1; i < size; i++) {
            str += ", ";
            assert(type_list[i] != cr_invalid_id);
            str += StringOfType(type_list[i], name_list[i], false);
        }
    } else {
        str += "void";
    }
    return str;
} // StringOfParamList

bool CR_NameScope::IsCrExtendedType(CR_TypeID tid) const {
    const auto& type = LogType(tid);
    if (type.m_flags & (TF_POINTER | TF_ARRAY | TF_VECTOR | TF_CONST)) {
        return IsCrExtendedType(type.m_sub_id);
    }
    const CR_TypeFlags flags = TF_ENUMITEM;
    if (type.m_flags & flags) {
        return true;
    }
    return false;
} // IsCrExtendedType

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
    const auto& type = LogType(tid);
    if (type.m_flags & (TF_POINTER | TF_ARRAY | TF_CONST)) {
        return IsPredefinedType(type.m_sub_id);
    }
    const CR_TypeFlags flags =
        (TF_ALIAS | TF_FUNCTION | TF_STRUCT | TF_ENUM |
         TF_UNION | TF_ENUMITEM | TF_VECTOR);
    if (type.m_flags & flags) {
        return false;
    }
    return true;
} // IsPredefinedType

bool CR_NameScope::IsIntegerType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    const CR_TypeFlags not_flags =
        (TF_DOUBLE | TF_FLOAT | TF_POINTER | TF_ARRAY | TF_VECTOR |
         TF_FUNCTION | TF_VA_LIST | TF_STRUCT | TF_UNION | TF_ENUM);
    if (m_types[tid].m_flags & not_flags)
        return false;
    const CR_TypeFlags flags =
        (TF_INT | TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG | TF_ENUMITEM);
    if (m_types[tid].m_flags & flags)
        return true;
    if (m_types[tid].m_flags & TF_CONST)
        return IsIntegerType(m_types[tid].m_sub_id);
    return false;
} // IsIntegerType


bool CR_NameScope::IsFloatingType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    if (m_types[tid].m_flags & (TF_DOUBLE | TF_FLOAT))
        return true;
    if (m_types[tid].m_flags & TF_CONST)
        return IsFloatingType(m_types[tid].m_sub_id);
    return false;
} // IsFloatingType

bool CR_NameScope::IsUnsignedType(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    if (m_types[tid].m_flags & TF_UNSIGNED)
        return true;
    if (m_types[tid].m_flags & TF_CONST)
        return IsUnsignedType(m_types[tid].m_sub_id);
    return false;
} // IsUnsignedType

CR_TypeID CR_NameScope::_ResolveAliasRecurse(CR_TypeID tid) const {
    assert(tid != cr_invalid_id);
    while (m_types[tid].m_flags & TF_ALIAS)
        tid = m_types[tid].m_sub_id;
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

CR_TypeID CR_NameScope::TypeIDFromName(const CR_String& name) const {
    auto it = m_mNameToTypeID.find(name);
    if (it != m_mNameToTypeID.end())
        return it->second;
    else
        return cr_invalid_id;
}

CR_String CR_NameScope::NameFromTypeID(CR_TypeID tid) const {
    auto it = m_mTypeIDToName.find(tid);
    if (it != m_mTypeIDToName.end())
        return it->second;
    else
        return "";
}

////////////////////////////////////////////////////////////////////////////
