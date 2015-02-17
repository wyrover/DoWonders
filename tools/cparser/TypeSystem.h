////////////////////////////////////////////////////////////////////////////
// TypeSystem.h
// Copyright (C) 2014 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse.
////////////////////////////////////////////////////////////////////////////

#ifndef TYPESYSTEM_H_
#define TYPESYSTEM_H_

////////////////////////////////////////////////////////////////////////////
// CR_TypeFlags

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
typedef unsigned long CR_TypeFlags;

////////////////////////////////////////////////////////////////////////////
// CrNormalizeTypeFlags

inline CR_TypeFlags CrNormalizeTypeFlags(CR_TypeFlags flags) {
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
} // CrNormalizeTypeFlags

////////////////////////////////////////////////////////////////////////////
// IDs

// CR_ID --- ID
typedef std::size_t CR_ID;

// CR_TypeID --- type ID
typedef CR_ID CR_TypeID;

// CR_FuncID --- function ID
typedef CR_ID CR_FuncID;

// CR_VarID --- variable ID
typedef CR_ID CR_VarID;

// CR_StructID --- struct or union ID
typedef CR_ID CR_StructID;

// CR_EnumID --- enum ID
typedef CR_ID CR_EnumID;

// cr_invalid_id --- invalid ID
#define cr_invalid_id   static_cast<CR_ID>(-1)

////////////////////////////////////////////////////////////////////////////
// CR_TypeSet

typedef CR_DeqSet<CR_TypeID> CR_TypeSet;
typedef CR_DeqSet<CR_ID> CR_IDSet;

////////////////////////////////////////////////////////////////////////////
// CR_LogFunc

struct CR_LogFunc {
    bool                    m_ellipsis;
    CR_TypeSet              m_type_list;
    CR_StringSet            m_name_list;
    CR_TypeID               m_return_type;
    enum {
        FT_CDECL, FT_STDCALL, FT_FASTCALL
    } m_func_type;

    CR_LogFunc() :
        m_ellipsis(false), m_return_type(0), m_func_type(FT_CDECL) { }

    CR_LogFunc(const CR_LogFunc& lf) :
        m_ellipsis(lf.m_ellipsis),
        m_type_list(lf.m_type_list),
        m_name_list(lf.m_name_list),
        m_return_type(lf.m_return_type),
        m_func_type(lf.m_func_type) { }

    CR_LogFunc& operator=(const CR_LogFunc& lf) {
        m_ellipsis = lf.m_ellipsis;
        m_type_list = lf.m_type_list;
        m_name_list = lf.m_name_list;
        m_return_type = lf.m_return_type;
        m_func_type = lf.m_func_type;
        return *this;
    }
}; // struct CR_LogFunc

////////////////////////////////////////////////////////////////////////////
// CR_LogType

struct CR_LogType {
    CR_TypeFlags m_flags;

    // For TF_POINTER:              the type ID (CR_TypeID)
    // For TF_ARRAY:                the type ID (CR_TypeID)
    // For TF_CONST:                the type ID (CR_TypeID)
    // For TF_CONST | TF_POINTER:   the type ID (CR_TypeID)
    // For TF_FUNCTION:             the function ID (CR_FuncID)
    // For TF_STRUCT:               the struct ID (CR_StructID)
    // For TF_ENUM:                 the enum ID (CR_EnumID)
    // For TF_UNION:                the struct ID (CR_StructID)
    // For TF_ENUMITEM:             the enum ID (CR_EnumID)
    // otherwise: zero
    CR_ID        m_id;

    size_t       m_count;   // for TF_ARRAY
    size_t       m_size;
    CR_Location  m_loc;

    CR_LogType() : m_flags(0), m_id(0), m_count(0), m_size(0) { }

    CR_LogType(CR_TypeFlags flags, size_t size, const CR_Location& location) :
        m_flags(flags), m_id(0), m_count(0), m_size(size), m_loc(location) { }

    CR_LogType(const CR_LogType& type) :
        m_flags(type.m_flags), m_id(type.m_id),
        m_count(type.m_count), m_size(type.m_size), m_loc(type.m_loc) { }

    CR_LogType& operator=(const CR_LogType& type) {
        m_flags = type.m_flags;
        m_id = type.m_id;
        m_count = type.m_count;
        m_size = type.m_size;
        m_loc = type.m_loc;
        return *this;
    }

