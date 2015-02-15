////////////////////////////////////////////////////////////////////////////
// TypeSystem.h
// Copyright (C) 2014 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse.
////////////////////////////////////////////////////////////////////////////

#ifndef TYPESYSTEM_H_
#define TYPESYSTEM_H_

////////////////////////////////////////////////////////////////////////////
// CP_TypeFlags

enum {
    TF_VOID         = 0x00000001,
    TF_CHAR         = 0x00000002,
    TF_SHORT        = 0x00000004,
    TF_LONG         = 0x00000008,
    TF_LONGLONG     = 0x00000010,
    TF_INT          = 0x00000020,
    TF_VA_LIST      = 0x00000040,
    TF_FLOAT        = 0x00000080,
    TF_DOUBLE       = 0x00000100,
    TF_SIGNED       = 0,
    TF_UNSIGNED     = 0x00000200,
    TF_XSIGNED      = 0x00000400, // CodeReverse extension: signed and/or unsigned
    TF_STRUCT       = 0x00000800,
    TF_UNION        = 0x00001000,
    TF_ENUM         = 0x00002000,
    TF_POINTER      = 0x00004000,
    TF_ARRAY        = 0x00008000,
    TF_FUNCTION     = 0x00010000,
    TF_CDECL        = 0x00020000,
    TF_STDCALL      = 0x00040000,
    TF_FASTCALL     = 0x00080000,
    TF_CONST        = 0x00100000,
    TF_VOLATILE     = 0x00200000,
    TF_COMPLEX      = 0x00400000,
    TF_IMAGINARY    = 0x00800000,
    TF_ATOMIC       = 0x01000000,
    TF_EXTERN       = 0x02000000,
    TF_STATIC       = 0x04000000,
    TF_THREADLOCAL  = 0x08000000,
    TF_INLINE       = 0x10000000,
    TF_ALIAS        = 0x20000000,
    TF_ENUMITEM     = 0x40000000,
    TF_INT128       = 0x80000000
};
typedef unsigned long CP_TypeFlags;

////////////////////////////////////////////////////////////////////////////
// CpNormalizeTypeFlags

inline CP_TypeFlags CpNormalizeTypeFlags(CP_TypeFlags flags) {
    if (flags & TF_INT) {
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
        flags |= TF_INT;
    }
    if (flags == 0)
        flags = TF_INT;
    return flags & ~(TF_EXTERN | TF_STATIC | TF_INLINE);
}

////////////////////////////////////////////////////////////////////////////
// IDs

// CP_ID --- ID
typedef std::size_t CP_ID;

// CP_TypeID --- type ID
typedef CP_ID CP_TypeID;

// CP_FuncID --- function ID
typedef CP_ID CP_FuncID;

// CP_VarID --- variable ID
typedef CP_ID CP_VarID;

// CP_StructID --- struct or union ID
typedef CP_ID CP_StructID;

// CP_EnumID --- enum ID
typedef CP_ID CP_EnumID;

// cr_invalid_id --- invalid ID
#define cr_invalid_id   static_cast<CP_ID>(-1)

////////////////////////////////////////////////////////////////////////////
// CP_TypeSet

typedef CP_DeqSet<CP_TypeID> CP_TypeSet;
typedef CP_DeqSet<CP_ID> CP_IDSet;

////////////////////////////////////////////////////////////////////////////
// CP_LogFunc

struct CP_LogFunc
{
    bool                    m_ellipsis;
    CP_TypeSet              m_type_list;
    CP_StringSet            m_name_list;
    CP_TypeID               m_return_type;
    enum {
        FT_CDECL, FT_STDCALL, FT_FASTCALL
    } m_func_type;

    CP_LogFunc() :
        m_ellipsis(false), m_return_type(0), m_func_type(FT_CDECL) { }

    CP_LogFunc(const CP_LogFunc& lf) :
        m_ellipsis(lf.m_ellipsis),
        m_type_list(lf.m_type_list),
        m_name_list(lf.m_name_list),
        m_return_type(lf.m_return_type),
        m_func_type(lf.m_func_type) { }

    void operator=(const CP_LogFunc& lf) {
        m_ellipsis = lf.m_ellipsis;
        m_type_list = lf.m_type_list;
        m_name_list = lf.m_name_list;
        m_return_type = lf.m_return_type;
        m_func_type = lf.m_func_type;
    }
}; // struct CP_LogFunc

////////////////////////////////////////////////////////////////////////////
// CP_LogType

