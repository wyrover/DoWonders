////////////////////////////////////////////////////////////////////////////
// TypeSystem.h
// Copyright (C) 2014-2015 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse. See file ReadMe.txt and License.txt.
////////////////////////////////////////////////////////////////////////////

#ifndef TYPESYSTEM_H_
#define TYPESYSTEM_H_

////////////////////////////////////////////////////////////////////////////
// CR_TypeFlags --- type flags

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
    TF_PTR64        = 0x00000400,
    TF_STRUCT       = 0x00000800,
    TF_UNION        = 0x00001000,
    TF_ENUM         = 0x00002000,
    TF_POINTER      = 0x00004000,
    TF_ARRAY        = 0x00008000,
    TF_FUNCTION     = 0x00010000,
    TF_INCOMPLETE   = 0x00020000,
    TF_CDECL        = 0,
    TF_STDCALL      = 0x00040000,
    TF_FASTCALL     = 0x00080000,
    TF_CONST        = 0x00100000,
    TF_VOLATILE     = 0x00200000,
    TF_COMPLEX      = 0x00400000,
    TF_IMAGINARY    = 0x00800000,
    TF_ATOMIC       = 0x01000000,
    TF_PTR32        = 0x02000000,
    TF_INACCURATE   = 0x04000000,   // size and/or alignment is not accurate
    TF_VECTOR       = 0x08000000,
    TF_BITFIELD     = 0x10000000,
    TF_ALIAS        = 0x20000000,
    TF_INT128       = 0x80000000
};
typedef unsigned long CR_TypeFlags;

////////////////////////////////////////////////////////////////////////////
// CrNormalizeTypeFlags

inline CR_TypeFlags CrNormalizeTypeFlags(CR_TypeFlags flags) {
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

////////////////////////////////////////////////////////////////////////////

// CR_ID --- ID
typedef std::size_t             CR_ID;

// cr_invalid_id --- invalid ID
#define cr_invalid_id           static_cast<CR_ID>(-1)

// CR_TypeID --- type ID
typedef CR_ID                   CR_TypeID;

// CR_FuncID --- function ID
typedef CR_ID                   CR_FuncID;

// CR_StructID --- struct or union ID
typedef CR_ID                   CR_StructID;

// CR_EnumID --- enum ID
typedef CR_ID                   CR_EnumID;

// CR_VarID --- variable ID
typedef CR_ID                   CR_VarID;

// a set of ID 
typedef CR_DeqSet<CR_ID>        CR_IDSet;

// a set of type id
typedef CR_DeqSet<CR_TypeID>    CR_TypeSet;

////////////////////////////////////////////////////////////////////////////
// CR_TypedValue --- typed value

struct CR_TypedValue {
    void *      m_ptr;
    size_t      m_size;
    CR_TypeID   m_type_id;

    CR_TypedValue() : m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id) { }

    CR_TypedValue(void *ptr, size_t size) :
        m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id)
    {
        m_ptr = malloc(size + 1);
        if (m_ptr) {
            memcpy(m_ptr, ptr, size);
            m_size = size;
        } else {
            throw std::bad_alloc();
        }
    }

    virtual ~CR_TypedValue() { free(m_ptr); }

    void Copy(const CR_TypedValue& value) {
        m_type_id = value.m_type_id;
        m_ptr = malloc(value.m_size + 1);
        if (m_ptr) {
            memcpy(m_ptr, value.m_ptr, value.m_size);
            m_size = value.m_size;
        } else {
            throw std::bad_alloc();
        }
    }

    CR_TypedValue(const CR_TypedValue& value) :
        m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id)
    {
        Copy(value);
    }

    CR_TypedValue& operator=(const CR_TypedValue& value) {
        if (this != &value) {
            free(m_ptr);
            Copy(value);
        }
        return *this;
    }

    CR_TypedValue(CR_TypedValue&& value) : m_ptr(NULL), m_size(0) {
        std::swap(m_ptr, value.m_ptr);
        std::swap(m_size, value.m_size);
        m_type_id = value.m_type_id;
    }

    CR_TypedValue& operator=(CR_TypedValue&& value) {
        if (this != &value) {
            std::swap(m_ptr, value.m_ptr);
            std::swap(m_size, value.m_size);
            m_type_id = value.m_type_id;
        }
        return *this;
    }


    bool empty() const { return m_size == 0 || m_ptr == NULL; }
    size_t size() const { return m_size; }

    template <typename T_VALUE>
    T_VALUE& get() {
        assert(sizeof(T_VALUE) <= m_size);
        return *reinterpret_cast<T_VALUE *>(m_ptr);
    }

    template <typename T_VALUE>
    const T_VALUE& get() const {
        assert(sizeof(T_VALUE) <= m_size);
        return *reinterpret_cast<const T_VALUE *>(m_ptr);
    }

    template <typename T_VALUE>
    void assign(const T_VALUE& v) {
        m_ptr = realloc(m_ptr, sizeof(v));
        if (m_ptr) {
            *reinterpret_cast<T_VALUE *>(m_ptr) = v;
        } else {
            throw std::bad_alloc();
        }
    }
};