    bool operator==(const CR_LogType& type) const {
        return m_flags == type.m_flags &&
               m_id == type.m_id &&
               m_count == type.m_count;
    }

    bool operator!=(const CR_LogType& type) const {
        return m_flags != type.m_flags ||
               m_id != type.m_id ||
               m_count != type.m_count;
    }

          CR_Location& location()       { return m_loc; }
    const CR_Location& location() const { return m_loc; }
}; // struct CR_LogType

////////////////////////////////////////////////////////////////////////////
// CR_LogStruct -- logical structure or union

struct CR_LogStruct {
    bool                    m_struct_or_union;
    CR_TypeSet              m_type_list;
    CR_StringSet            m_name_list;
    CR_DeqSet<size_t>       m_offset_list;
    CR_DeqSet<size_t>       m_bitfield;
    size_t                  m_pack;

    CR_LogStruct(bool struct_or_union = true) :
        m_struct_or_union(struct_or_union), m_pack(1) { }

    CR_LogStruct(const CR_LogStruct& ls) :
        m_struct_or_union(ls.m_struct_or_union),
        m_type_list(ls.m_type_list),
        m_name_list(ls.m_name_list),
        m_offset_list(ls.m_offset_list),
        m_bitfield(ls.m_bitfield),
        m_pack(ls.m_pack) { }

    CR_LogStruct& operator=(const CR_LogStruct& ls) {
        m_struct_or_union = ls.m_struct_or_union;
        m_type_list = ls.m_type_list;
        m_name_list = ls.m_name_list;
        m_offset_list = ls.m_offset_list;
        m_bitfield = ls.m_bitfield;
        m_pack = ls.m_pack;
        return *this;
    }

    size_t FindName(const CR_String& name) const {
        for (size_t i = 0; i < m_name_list.size(); i++) {
            if (m_name_list[i] == name)
                return i;
        }
        return cr_invalid_id;
    }

    bool operator==(const CR_LogStruct& ls) const {
        return m_struct_or_union == ls.m_struct_or_union &&
               m_type_list == ls.m_type_list &&
               m_name_list == ls.m_name_list &&
               m_offset_list == ls.m_offset_list &&
               m_bitfield == ls.m_bitfield &&
               m_pack == ls.m_pack;
    }

    bool operator!=(const CR_LogStruct& ls) const {
        return m_struct_or_union != ls.m_struct_or_union ||
               m_type_list != ls.m_type_list ||
               m_name_list != ls.m_name_list ||
               m_offset_list != ls.m_offset_list ||
               m_bitfield != ls.m_bitfield ||
               m_pack != ls.m_pack;
    }
}; // struct CR_LogStruct

////////////////////////////////////////////////////////////////////////////
// CR_LogEnum

struct CR_LogEnum {
    CR_LogEnum() { }

    CR_LogEnum(const CR_LogEnum& le) :
        m_mNameToValue(le.m_mNameToValue),
        m_mValueToName(le.m_mValueToName) { }

    CR_LogEnum& operator=(const CR_LogEnum& le) {
        m_mNameToValue = le.m_mNameToValue;
        m_mValueToName = le.m_mValueToName;
        return *this;
    }

    CR_UnorderedMap<CR_String, int>& MapNameToValue()
    { return m_mNameToValue; }

    CR_UnorderedMap<int, CR_String>& MapValueToName()
    { return m_mValueToName; }

    const CR_UnorderedMap<CR_String, int>& MapNameToValue() const
    { return m_mNameToValue; }

    const CR_UnorderedMap<int, CR_String>& MapValueToName() const
    { return m_mValueToName; }

protected:
    CR_UnorderedMap<CR_String, int>     m_mNameToValue;
    CR_UnorderedMap<int, CR_String>     m_mValueToName;
}; // struct CR_LogEnum

////////////////////////////////////////////////////////////////////////////
// CR_LogVar

struct CR_LogVar {
    CR_LogVar() : m_has_value(false) { }

    bool            m_has_value;
    CR_TypeID       m_type_id;
    CR_Location     m_loc;
    CR_ID           m_enum_type_id;
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

          CR_Location& location()       { return m_loc; }
    const CR_Location& location() const { return m_loc; }
}; // struct CR_LogVar

////////////////////////////////////////////////////////////////////////////
// CR_NameScope

class CR_NameScope {
public:
    CR_NameScope(bool is_64bit) : m_is_64bit(is_64bit) {
        Init();
    }