struct CP_LogType
{
    CP_TypeFlags m_flags;

    // For TF_POINTER:              the type ID (CP_TypeID)
    // For TF_ARRAY:                the type ID (CP_TypeID)
    // For TF_CONST:                the type ID (CP_TypeID)
    // For TF_CONST | TF_POINTER:   the type ID (CP_TypeID)
    // For TF_FUNCTION:             the function ID (CP_FuncID)
    // For TF_STRUCT:               the struct ID (CP_StructID)
    // For TF_ENUM:                 the enum ID (CP_EnumID)
    // For TF_UNION:                the struct ID (CP_StructID)
    // For TF_ENUMITEM:             the enum ID (CP_EnumID)
    // otherwise: zero
    CP_ID        m_id;

    size_t       m_count;   // for TF_ARRAY
    size_t       m_size;
    CP_Location  m_loc;

    CP_LogType() : m_flags(0), m_id(0), m_count(0), m_size(0) { }

    CP_LogType(CP_TypeFlags flags, size_t size, const CP_Location& location) :
        m_flags(flags), m_id(0), m_count(0), m_size(size), m_loc(location) { }

    bool operator==(const CP_LogType& type) const {
        return m_flags == type.m_flags &&
               m_id == type.m_id &&
               m_count == type.m_count;
    }

    bool operator!=(const CP_LogType& type) const {
        return m_flags != type.m_flags ||
               m_id != type.m_id ||
               m_count != type.m_count;
    }

          CP_Location& location()       { return m_loc; }
    const CP_Location& location() const { return m_loc; }
}; // struct CP_LogType

////////////////////////////////////////////////////////////////////////////
// CP_LogStruct -- logical structure or union

struct CP_LogStruct
{
    bool                    m_struct_or_union;
    CP_TypeSet              m_type_list;
    CP_StringSet            m_name_list;
    CP_DeqSet<size_t>       m_bitfield;
    size_t                  m_pack;

    CP_LogStruct(bool struct_or_union = true) :
        m_struct_or_union(struct_or_union), m_pack(1) { }

    CP_LogStruct(const CP_LogStruct& ls) :
        m_struct_or_union(ls.m_struct_or_union),
        m_type_list(ls.m_type_list),
        m_name_list(ls.m_name_list),
        m_bitfield(ls.m_bitfield),
        m_pack(ls.m_pack) { }

    void operator=(const CP_LogStruct& ls) {
        m_struct_or_union = ls.m_struct_or_union;
        m_type_list = ls.m_type_list;
        m_name_list = ls.m_name_list;
        m_bitfield = ls.m_bitfield;
        m_pack = ls.m_pack;
    }

    size_t FindName(const CP_String& name) const {
        for (size_t i = 0; i < m_name_list.size(); i++) {
            if (m_name_list[i] == name)
                return i;
        }
        return cr_invalid_id;
    }

    bool operator==(const CP_LogStruct& ls) const {
        return m_struct_or_union == ls.m_struct_or_union &&
               m_type_list == ls.m_type_list &&
               m_name_list == ls.m_name_list &&
               m_bitfield == ls.m_bitfield &&
               m_pack == ls.m_pack;
    }

    bool operator!=(const CP_LogStruct& ls) const {
        return m_struct_or_union != ls.m_struct_or_union ||
               m_type_list != ls.m_type_list ||
               m_name_list != ls.m_name_list ||
               m_bitfield != ls.m_bitfield ||
               m_pack != ls.m_pack;
    }
}; // struct CP_LogStruct

////////////////////////////////////////////////////////////////////////////
// CP_LogEnum

struct CP_LogEnum
{
    CP_LogEnum() { }

    CP_LogEnum(const CP_LogEnum& le) :
        m_mNameToValue(le.m_mNameToValue),
        m_mValueToName(le.m_mValueToName) { }

    void operator=(const CP_LogEnum& le) {
        m_mNameToValue = le.m_mNameToValue;
        m_mValueToName = le.m_mValueToName;
    }

    CP_UnorderedMap<CP_String, int>& MapNameToValue()
    { return m_mNameToValue; }

    CP_UnorderedMap<int, CP_String>& MapValueToName()
    { return m_mValueToName; }

    const CP_UnorderedMap<CP_String, int>& MapNameToValue() const
    { return m_mNameToValue; }

    const CP_UnorderedMap<int, CP_String>& MapValueToName() const
    { return m_mValueToName; }

protected:
    CP_UnorderedMap<CP_String, int>     m_mNameToValue;
    CP_UnorderedMap<int, CP_String>     m_mValueToName;
}; // struct CP_LogEnum