////////////////////////////////////////////////////////////////////////////
// CR_LogType --- logical type

struct CR_LogType {
    CR_TypeFlags m_flags;           // type flags

    CR_ID        m_sub_id;          // sub ID.
    // m_sub_id means...
    // For TF_ALIAS:                A type ID (CR_TypeID).
    // For TF_ALIAS | TF_FUNCTION:  A type ID (CR_TypeID).
    // For TF_POINTER:              A type ID (CR_TypeID).
    // For TF_ARRAY:                A type ID (CR_TypeID).
    // For TF_CONST:                A type ID (CR_TypeID).
    // For TF_CONST | TF_POINTER:   A type ID (CR_TypeID).
    // For TF_VECTOR:               A type ID (CR_TypeID).
    // For TF_FUNCTION:             A function ID (CR_FuncID).
    // For TF_STRUCT:               A struct ID (CR_StructID).
    // For TF_ENUM:                 An enum ID (CR_EnumID).
    // For TF_UNION:                A struct ID (CR_StructID).
    // otherwise: zero

    size_t          m_count;        // for TF_ARRAY, TF_STRUCT, TF_UNION
                                    //     TF_VECTOR

    int             m_size;         // the size of type
    int             m_align;        // alignment requirement of type
    int             m_alignas;      // # of alignas(#) if specified
    bool            m_alignas_explicit;
    CR_Location     m_location;          // the location

    CR_LogType() : m_flags(0), m_sub_id(0), m_count(0), m_size(0), m_align(0),
                   m_alignas(0), m_alignas_explicit(false) { }

    CR_LogType(CR_TypeFlags flags, int size, const CR_Location& location) :
        m_flags(flags), m_sub_id(0), m_count(0), m_size(size), m_align(size),
        m_alignas(0), m_alignas_explicit(false), m_location(location) { }

    CR_LogType(CR_TypeFlags flags, int size, int align,
               const CR_Location& location) :
        m_flags(flags), m_sub_id(0), m_count(0), m_size(size), m_align(align),
            m_alignas(0), m_alignas_explicit(false), m_location(location) { }

    CR_LogType(CR_TypeFlags flags, int size, int align, int alignas_,
               const CR_Location& location) :
        m_flags(flags), m_sub_id(0), m_count(0), m_size(size), m_align(align),
            m_alignas(alignas_), m_alignas_explicit(false),
                m_location(location) { }

    // incomplete comparison 
    bool operator==(const CR_LogType& type) const;
    bool operator!=(const CR_LogType& type) const;

          CR_Location& location()       { return m_location; }
    const CR_Location& location() const { return m_location; }
}; // struct CR_LogType

////////////////////////////////////////////////////////////////////////////
// CR_LogFunc --- logical function

struct CR_LogFunc {
    bool                    m_ellipsis;
    CR_TypeSet              m_type_list;
    CR_StringSet            m_name_list;
    CR_TypeID               m_return_type;
    enum Type {
        FT_CDECL, FT_STDCALL, FT_FASTCALL
    };
    Type m_func_type;  // calling convention