    CR_NameScope(const CR_NameScope& ns) :
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

    CR_NameScope& operator=(const CR_NameScope& ns) {
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
        #ifdef __GNUC__
            if (m_is_64bit)
                AddType("long double", TF_LONG | TF_DOUBLE, 16);
            else
                AddType("long double", TF_LONG | TF_DOUBLE, 12);
        #else
            AddType("long double", TF_LONG | TF_DOUBLE, 8);
        #endif

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

    CR_TypeID TypeIDFromFlags(CR_TypeFlags flags) const {
        const size_t siz = m_types.size();
        for (size_t i = 0; i < siz; ++i) {
            if (m_types[i].m_flags == flags)
                return i;
        }
        return cr_invalid_id;
    }

    size_t SizeFromFlags(CR_TypeFlags flags) const {
        flags = CrNormalizeTypeFlags(flags & ~TF_CONST);
        for (const auto& t : m_types) {
            if (t.m_flags == flags)
                return t.m_size;
        }
        return 0;
    }

    CR_TypeID TypeIDFromName(const CR_String& name) const {
        auto it = m_mNameToTypeID.find(name);
        if (it != m_mNameToTypeID.end())
            return it->second;
        else
            return cr_invalid_id;
    }

    CR_String NameFromTypeID(CR_TypeID tid) const {
        auto it = m_mTypeIDToName.find(tid);
        if (it != m_mTypeIDToName.end())
            return it->second;
        else
            return "";
    }

    CR_TypeID AddType(const CR_String& name, const CR_LogType& lt) {
        auto tid = m_types.AddUnique(lt);
        if (!name.empty()) {
            m_mNameToTypeID[name] = tid;
            m_mTypeIDToName[tid] = name;
        }
        return tid;
    }

    CR_TypeID AddType(const CR_String& name, CR_TypeFlags flags, size_t size,
                      const CR_Location& location = CR_Location())
    {
        return AddType(name, CR_LogType(flags, size, location));
    }

    CR_TypeID AddAliasType(const CR_String& name, CR_TypeID tid,
                           const CR_Location& location)
    {
        assert(!name.empty());
        CR_LogType lt;
        lt.m_flags = TF_ALIAS;
        lt.m_id = tid;
        lt.m_size = GetSizeofType(tid);
        lt.location() = location;
        tid = m_types.insert(lt);
        m_mNameToTypeID[name] = tid;
        m_mTypeIDToName[tid] = name;
        return tid;
    }

    CR_VarID AddVar(const CR_String& name, CR_TypeID tid,
                    const CR_Location& location)
    {
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

    CR_VarID AddVar(const CR_String& name, const CR_LogType& lt) {
        auto tid = m_types.AddUnique(lt);
        if (!name.empty()) {
            m_mNameToTypeID[name] = tid;
            m_mTypeIDToName[tid] = name;
        }
        return AddVar(name, tid, lt.location());
    }

    CR_TypeID AddConstType(CR_TypeID tid) {
        assert(tid != cr_invalid_id);
        CR_LogType lt;
        lt.m_flags = TF_CONST;
        lt.m_id = tid;
        lt.m_size = GetSizeofType(tid);
        auto newtid = m_types.AddUnique(lt);
        auto name = NameFromTypeID(tid);
        if (!name.empty()) {
            name = CR_String("const ") + name;
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
        }
        return newtid;
    }

    CR_TypeID AddPtrType(CR_TypeID tid, CR_TypeFlags flags,
                         const CR_Location& location)
    {
        assert(tid != cr_invalid_id);
        CR_LogType lt;
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

    CR_TypeID AddArrayType(CR_TypeID tid, int count,
                           const CR_Location& location)
    {
        assert(tid != cr_invalid_id);
        CR_LogType lt;
        lt.m_flags = TF_ARRAY;
        lt.m_id = tid;
        lt.m_count = count;
        lt.m_size = GetSizeofType(tid) * count;
        lt.location() = location;
        tid = m_types.AddUnique(lt);
        m_mTypeIDToName[tid] = "";
        return tid;
    }

    CR_TypeID AddFuncType(const CR_LogFunc& lf, const CR_Location& location) {
        CR_LogFunc func(lf);
        if (func.m_type_list.size() == 1 && func.m_type_list[0] == 0) {
            // parameter list is void
            func.m_type_list.clear();
            func.m_name_list.clear();
        }
        auto fid = m_funcs.insert(func);
        CR_LogType lt;
        lt.m_flags = TF_FUNCTION;
        lt.m_id = fid;
        lt.m_size = (Is64Bit() ? 8 : 4);
        lt.location() = location;
        CR_TypeID tid = m_types.AddUnique(lt);
        m_mTypeIDToName[tid] = "";
        return tid;
    }

    CR_TypeID AddStructType(const CR_String& name, const CR_LogStruct& ls,
                            const CR_Location& location)
    {
        CR_LogType lt;
        if (name.empty()) {
            CR_StructID sid = m_structs.insert(ls);
            lt.m_flags = (ls.m_struct_or_union ? TF_STRUCT : TF_UNION);
            lt.m_id = sid;
            lt.m_size = _CalculateStruct(sid);
            lt.location() = location;
            CR_TypeID newtid = m_types.AddUnique(lt);
            m_mTypeIDToName[newtid] = name;
            return newtid;
        }
        auto it = m_mNameToTypeID.find(name);
        if (it == m_mNameToTypeID.end()) {
            CR_StructID sid = m_structs.insert(ls);
            lt.m_flags = (ls.m_struct_or_union ? TF_STRUCT : TF_UNION);
            lt.m_id = sid;
            lt.m_size = _CalculateStruct(sid);
            lt.location() = location;
            CR_TypeID newtid = m_types.AddUnique(lt);
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
            return newtid;
        } else {
            CR_TypeID tid = ResolveAlias(it->second);
            assert(m_types[tid].m_flags & (TF_STRUCT | TF_UNION));
            CR_StructID sid = m_types[tid].m_id;
            _CalculateStruct(sid);
            if (ls.m_type_list.size())
                m_structs[sid] = ls;
            return tid;
        }
    }

    CR_TypeID AddUnionType(const CR_String& name, const CR_LogStruct& ls,
                           const CR_Location& location)
    {
        CR_LogType lt;
        if (name.empty()) {
            CR_StructID sid = m_structs.insert(ls);
            lt.m_flags = (ls.m_struct_or_union ? TF_STRUCT : TF_UNION);
            lt.m_id = sid;
            lt.m_size = _CalculateUnion(sid);
            lt.location() = location;
            CR_TypeID newtid = m_types.AddUnique(lt);
            m_mTypeIDToName[newtid] = name;
            return newtid;
        }
        auto it = m_mNameToTypeID.find(name);
        if (it == m_mNameToTypeID.end()) {
            CR_StructID sid = m_structs.insert(ls);
            lt.m_flags = (ls.m_struct_or_union ? TF_STRUCT : TF_UNION);
            lt.m_id = sid;
            lt.m_size = _CalculateUnion(sid);
            lt.location() = location;
            CR_TypeID newtid = m_types.AddUnique(lt);
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
            return newtid;
        } else {
            CR_TypeID tid = ResolveAlias(it->second);
            assert(m_types[tid].m_flags & (TF_STRUCT | TF_UNION));
            CR_StructID sid = m_types[tid].m_id;
            _CalculateUnion(sid);
            if (ls.m_type_list.size())
                m_structs[sid] = ls;
            return tid;
        }
    }

    CR_TypeID AddEnumType(const CR_String& name, const CR_LogEnum& le,
                          const CR_Location& location)
    {
        CR_LogType lt;
        if (name.empty()) {
            CR_EnumID eid = m_enums.insert(le);
            lt.m_flags = TF_ENUM;
            lt.m_id = eid;
            lt.m_size = 4;
            lt.location() = location;
            CR_TypeID newtid = m_types.AddUnique(lt);
            m_mTypeIDToName[newtid] = name;
            return newtid;
        }
        auto it = m_mNameToTypeID.find(name);
        if (it == m_mNameToTypeID.end()) {
            CR_EnumID eid = m_enums.insert(le);
            lt.m_flags = TF_ENUM;
            lt.m_id = eid;
            lt.m_size = 4;
            lt.location() = location;
            CR_TypeID newtid = m_types.AddUnique(lt);
            m_mNameToTypeID[name] = newtid;
            m_mTypeIDToName[newtid] = name;
            return newtid;
        } else {
            CR_TypeID tid = ResolveAlias(it->second);
            assert(m_types[tid].m_flags & TF_ENUM);
            CR_EnumID eid = m_types[tid].m_id;
            m_enums[eid] = le;
            return tid;
        }
    }

    size_t _CalculateStruct(CR_StructID sid) {
        assert(sid != cr_invalid_id);
        if (sid == cr_invalid_id)
            return 0;

        CR_LogStruct& ls = m_structs[sid];
        ls.m_offset_list.clear();

        size_t size = 0, align = 0, bitremain = 0, oldtypesize = 0;
        CR_TypeID oldtid = cr_invalid_id;
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
                    ls.m_offset_list.push_back(size);
                    bitremain -= bits;
                } else if (bitremain == 0) {
                    ls.m_offset_list.push_back(size);
                    bitremain += typesize * 8;
                    bitremain -= bits;
                } else {
                    size += oldtypesize;
                    ls.m_offset_list.push_back(size);
                    bitremain += oldtypesize * 8;
                    bitremain -= bits;
                }
            } else {
                if (bitremain)
                    size += oldtypesize;

                ls.m_offset_list.push_back(size);

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
        assert(ls.m_offset_list.size() == ls.m_type_list.size());
        return size;
    } // _CalculateStruct

    size_t _CalculateUnion(CR_StructID sid) {
        assert(sid != cr_invalid_id);
        if (sid == cr_invalid_id)
            return 0;

        CR_LogStruct& ls = m_structs[sid];
        ls.m_offset_list.clear();

        size_t maxsize = 0, size;
        for (auto tid : ls.m_type_list) {
            size = GetSizeofType(tid);
            if (maxsize < size)
                maxsize = size;

            ls.m_offset_list.push_back(0);
        }
        assert(ls.m_offset_list.size() == ls.m_type_list.size());
        return maxsize;
    }

    size_t GetSizeofType(CR_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return 0;
        auto& type = m_types[tid];
        return type.m_size;
    }

    CR_String StringOfEnumType(const CR_String& name, CR_EnumID eid) const {
        assert(eid != cr_invalid_id);
        if (eid == cr_invalid_id) {
            return "";
        }
        CR_String str = "enum ";
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

    CR_String StringOfStructType(const CR_String& name, CR_StructID sid) const {
        assert(sid != cr_invalid_id);
        if (sid == cr_invalid_id) {
            return "";
        }
        CR_String str;
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

    CR_String StringOfType(CR_TypeID tid, const CR_String& name,
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
            CR_String convension;
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

    CR_String StringOfParamList(
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
    }

    int GetIntValueFromVarName(const CR_String& name) const {
        auto it = m_mNameToVarID.find(name);
        if (it == m_mNameToVarID.end())
            return 0;
        return m_vars[it->second].m_int_value;
    }

    bool IsFuncType(CR_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return false;
        tid = ResolveAlias(tid);
        if (m_types[tid].m_flags & TF_FUNCTION)
            return true;
        return false;
    }

    bool IsExtendedType(CR_TypeID tid) const {
        const auto& type = LogType(tid);
        if (type.m_flags & (TF_POINTER | TF_ARRAY | TF_CONST)) {
            return IsExtendedType(type.m_id);
        }
        const CR_TypeFlags flags = (TF_XSIGNED | TF_ENUMITEM);
        if (type.m_flags & flags) {
            return true;
        }
        return false;
    }

    bool IsPredefinedType(CR_TypeID tid) const {
        const auto& type = LogType(tid);
        if (type.m_flags & (TF_POINTER | TF_ARRAY | TF_CONST)) {
            return IsPredefinedType(type.m_id);
        }
        const CR_TypeFlags flags =
            (TF_ALIAS | TF_FUNCTION | TF_STRUCT | TF_ENUM |
             TF_UNION | TF_ENUMITEM);
        if (type.m_flags & flags) {
            return false;
        }
        return true;
    }

    bool IsIntegerType(CR_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return false;
        tid = ResolveAlias(tid);
        const CR_TypeFlags not_flags =
            (TF_DOUBLE | TF_FLOAT | TF_POINTER | TF_ARRAY | TF_FUNCTION |
             TF_VA_LIST | TF_STRUCT | TF_UNION | TF_ENUM);
        if (m_types[tid].m_flags & not_flags)
            return false;
        const CR_TypeFlags flags =
            (TF_INT | TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG | TF_ENUMITEM);
        if (m_types[tid].m_flags & flags)
            return true;
        if (m_types[tid].m_flags & TF_CONST)
            return IsIntegerType(m_types[tid].m_id);
        return false;
    }

    bool IsFloatingType(CR_TypeID tid) const {
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

    bool IsUnsignedType(CR_TypeID tid) const {
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

    CR_TypeID ResolveAlias(CR_TypeID tid) const {
        if (tid == cr_invalid_id)
            return tid;
        return ResolveAliasRecurse(tid);
    }

    CR_TypeID ResolveAliasRecurse(CR_TypeID tid) const {
        assert(tid != cr_invalid_id);
        while (m_types[tid].m_flags & TF_ALIAS)
            tid = m_types[tid].m_id;
        return tid;
    }

    void AddTypeFlags(CR_TypeID tid, CR_TypeFlags flags) {
        m_types[tid].m_flags |= flags;
    }

    CR_LogType& LogType(CR_TypeID tid) {
        return m_types[tid];
    }

    const CR_LogType& LogType(CR_TypeID tid) const {
        return m_types[tid];
    }

    CR_LogVar& LogVar(CR_VarID vid) {
        return m_vars[vid];
    }

    const CR_LogVar& LogVar(CR_VarID vid) const {
        return m_vars[vid];
    }

    CR_Map<CR_String, CR_TypeID>& MapNameToTypeID() {
        return m_mNameToTypeID;
    }

    const CR_Map<CR_String, CR_TypeID>& MapNameToTypeID() const {
        return m_mNameToTypeID;
    }

    CR_Map<CR_TypeID, CR_String>& MapTypeIDToName() {
        return m_mTypeIDToName;
    }

    const CR_Map<CR_TypeID, CR_String>& MapTypeIDToName() const {
        return m_mTypeIDToName;
    }

    CR_Map<CR_String, CR_VarID>& MapNameToVarID() {
        return m_mNameToVarID;
    }

    const CR_Map<CR_String, CR_VarID>& MapNameToVarID() const {
        return m_mNameToVarID;
    }

    CR_Map<CR_VarID, CR_String>& MapVarIDToName() {
        return m_mVarIDToName;
    }

    const CR_Map<CR_VarID, CR_String>& MapVarIDToName() const {
        return m_mVarIDToName;
    }

    CR_DeqSet<CR_LogVar>& Vars() {
        return m_vars;
    }

    const CR_DeqSet<CR_LogVar>& Vars() const {
        return m_vars;
    }

    CR_LogStruct& LogStruct(CR_StructID sid) {
        assert(sid < m_structs.size());
        return m_structs[sid];
    }

    const CR_LogStruct& LogStruct(CR_StructID sid) const {
        assert(sid < m_structs.size());
        return m_structs[sid];
    }

    CR_LogFunc& LogFunc(CR_FuncID fid) {
        assert(fid < m_funcs.size());
        return m_funcs[fid];
    }

    const CR_LogFunc& LogFunc(CR_FuncID fid) const {
        assert(fid < m_funcs.size());
        return m_funcs[fid];
    }

    CR_LogEnum& LogEnum(CR_EnumID eid) {
        assert(eid < m_enums.size());
        return m_enums[eid];
    }

    const CR_LogEnum& LogEnum(CR_EnumID eid) const {
        assert(eid < m_enums.size());
        return m_enums[eid];
    }

    CR_DeqSet<CR_LogType>& LogTypes() {
        return m_types;
    }

    const CR_DeqSet<CR_LogType>& LogTypes() const {
        return m_types;
    }

protected:
    bool                            m_is_64bit;
    CR_Map<CR_String, CR_TypeID>    m_mNameToTypeID;
    CR_Map<CR_TypeID, CR_String>    m_mTypeIDToName;
    CR_Map<CR_String, CR_VarID>     m_mNameToVarID;
    CR_Map<CR_VarID, CR_String>     m_mVarIDToName;
    CR_Map<CR_String, CR_TypeID>    m_mNameToFuncTypeID;
    CR_DeqSet<CR_LogType>           m_types;
    CR_DeqSet<CR_LogFunc>           m_funcs;
    CR_DeqSet<CR_LogStruct>         m_structs;
    CR_DeqSet<CR_LogEnum>           m_enums;
    CR_DeqSet<CR_LogVar>            m_vars;
}; // class CR_NameScope

#endif  // ndef TYPESYSTEM_H_