////////////////////////////////////////////////////////////////////////////
// CP_LogVar

struct CP_LogVar
{
    CP_LogVar() : m_has_value(false) { }

    bool            m_has_value;
    CP_TypeID       m_type_id;
    CP_Location     m_loc;
    CP_ID           m_enum_type_id;
    union {
        char        m_char_value;
        short       m_short_value;
        long        m_long_value;
        long long   m_long_long_value;
        int         m_int_value;
        float       m_float_value;
        double      m_double_value;
        long double m_long_double_value;
        void *      m_pointer_value;
    };

          CP_Location& location()       { return m_loc; }
    const CP_Location& location() const { return m_loc; }
}; // struct CP_LogVar

////////////////////////////////////////////////////////////////////////////
// CP_NameScope

class CP_NameScope
{
public:
    CP_NameScope(bool is_64bit) : m_is_64bit(is_64bit) {
        Init();
    }

    CP_NameScope(const CP_NameScope& ns) :
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
        m_vars(ns.m_vars) { }

    CP_NameScope& operator=(const CP_NameScope& ns) {
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
        return *this;
    }

    bool Is64Bit() const {
        return m_is_64bit;
    }

    void Set64Bit(bool is_64bit) {
        m_is_64bit = is_64bit;
    }

    void Init() {
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
        AddType("long double", TF_LONG | TF_DOUBLE, 10);

        AddType("va_list", TF_VA_LIST, (Is64Bit() ? 8 : 4));

        // CodeReverse extension
        AddType("xsigned char", TF_XSIGNED | TF_CHAR, 1);
        AddType("xsigned short", TF_XSIGNED | TF_SHORT, 2);
        AddType("xsigned long", TF_XSIGNED | TF_LONG, 4);
        AddType("xsigned __int64", TF_XSIGNED | TF_LONGLONG, 8);
        AddType("xsigned long long", TF_XSIGNED | TF_LONGLONG, 8);
        AddType("xsigned __int128", TF_XSIGNED | TF_LONGLONG, 16);
        AddType("xsigned int", TF_XSIGNED | TF_INT, 4);
        AddType("enumitem", TF_ENUMITEM, 4);
    }

    CP_TypeID TypeIDFromFlags(CP_TypeFlags flags) const {
        const size_t siz = m_types.size();
        for (size_t i = 0; i < siz; ++i) {
            if (m_types[i].m_flags == flags)
                return i;
        }
        return cr_invalid_id;
    }

    size_t SizeFromFlags(CP_TypeFlags flags) const {
        flags = CpNormalizeTypeFlags(flags & ~TF_CONST);
        for (const auto& t : m_types) {
            if (t.m_flags == flags)
                return t.m_size;
        }
        return 0;
    }

    CP_TypeID TypeIDFromName(const CP_String& name) const {
        auto it = m_mNameToTypeID.find(name);
        if (it != m_mNameToTypeID.end())
            return it->second;
        else
            return cr_invalid_id;
    }

    CP_String NameFromTypeID(CP_TypeID tid) const {
        auto it = m_mTypeIDToName.find(tid);
        if (it != m_mTypeIDToName.end())
            return it->second;
        else
            return "";
    }

    CP_TypeID AddType(const CP_String& name, const CP_LogType& lt) {
        auto tid = m_types.AddUnique(lt);
        if (!name.empty()) {
            m_mNameToTypeID[name] = tid;
            m_mTypeIDToName[tid] = name;
        }
        return tid;
    }

    CP_TypeID AddType(const CP_String& name, CP_TypeFlags flags, size_t size,
                      const CP_Location& location = CP_Location())
    {
        return AddType(name, CP_LogType(flags, size, location));
    }

    CP_TypeID AddAliasType(const CP_String& name, CP_TypeID tid,
                           const CP_Location& location)
    {
        assert(!name.empty());
        CP_LogType lt;
        lt.m_flags = TF_ALIAS;
        lt.m_id = tid;
        lt.m_size = GetSizeofType(tid);
        lt.location() = location;
        tid = m_types.insert(lt);
        m_mNameToTypeID[name] = tid;
        m_mTypeIDToName[tid] = name;
        return tid;
    }