    CR_LogFunc() :
        m_ellipsis(false), m_return_type(0), m_func_type(FT_CDECL) { }
}; // struct CR_LogFunc

////////////////////////////////////////////////////////////////////////////
// CR_LogStruct --- logical structure or union

struct CR_LogStruct {
    CR_TypeID               m_tid;              // type ID
    bool                    m_is_struct;        // it's not union if true
    CR_TypeSet              m_type_list;        // list of type IDs
    CR_StringSet            m_name_list;        // list of names
    CR_DeqSet<int>          m_bit_offset_list;  // list of offset
    CR_DeqSet<int>          m_bits_list;        // list of bits
    int                     m_pack;             // pack
    int                     m_align;            // alignment requirement
    int                     m_alignas;          // _Alignas(#)
    bool                    m_alignas_explicit;
    bool                    m_is_complete;      // is it complete?

    CR_LogStruct(bool is_struct = true) :
        m_is_struct(is_struct), m_pack(8), m_align(0),
        m_alignas_explicit(false), m_is_complete(false) { }

    size_t FindName(const std::string& name) const {
        for (size_t i = 0; i < m_name_list.size(); i++) {
            if (m_name_list[i] == name)
                return i;
        }
        return cr_invalid_id;
    }

    // incomplete comparison
    bool operator==(const CR_LogStruct& ls) const;
    bool operator!=(const CR_LogStruct& ls) const;

    bool empty() const { return m_type_list.empty(); }
}; // struct CR_LogStruct

////////////////////////////////////////////////////////////////////////////
// CR_LogEnum

struct CR_LogEnum {
    std::unordered_map<std::string, int>     m_mNameToValue;
    std::unordered_map<int, std::string>     m_mValueToName;

    CR_LogEnum() { }

    std::unordered_map<std::string, int>& MapNameToValue()
    { return m_mNameToValue; }

    std::unordered_map<int, std::string>& MapValueToName()
    { return m_mValueToName; }

    const std::unordered_map<std::string, int>& MapNameToValue() const
    { return m_mNameToValue; }

    const std::unordered_map<int, std::string>& MapValueToName() const
    { return m_mValueToName; }

    bool empty() const {
        return m_mNameToValue.empty() && m_mValueToName.empty();
    }
}; // struct CR_LogEnum

////////////////////////////////////////////////////////////////////////////
// CR_LogVar --- logical variable

struct CR_LogVar {
    CR_TypeID       m_type_id;          // the type ID of a variable
    CR_Location     m_location;         // the location

          CR_Location& location()       { return m_location; }
    const CR_Location& location() const { return m_location; }
}; // struct CR_LogVar

////////////////////////////////////////////////////////////////////////////
// CR_NameScope --- universe of names, types, functions and variables etc.

class CR_NameScope {
public:
    CR_NameScope(shared_ptr<CR_ErrorInfo> error_info, bool is_64bit) :
        m_error_info(error_info), m_is_64bit(is_64bit)
    {
        Init();
    }

    // is 64-bit mode or not?
    bool Is64Bit() const { return m_is_64bit; }

    // initialize
    void Init();

    bool LoadFromFiles(
        const std::string& prefix = "",
        const std::string& suffix = ".dat");
    
    bool SaveToFiles(
        const std::string& prefix = "",
        const std::string& suffix = ".dat") const;

    CR_TypeID AddType(const std::string& name, const CR_LogType& lt) {
        auto tid = m_types.AddUnique(lt);
        if (!name.empty()) {
            m_mNameToTypeID[name] = tid;
            m_mTypeIDToName[tid] = name;
        }
        return tid;
    }

    CR_TypeID AddType(const std::string& name, CR_TypeFlags flags, int size,
                      const CR_Location& location = CR_Location())
    {
        return AddType(name, CR_LogType(flags, size, location));
    }

    CR_TypeID AddType(const std::string& name, CR_TypeFlags flags, int size,
                      int align, const CR_Location& location = CR_Location())
    {
        return AddType(name, CR_LogType(flags, size, align, location));
    }

    CR_TypeID AddType(const std::string& name, CR_TypeFlags flags, int size,
                      int align, int alignas_,
                      const CR_Location& location = CR_Location())
    {
        return AddType(name, CR_LogType(flags, size, align, alignas_, location));
    }

    // add alias type
    CR_TypeID AddAliasType(const std::string& name, CR_TypeID tid,
                           const CR_Location& location);

    // add a variable
    CR_VarID AddVar(const std::string& name, CR_TypeID tid,
                    const CR_Location& location);

    // add a variable
    CR_VarID AddVar(const std::string& name, const CR_LogType& type) {
        auto tid = m_types.AddUnique(type);
        if (!name.empty()) {
            m_mNameToTypeID[name] = tid;
            m_mTypeIDToName[tid] = name;
        }
        return AddVar(name, tid, type.location());
    }

    // add a constant type
    CR_TypeID AddConstType(CR_TypeID tid);

    // add a pointer type
    CR_TypeID AddPtrType(CR_TypeID tid, CR_TypeFlags flags,
                         const CR_Location& location);

    CR_TypeID AddArrayType(CR_TypeID tid, int count,
                           const CR_Location& location);

    CR_TypeID AddVectorType(
        const std::string& name, CR_TypeID tid, int vector_size,
        const CR_Location& location);

    // add function type
    CR_TypeID AddFuncType(const CR_LogFunc& lf, const CR_Location& location);

    // add struct type
    CR_TypeID AddStructType(const std::string& name, const CR_LogStruct& ls,
                            int alignas_, const CR_Location& location);

    // add union type
    CR_TypeID AddUnionType(const std::string& name, const CR_LogStruct& ls,
                           int alignas_, const CR_Location& location);

    CR_TypeID AddEnumType(const std::string& name, const CR_LogEnum& le,
                          const CR_Location& location);

    void AddTypeFlags(CR_TypeID tid, CR_TypeFlags flags) {
        LogType(tid).m_flags |= flags;
    }

    bool CompleteStructType(CR_TypeID tid, CR_StructID sid);

    bool CompleteUnionType(CR_TypeID tid, CR_StructID sid);

    bool CompleteType(CR_TypeID tid) {
        return CompleteType(tid, LogType(tid));
    }

    bool CompleteType(CR_TypeID tid, CR_LogType& type);

    void CompleteTypeInfo();