    CP_VarID AddVar(const CP_String& name, CP_TypeID tid,
                    const CP_Location& location)
    {
        assert(tid != cr_invalid_id);
        CP_LogVar var;
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

    CP_VarID AddVar(const CP_String& name, const CP_LogType& lt) {
        auto tid = m_types.AddUnique(lt);
        if (!name.empty()) {
            m_mNameToTypeID[name] = tid;
            m_mTypeIDToName[tid] = name;
        }
        return AddVar(name, tid, lt.location());
    }

    CP_TypeID AddConstType(CP_TypeID tid) {
        assert(tid != cr_invalid_id);
        CP_LogType lt;
        lt.m_flags = TF_CONST;
        lt.m_id = tid;
        lt.m_size = GetSizeofType(tid);
        auto newtid = m_types.AddUnique(lt);
        auto name = NameFromTypeID(tid);
        if (!name.empty()) {
            name = CP_String("const ") + name;
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
        }
        return newtid;
    }

    CP_TypeID AddPtrType(CP_TypeID tid, CP_TypeFlags flags,
                         const CP_Location& location)
    {
        assert(tid != cr_invalid_id);
        CP_LogType lt;
        lt.m_flags = TF_POINTER | flags;
        lt.m_id = tid;
        lt.m_size = (Is64Bit() ? 8 : 4);
        lt.location() = location;
        auto newtid = m_types.AddUnique(lt);
        auto type = m_types[tid];
        auto name = NameFromTypeID(tid);
        if (!name.empty()) {
            if (!(type.m_flags & TF_FUNCTION)) {
                if (flags & TF_CONST)
                    name += "* const ";
                else
                    name += "*";
            }
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
        }
        return newtid;
    }

    CP_TypeID AddArrayType(CP_TypeID tid, int count,
                           const CP_Location& location)
    {
        assert(tid != cr_invalid_id);
        CP_LogType lt;
        lt.m_flags = TF_ARRAY;
        lt.m_id = tid;
        lt.m_count = count;
        lt.m_size = GetSizeofType(tid) * count;
        lt.location() = location;
        tid = m_types.AddUnique(lt);
        m_mTypeIDToName[tid] = "";
        return tid;
    }

    CP_TypeID AddFuncType(const CP_LogFunc& lf, const CP_Location& location) {
        CP_LogFunc func(lf);
        if (func.m_type_list.size() == 1 && func.m_type_list[0] == 0) {
            // parameter list is void
            func.m_type_list.clear();
            func.m_name_list.clear();
        }
        auto fid = m_funcs.insert(func);
        CP_LogType lt;
        lt.m_flags = TF_FUNCTION;
        lt.m_id = fid;
        lt.m_size = (Is64Bit() ? 8 : 4);
        lt.location() = location;
        CP_TypeID tid = m_types.AddUnique(lt);
        m_mTypeIDToName[tid] = "";
        return tid;
    }

    CP_TypeID AddStructType(const CP_String& name, const CP_LogStruct& ls,
                            const CP_Location& location)
    {
        CP_LogType lt;
        if (name.empty()) {
            CP_StructID sid = m_structs.insert(ls);
            lt.m_flags = (ls.m_struct_or_union ? TF_STRUCT : TF_UNION);
            lt.m_id = sid;
            lt.m_size = GetSizeofStruct(sid);
            lt.location() = location;
            CP_TypeID newtid = m_types.AddUnique(lt);
            m_mTypeIDToName[newtid] = name;
            return newtid;
        }
        auto it = m_mNameToTypeID.find(name);
        if (it == m_mNameToTypeID.end()) {
            CP_StructID sid = m_structs.insert(ls);
            lt.m_flags = (ls.m_struct_or_union ? TF_STRUCT : TF_UNION);
            lt.m_id = sid;
            lt.m_size = GetSizeofStruct(sid);
            lt.location() = location;
            CP_TypeID newtid = m_types.AddUnique(lt);
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
            return newtid;
        } else {
            CP_TypeID tid = it->second;
            while (m_types[tid].m_flags & TF_ALIAS)
                tid = m_types[tid].m_id;
            assert(m_types[tid].m_flags & (TF_STRUCT | TF_UNION));
            CP_StructID sid = m_types[tid].m_id;
            if (ls.m_type_list.size())
                m_structs[sid] = ls;
            return tid;
        }
    }

    CP_TypeID AddUnionType(const CP_String& name, const CP_LogStruct& ls,
                           const CP_Location& location)
    {
        CP_LogType lt;
        if (name.empty()) {
            CP_StructID sid = m_structs.insert(ls);
            lt.m_flags = (ls.m_struct_or_union ? TF_STRUCT : TF_UNION);
            lt.m_id = sid;
            lt.m_size = GetSizeofUnion(sid);
            lt.location() = location;
            CP_TypeID newtid = m_types.AddUnique(lt);
            m_mTypeIDToName[newtid] = name;
            return newtid;
        }
        auto it = m_mNameToTypeID.find(name);
        if (it == m_mNameToTypeID.end()) {
            CP_StructID sid = m_structs.insert(ls);
            lt.m_flags = (ls.m_struct_or_union ? TF_STRUCT : TF_UNION);
            lt.m_id = sid;
            lt.m_size = GetSizeofUnion(sid);
            lt.location() = location;
            CP_TypeID newtid = m_types.AddUnique(lt);
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
            return newtid;
        } else {
            CP_TypeID tid = it->second;
            while (m_types[tid].m_flags & TF_ALIAS)
                tid = m_types[tid].m_id;
            assert(m_types[tid].m_flags & (TF_STRUCT | TF_UNION));
            CP_StructID sid = m_types[tid].m_id;
            if (ls.m_type_list.size())
                m_structs[sid] = ls;
            return tid;
        }
    }

    CP_TypeID AddEnumType(const CP_String& name, const CP_LogEnum& le,
                          const CP_Location& location)
    {
        CP_LogType lt;
        if (name.empty()) {
            CP_EnumID eid = m_enums.insert(le);
            lt.m_flags = TF_ENUM;
            lt.m_id = eid;
            lt.m_size = 4;
            lt.location() = location;
            CP_TypeID newtid = m_types.AddUnique(lt);
            m_mTypeIDToName[newtid] = name;
            return newtid;
        }
        auto it = m_mNameToTypeID.find(name);
        if (it == m_mNameToTypeID.end()) {
            CP_EnumID eid = m_enums.insert(le);
            lt.m_flags = TF_ENUM;
            lt.m_id = eid;
            lt.m_size = 4;
            lt.location() = location;
            CP_TypeID newtid = m_types.AddUnique(lt);
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
            return newtid;
        } else {
            CP_TypeID tid = it->second;
            while (m_types[tid].m_flags & TF_ALIAS)
                tid = m_types[tid].m_id;
            assert(m_types[tid].m_flags & TF_ENUM);
            CP_EnumID eid = m_types[tid].m_id;
            m_enums[eid] = le;
            return tid;
        }
    }

    size_t GetSizeofStruct(CP_StructID sid) const {
        assert(sid != cr_invalid_id);
        if (sid == cr_invalid_id)
            return 0;

        const CP_LogStruct& ls = m_structs[sid];
        size_t size = 0, align = 0, bitremain = 0, oldtypesize = 0;
        CP_TypeID oldtid = cr_invalid_id;
        const std::size_t count = ls.m_type_list.size();
        for (std::size_t i = 0; i < count; i++) {
            auto tid = ls.m_type_list[i];
            size_t typesize = GetSizeofType(tid);
            size_t bits = ls.m_bitfield[i];
            if (bits) {
                // bitfield
                assert(bits <= typesize * 8);
                if ((oldtid == cr_invalid_id || tid == oldtid) &&
                    bitremain >= bits)
                {
                    bitremain -= bits;
                } else if (bitremain == 0) {
                    bitremain += typesize * 8;
                    bitremain -= bits;
                } else {
                    size += oldtypesize;
                    bitremain += oldtypesize * 8;
                    bitremain -= bits;
                }
            } else {
                if (bitremain)
                    size += oldtypesize;

                size += typesize;

                // consider struct packing...
                if (align && typesize >= ls.m_pack) {
                    size += ls.m_pack - (typesize + align) % ls.m_pack;
                    align = typesize % ls.m_pack;
                } else {
                    align += typesize;
                    align %= ls.m_pack;
                }

                bitremain = 0;
            }
            oldtid = tid;
            oldtypesize = typesize;
        }
        if (bitremain)
            size += oldtypesize;
        return size;
    }

    size_t GetSizeofUnion(CP_StructID sid) const {
        assert(sid != cr_invalid_id);
        if (sid == cr_invalid_id)
            return 0;

        const CP_LogStruct& ls = m_structs[sid];
        size_t maxsize = 0, size;
        for (auto tid : ls.m_type_list) {
            size = GetSizeofType(tid);
            if (maxsize < size)
                maxsize = size;
        }
        return maxsize;
    }

    size_t GetSizeofType(CP_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return 0;
        auto& type = m_types[tid];
        return type.m_size;
    }

    CP_String StringOfEnumType(const CP_String& name, CP_EnumID eid) const {
        assert(eid != cr_invalid_id);
        if (eid == cr_invalid_id) {
            return "";
        }
        CP_String str = "enum ";
        str += name;
        str += " ";
        if (!m_enums[eid].MapValueToName().empty()) {
            str += "{ ";
            auto& e = m_enums[eid];
            for (auto it : e.MapNameToValue()) {
                str += it.first;
                str += " = ";
                char buf[32];
                std::sprintf(buf, "%d", it.second);
                str += buf;
                str += ", ";
            }
            str += "} ";
        }
        return str;
    }

    CP_String StringOfStructType(const CP_String& name, CP_StructID sid) const {
        assert(sid != cr_invalid_id);
        if (sid == cr_invalid_id) {
            return "";
        }
        CP_String str;
        const auto& s = m_structs[sid];
        if (s.m_struct_or_union) {
            str += "struct ";
        } else {
            str += "union ";
        }
        if (name.size()) {
            str += name;
            str += " ";
        }
        if (s.m_type_list.size()) {
            str += "{ ";
            const std::size_t siz = s.m_type_list.size();
            for (std::size_t i = 0; i < siz; i++) {
                str += StringOfType(s.m_type_list[i], s.m_name_list[i], false);
                if (s.m_bitfield[i]) {
                    char buf[64];
                    std::sprintf(buf, " : %u", static_cast<int>(s.m_bitfield[i]));
                    str += buf;
                }
                str += "; ";
            }
            str += "} ";
        }
        return str;
    }

    CP_String StringOfType(CP_TypeID tid, const CP_String& name,
                           bool expand = true, bool no_convension = false) const
    {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id) {
            return "";
        }
        assert(tid < m_types.size());
        const auto& type = m_types[tid];
        auto type_name = NameFromTypeID(tid);
        if (type.m_flags & TF_ALIAS) {
            if (expand || type_name.empty()) {
                return StringOfType(type.m_id, name, false);
            } else {
                return type_name + " " + name;
            }
        }
        if (type.m_flags & (TF_STRUCT | TF_UNION)) {
            if (expand || type_name.empty()) {
                return StringOfStructType(type_name, type.m_id) + name;
            } else {
                if (type.m_flags & TF_STRUCT) {
                    return "struct " + type_name + " " + name;
                } else {
                    return "union " + type_name + " " + name;
                }
            }
        }
        if (type.m_flags & TF_ENUM) {
            if (expand || type_name.empty()) {
                return StringOfEnumType(type_name, type.m_id) + name;
            } else {
                return "enum " + type_name + " " + name;
            }
        }
        if (type.m_flags & TF_ENUMITEM) {
            return "enumitem " + type_name + " = " + name;
        }
        if (type.m_flags & TF_ARRAY) {
            if (type.m_count) {
                char buf[64];
                std::sprintf(buf, "[%d]", static_cast<int>(type.m_count));
                return StringOfType(type.m_id, name + buf, false);
            } else {
                return StringOfType(type.m_id, name + "[]", false);
            }
        }
        if (type.m_flags & TF_FUNCTION) {
            assert(type.m_id < m_funcs.size());
            const auto& func = m_funcs[type.m_id];
            auto rettype = StringOfType(func.m_return_type, "", false);
            auto paramlist = StringOfParamList(func.m_type_list, func.m_name_list);
            CP_String convension;
            if (!no_convension) {
                if (type.m_flags & TF_STDCALL)
                    convension = "__stdcall ";
                else if (type.m_flags & TF_FASTCALL)
                    convension = "__fastcall ";
                else if (type.m_flags & TF_CDECL)
                    convension = "__cdecl ";
            }
            if (func.m_ellipsis)
                paramlist += ", ...";
            return rettype + convension + name + "(" + paramlist + ")";
        }
        if (type.m_flags & TF_POINTER) {
            auto tid2 = type.m_id;
            assert(tid2 < m_types.size());
            const auto& type2 = m_types[tid2];
            if (type2.m_flags & TF_FUNCTION) {
                if (type.m_flags & TF_CONST) {
                    if (type.m_flags & TF_STDCALL)
                        return StringOfType(tid2, "(__stdcall * const " + name + ")", false, true);
                    else if (type.m_flags & TF_FASTCALL)
                        return StringOfType(tid2, "(__fastcall * const " + name + ")", false, true);
                    else
                        return StringOfType(tid2, "(__cdecl * const " + name + ")", false, true);
                } else {
                    if (type.m_flags & TF_STDCALL)
                        return StringOfType(tid2, "(__stdcall *" + name + ")", false, true);
                    else if (type.m_flags & TF_FASTCALL)
                        return StringOfType(tid2, "(__fastcall *" + name + ")", false, true);
                    else
                        return StringOfType(tid2, "(__cdecl *" + name + ")", false, true);
                }
            } else if (type2.m_flags & TF_POINTER) {
                if (type.m_flags & TF_CONST) {
                    return StringOfType(tid2, "(* const " + name + ")", false);
                } else {
                    return StringOfType(tid2, "(*" + name + ")", false);
                }
            } else if (type2.m_flags & TF_ARRAY) {
                if (type.m_flags & TF_CONST) {
                    if (type2.m_count) {
                        char buf[64];
                        std::sprintf(buf, "[%d]", static_cast<int>(type2.m_count));
                        return StringOfType(tid2, "(* const " + name + buf + ")", false);
                    } else {
                        return StringOfType(tid2, "(* const " + name + "[])", false);
                    }
                } else {
                    if (type2.m_count) {
                        char buf[64];
                        std::sprintf(buf, "[%d]", static_cast<int>(type2.m_count));
                        return StringOfType(tid2, "(*" + name + buf + ")", false);
                    } else {
                        return StringOfType(tid2, "(*" + name + "[])", false);
                    }
                }
            } else {
                if (type.m_flags & TF_CONST) {
                    return StringOfType(tid2, "", false) + "* const " + name;
                } else {
                    return StringOfType(tid2, "", false) + "*" + name;
                }
            }
        }
        if (type.m_flags & TF_CONST) {
            return "const " + StringOfType(type.m_id, name, false);
        }
        if (type_name.size()) {
            return type_name + " " + name;
        }
        return "";
    }