    void SetAlignas(CR_TypeID tid, int alignas_) {
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

    //
    // getters
    //

    // get size of type
    int SizeOfType(CR_TypeID tid) const {
        assert(tid != cr_invalid_id);
        if (tid == cr_invalid_id)
            return 0;
        auto& type = LogType(tid);
        return type.m_size;
    }

    std::string
    StringOfEnumTag(const std::string& name) const {
        std::string str = "enum ";
        if (name.size()) {
            assert(name.find("enum ") == 0);
            str += name.substr(5);
            str += ' ';
        }
        return str;
    }

    // get string of enum
    std::string StringOfEnum(const std::string& name, CR_EnumID eid) const;

    std::string
    StringOfStructTag(const std::string& name, const CR_LogStruct& s) const;

    // get string of struct or union
    std::string StringOfStruct(const std::string& name, CR_StructID sid) const;

    // get string of type
    std::string StringOfType(CR_TypeID tid, const std::string& name,
                           bool expand = true, bool no_convension = false) const;

    // get string of parameter list
    std::string StringOfParamList(
        const CR_TypeSet& type_list, const CR_StringSet& name_list) const;

    //
    // type judgements
    //

    // is it function type?
    bool IsFuncType(CR_TypeID tid) const;

    // is it predefined type?
    bool IsPredefinedType(CR_TypeID tid) const;

    // is it integer type?
    bool IsIntegralType(CR_TypeID tid) const;

    // is it floating type?
    bool IsFloatingType(CR_TypeID tid) const;

    // is it unsigned type?
    bool IsUnsignedType(CR_TypeID tid) const;

    //
    // ResolveAlias
    //

    CR_TypeID ResolveAlias(CR_TypeID tid) const {
        if (tid == cr_invalid_id)
            return tid;
        return _ResolveAliasRecurse(tid);
    }

    CR_TypeID _ResolveAliasRecurse(CR_TypeID tid) const;

    //
    // accessors
    //

    CR_TypeID TypeIDFromFlags(CR_TypeFlags flags) const;

    CR_TypeID TypeIDFromName(const std::string& name) const;

    std::string NameFromTypeID(CR_TypeID tid) const;

    CR_LogType& LogType(CR_TypeID tid) {
        assert(tid < m_types.size());
        return m_types[tid];
    }

    const CR_LogType& LogType(CR_TypeID tid) const {
        assert(tid < m_types.size());
        return m_types[tid];
    }

    CR_LogVar& LogVar(CR_VarID vid) {
        assert(vid < m_vars.size());
        return m_vars[vid];
    }

    const CR_LogVar& LogVar(CR_VarID vid) const {
        assert(vid < m_vars.size());
        return m_vars[vid];
    }

    std::map<std::string, CR_TypeID>& MapNameToTypeID()
    { return m_mNameToTypeID; }

    const std::map<std::string, CR_TypeID>& MapNameToTypeID() const
    { return m_mNameToTypeID; }

    std::map<CR_TypeID, std::string>& MapTypeIDToName()
    { return m_mTypeIDToName; }

    const std::map<CR_TypeID, std::string>& MapTypeIDToName() const
    { return m_mTypeIDToName; }

    std::map<std::string, CR_VarID>& MapNameToVarID()
    { return m_mNameToVarID; }

    const std::map<std::string, CR_VarID>& MapNameToVarID() const
    { return m_mNameToVarID; }

    std::map<CR_VarID, std::string>& MapVarIDToName()
    { return m_mVarIDToName; }

    const std::map<CR_VarID, std::string>& MapVarIDToName() const
    { return m_mVarIDToName; }

          CR_DeqSet<CR_LogVar>& Vars()       { return m_vars; }
    const CR_DeqSet<CR_LogVar>& Vars() const { return m_vars; }

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

          CR_DeqSet<CR_LogType>& LogTypes()       { return m_types; }
    const CR_DeqSet<CR_LogType>& LogTypes() const { return m_types; }

          CR_DeqSet<CR_LogStruct>& LogStructs()       { return m_structs; }
    const CR_DeqSet<CR_LogStruct>& LogStructs() const { return m_structs; }

          CR_DeqSet<CR_LogFunc>& LogFuncs()       { return m_funcs; }
    const CR_DeqSet<CR_LogFunc>& LogFuncs() const { return m_funcs; }

          CR_DeqSet<CR_LogEnum>& LogEnums()       { return m_enums; }
    const CR_DeqSet<CR_LogEnum>& LogEnums() const { return m_enums; }

          CR_DeqSet<CR_LogVar>& LogVars()       { return m_vars; }
    const CR_DeqSet<CR_LogVar>& LogVars() const { return m_vars; }

protected:
    shared_ptr<CR_ErrorInfo>            m_error_info;
    bool                                m_is_64bit;
    std::map<std::string, CR_TypeID>    m_mNameToTypeID;
    std::map<CR_TypeID, std::string>    m_mTypeIDToName;
    std::map<std::string, CR_VarID>     m_mNameToVarID;
    std::map<CR_VarID, std::string>     m_mVarIDToName;
    std::map<std::string, CR_TypeID>    m_mNameToFuncTypeID;
    CR_DeqSet<CR_LogType>               m_types;
    CR_DeqSet<CR_LogFunc>               m_funcs;
    CR_DeqSet<CR_LogStruct>             m_structs;
    CR_DeqSet<CR_LogEnum>               m_enums;
    CR_DeqSet<CR_LogVar>                m_vars;
}; // class CR_NameScope

#endif  // ndef TYPESYSTEM_H_