    CP_String StringOfParamList(
        const CP_TypeSet& type_list,
        const CP_StringSet& name_list) const
    {
        assert(type_list.size() == name_list.size());
        std::size_t i, size = type_list.size();
        CP_String str;
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
    }

    int GetIntValueFromVarName(const CP_String& name) const {
        auto it = m_mNameToVarID.find(name);
        if (it == m_mNameToVarID.end())
            return 0;
        return m_vars[it->second].m_int_value;
    }

    bool IsFuncType(CP_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return false;
        tid = ResolveAlias(tid);
        if (m_types[tid].m_flags & TF_FUNCTION)
            return true;
        return false;
    }

    bool IsExtendedType(CP_TypeID tid) const {
        const auto& type = LogType(tid);
        if (type.m_flags & (TF_POINTER | TF_ARRAY | TF_CONST)) {
            return IsExtendedType(type.m_id);
        }
        const CP_TypeFlags flags = (TF_XSIGNED | TF_ENUMITEM);
        if (type.m_flags & flags) {
            return true;
        }
        return false;
    }

    bool IsPredefinedType(CP_TypeID tid) const {
        const auto& type = LogType(tid);
        if (type.m_flags & (TF_POINTER | TF_ARRAY | TF_CONST)) {
            return IsPredefinedType(type.m_id);
        }
        const CP_TypeFlags flags =
            (TF_ALIAS | TF_FUNCTION | TF_STRUCT | TF_ENUM |
             TF_UNION | TF_ENUMITEM);
        if (type.m_flags & flags) {
            return false;
        }
        return true;
    }

    bool IsIntegerType(CP_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return false;
        tid = ResolveAlias(tid);
        const CP_TypeFlags not_flags =
            (TF_DOUBLE | TF_FLOAT | TF_POINTER | TF_ARRAY | TF_FUNCTION |
             TF_VA_LIST | TF_STRUCT | TF_UNION | TF_ENUM);
        if (m_types[tid].m_flags & not_flags)
            return false;
        const CP_TypeFlags flags =
            (TF_INT | TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG | TF_ENUMITEM);
        if (m_types[tid].m_flags & flags)
            return true;
        if (m_types[tid].m_flags & TF_CONST)
            return IsIntegerType(m_types[tid].m_id);
        return false;
    }

    bool IsFloatingType(CP_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return false;
        tid = ResolveAlias(tid);
        if (m_types[tid].m_flags & (TF_DOUBLE | TF_FLOAT))
            return true;
        if (m_types[tid].m_flags & TF_CONST)
            return IsFloatingType(m_types[tid].m_id);
        return false;
    }

    bool IsUnsignedType(CP_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return false;
        tid = ResolveAlias(tid);
        if (m_types[tid].m_flags & TF_UNSIGNED)
            return true;
        if (m_types[tid].m_flags & TF_CONST)
            return IsUnsignedType(m_types[tid].m_id);
        return false;
    }

    CP_TypeID ResolveAlias(CP_TypeID tid) const {
        if (tid == cr_invalid_id)
            return tid;
        return ResolveAlias_(tid);
    }

    CP_TypeID ResolveAlias_(CP_TypeID tid) const {
        assert(tid != cr_invalid_id);
        while (m_types[tid].m_flags & TF_ALIAS)
            tid = m_types[tid].m_id;
        return tid;
    }

    void AddTypeFlags(CP_TypeID tid, CP_TypeFlags flags) {
        m_types[tid].m_flags |= flags;
    }

    CP_LogType& LogType(CP_TypeID tid) {
        return m_types[tid];
    }

    const CP_LogType& LogType(CP_TypeID tid) const {
        return m_types[tid];
    }

    CP_LogVar& LogVar(CP_VarID vid) {
        return m_vars[vid];
    }

    const CP_LogVar& LogVar(CP_VarID vid) const {
        return m_vars[vid];
    }

    CP_Map<CP_String, CP_TypeID>& MapNameToTypeID() {
        return m_mNameToTypeID;
    }

    const CP_Map<CP_String, CP_TypeID>& MapNameToTypeID() const {
        return m_mNameToTypeID;
    }

    CP_Map<CP_TypeID, CP_String>& MapTypeIDToName() {
        return m_mTypeIDToName;
    }

    const CP_Map<CP_TypeID, CP_String>& MapTypeIDToName() const {
        return m_mTypeIDToName;
    }

    CP_Map<CP_String, CP_VarID>& MapNameToVarID() {
        return m_mNameToVarID;
    }

    const CP_Map<CP_String, CP_VarID>& MapNameToVarID() const {
        return m_mNameToVarID;
    }

    CP_Map<CP_VarID, CP_String>& MapVarIDToName() {
        return m_mVarIDToName;
    }

    const CP_Map<CP_VarID, CP_String>& MapVarIDToName() const {
        return m_mVarIDToName;
    }

    CP_DeqSet<CP_LogVar>& Vars() {
        return m_vars;
    }

    const CP_DeqSet<CP_LogVar>& Vars() const {
        return m_vars;
    }

    CP_LogStruct& LogStruct(CP_StructID sid) {
        assert(sid < m_structs.size());
        return m_structs[sid];
    }

    const CP_LogStruct& LogStruct(CP_StructID sid) const {
        assert(sid < m_structs.size());
        return m_structs[sid];
    }

    CP_LogFunc& LogFunc(CP_FuncID fid) {
        assert(fid < m_funcs.size());
        return m_funcs[fid];
    }

    const CP_LogFunc& LogFunc(CP_FuncID fid) const {
        assert(fid < m_funcs.size());
        return m_funcs[fid];
    }

    CP_LogEnum& LogEnum(CP_EnumID eid) {
        assert(eid < m_enums.size());
        return m_enums[eid];
    }

    const CP_LogEnum& LogEnum(CP_EnumID eid) const {
        assert(eid < m_enums.size());
        return m_enums[eid];
    }

    CP_DeqSet<CP_LogType>& LogTypes() {
        return m_types;
    }

    const CP_DeqSet<CP_LogType>& LogTypes() const {
        return m_types;
    }

protected:
    bool                            m_is_64bit;
    CP_Map<CP_String, CP_TypeID>    m_mNameToTypeID;
    CP_Map<CP_TypeID, CP_String>    m_mTypeIDToName;
    CP_Map<CP_String, CP_VarID>     m_mNameToVarID;
    CP_Map<CP_VarID, CP_String>     m_mVarIDToName;
    CP_Map<CP_String, CP_TypeID>    m_mNameToFuncTypeID;
    CP_DeqSet<CP_LogType>           m_types;
    CP_DeqSet<CP_LogFunc>           m_funcs;
    CP_DeqSet<CP_LogStruct>         m_structs;
    CP_DeqSet<CP_LogEnum>           m_enums;
    CP_DeqSet<CP_LogVar>            m_vars;
}; // class CP_NameScope

#endif  // ndef TYPESYSTEM_H_
