<?php

// Wonders data format version
const $cr_data_version = 1;

const TF_VOID         = 0x00000001;
const TF_CHAR         = 0x00000002;
const TF_SHORT        = 0x00000004;
const TF_LONG         = 0x00000008;
const TF_LONGLONG     = 0x00000010;
const TF_INT          = 0x00000020;
//
const TF_FLOAT        = 0x00000080;
const TF_DOUBLE       = 0x00000100;
const TF_SIGNED       = 0;
const TF_UNSIGNED     = 0x00000200;
const TF_PTR64        = 0x00000400;
const TF_STRUCT       = 0x00000800;
const TF_UNION        = 0x00001000;
const TF_ENUM         = 0x00002000;
const TF_POINTER      = 0x00004000;
const TF_ARRAY        = 0x00008000;
const TF_FUNCTION     = 0x00010000;
const TF_INCOMPLETE   = 0x00020000;
const TF_CDECL        = 0;
const TF_STDCALL      = 0x00040000;
const TF_FASTCALL     = 0x00080000;
const TF_CONST        = 0x00100000;
//
const TF_COMPLEX      = 0x00400000;
const TF_IMAGINARY    = 0x00800000;
const TF_ATOMIC       = 0x01000000;
const TF_PTR32        = 0x02000000;
const TF_INACCURATE   = 0x04000000;   // size and/or alignment is not accurate
const TF_VECTOR       = 0x08000000;
const TF_BITFIELD     = 0x10000000;
const TF_ALIAS        = 0x20000000;
const TF_INT128       = 0x80000000;

function CrNormalizeTypeFlags($flags) {
    if ($flags & TF_INT) {
        // remove "int" if wordy
        if ($flags & TF_SHORT)
            $flags &= ~TF_INT;
        else if ($flags & TF_LONG)
            $flags &= ~TF_INT;
        else if ($flags & TF_LONGLONG)
            $flags &= ~TF_INT;
        else if ($flags & TF_INT128)
            $flags &= ~TF_INT;
    }
    if (($flags & TF_UNSIGNED) &&
        !($flags & (TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG |
                   TF_INT128 | TF_INT)))
    {
        // add "int" for single "unsigned"
        $flags |= TF_INT;
    }
    // add "int" if no type specified
    if ($flags == 0)
        $flags = TF_INT;
    // remove storage class specifiers
    return $flags & ~TF_INCOMPLETE;
} // CrNormalizeTypeFlags


function CrChop($str) {
	return chop($str);
}

function CrEscapeString($str) {
	return '"' . addcslashes($str, "\0..\x1F\'\"?\\\a\b\f\r\t\v") . '"';
}

function CrUnscapeString(&$ret, $str) {
	$str = trim($str);
	if ($str[0] == '"') {
		$str = substr($str, 1);
	}
	if ($str[strlen($str) - 1] == '"') {
		$str = substr($str, 0, strlen($str) - 1);
	}
	$ret = stripcslashes($str);
	return true;
}

function CrEscapeChar($str) {
	$ret = "'";
	for ($str as $ch) {
		$ret .= addcslashes($ch, "\0..\x1F\'\"?\\\a\b\f\r\t\v");
	}
	return $ret + "'";
}

std::string CrUnescapeChar(const std::string& str) {
}

$cr_invalid_id = -1;

class CR_TypedValue {
    var $m_value;
    var $m_size;
    var $m_type_id;
    var $m_text;
    var $m_extra;

    function empty() {
        return m_size == 0 || !isset($this->m_value);
    }

    function size() {
        return $this->m_size;
    }
    
    function get() {
        return $this->m_value;
    }

    function assign($value, $size) {
        $this->m_value = $value;
        $this->m_size = $size;
    }
    
    function __construct($type_id, $value = NULL, $size = 0) {
        $this->m_type_id = $type_id;
        $this->m_value = $value;
        $this->m_size = $size;
        $this->m_text = "";
        $this->m_extra = "";
    }
}

class CR_LogType {
    var $m_flags;
    var $m_sub_id;
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
    var $m_count;
    var $m_size;
    var $m_align;
    var $m_alignas;
    var $m_alignas_explicit;
    var $m_location;

    function __construct() {
        $this->m_count = 0;
        $this->m_size = 0;
        $this->m_align = 0;
        $this->m_alignas = 0;
        $this->m_alignas_explicit = 0;
        $this->m_location = array("", 0);
    }
}

class CR_FuncParam {
    var $m_type_id;
    var $m_name;
    
    function __construct($tid, $name) {
        $this->m_type_id = tid;
        $this->m_name = name;
    }
}

// for CR_LogFunc::m_convention
const FT_CDECL = 0;
const FT_STDCALL = 1;
const FT_FASTCALL = 2;

class CR_LogFunc {
    var $m_ellipsis;
    var $m_return_type;
    var $m_convention;
    var $m_params;

    function __construct() {
        $this->m_ellipsis = false;
        $this->m_return_type = 0;
        $this->m_convention = FT_CDECL;
        $this->m_params = array();
    }
}

class CR_StructMember {
    var $m_type_id;
    var $m_name;
    var $m_bit_offset;
    var $m_bits;
    
    function __construct($tid, $name, $bit_offset = 0, $bits = -1) {
        $this->m_type_id = $tid;
        $this->m_name = $name;
        $this->m_bit_offset = $bit_offset;
        $this->m_bits = $bits;
    }
}

class CR_LogStruct {
    var $m_tid;
    var $m_is_struct;
    var $m_pack;
    var $m_align;
    var $m_alignas;
    var $m_alignas_explicit;
    var $m_is_complete;
    var $m_members;

    function __construct($is_struct = true) {
        $this->m_tid = 0;
        $this->m_is_struct = $is_struct;
        $this->m_pack = 8;
        $this->m_align = 0;
        $this->m_alignas = 0;
        $this->m_alignas_explicit = 0;
        $this->m_is_complete = false;
        $this->m_members = array();
    }
}

class CR_LogEnum {
    var $m_mNameToValue;
    var $m_mValueToName;
    function __construct() {
        $this->m_mNameToValue = array();
        $this->m_mValueToName = array();
    }
}

class CR_LogVar {
    var $m_typed_value;
    var $m_location;

    function __construct($type_id, $value = NULL, $size = 0) {
        $this->m_typed_value = new CR_TypedValue($type_id, $value, $size);
        $this->m_location = array("", 0);
    }
}; // struct CR_LogVar

class CR_NameScope {
    var $m_is_64bit;
    var $m_mNameToTypeID;
    var $m_mTypeIDToName;
    var $m_mNameToVarID;
    var $m_mVarIDToName;
    var $m_mNameToFuncTypeID;
    var $m_types;
    var $m_funcs;
    var $m_structs;
    var $m_enums;
    var $m_vars;

    function __construct($is_64bit = false) {
        $this->m_is_64bit = $is_64bit;
        $this->m_mNameToTypeID = array();
        $this->m_mTypeIDToName = array();
        $this->m_mNameToVarID = array();
        $this->m_mVarIDToName = array();
        $this->m_mNameToFuncTypeID = array();
        $this->m_types = array();
        $this->m_funcs = array();
        $this->m_structs = array();
        $this->m_enums = array();
        $this->m_vars = array();
    }

    function Is64Bit() {
        return m_is_64bit;
    }

    function LoadFromFiles($prefix = "", $suffix = ".dat") {
        $this->m_types = array();
        $this->m_structs = array();
        $this->m_enums = array();
        $this->m_funcs = array();
        $this->m_mNameToTypeID = array();
        $this->m_mTypeIDToName = array();
        $this->m_mNameToVarID = array();
        $this->m_mVarIDToName = array();

        $fp = fopen($prefix . "types" . $suffix, "r");
        if ($fp) {
            fgets($fp); // skip header
            while ($line = fgets($fp)) {
                $line = chop($line);
                $fields = explode("\t", $line);
                list($type_id, $name, $flags, $sub_id, $count, $size,
                     $align, $alignas_, $alignas_explicit,
                     $file, $line) = $fields;

                if (strlen($name) > 0) {
                    $this->m_mNameToTypeID[$name] = $type_id;
                    $this->m_mTypeIDToName[$type_id] = $name;
                } else {
                    $this->m_mTypeIDToName[$type_id] = "";
                }

                $type = new CR_LogType();
                $type->m_flags = intval($flags, 0);
                $type->m_sub_id = intval($sub_id, 0);
                $type->m_size = intval($size, 0);
                $type->m_count = intval($count, 0);
                $type->m_align = intval($align, 0);
                $type->m_alignas = intval($alignas_, 0);
                $type->m_alignas_explicit = intval($alignas_explicit, 0);
                $type->m_location = array($file, intval($line, 0));

                $this->m_types[$type_id] = $type;
            }
            fclose($fp);
        } else {
            return false;
        }

        $fp = fopen($prefix . "structures" . $suffix, "r");
        if ($fp) {
            fgets($fp); // skip header
            while ($line = fgets($fp)) {
                $line = chop($line);
                $fields = explode("\t", $line);
                list($struct_id, $name, $type_id, $flags, $is_struct,
                     $size, $count, $pack, $align, $alignas_,
                     $alignas_explicit, $file, $line) = $fields;

                $structure = new CR_LogStruct($is_struct);
                $structure->m_tid = intval($type_id, 0);
                $structure->m_pack = intval($pack, 0);
                $structure->m_align = intval($align, 0);
                $structure->m_alignas = intval($alignas_, 0);
                $structure->m_alignas_explicit = intval($alignas_explicit, 0);
                $structure->m_is_complete = !($flags & TF_INCOMPLETE);

                for ($i = 0; $i < $count; ++$i) {
                    $j = 12 + $i * 3;

                    $name = $fields[$j + 0];
                    $type_id = intval($fields[$j + 1], 0);
                    $bit_offset = $fields[$j + 2];
                    $bits = intval($fields[$j + 3], 0);

                    $mem = new CR_StructMember($type_id, $name, $bit_offset, $bits);
                    $structure->m_members[] = $mem;
                }
            }
            fclose($fp);
        } else {
            return false;
        }

        $fp = fopen($prefix . "enum" . $suffix, "r");
        if ($fp) {
            fgets($fp); // skip header
            while ($line = fgets($fp)) {
                $line = chop($line);
                $fields = explode("\t", $line);
                list($enum_id, $num_items) = $fields;

                $enum_id = intval($enum_id, 0);
                $num_items = intval($num_items, 0);

                $enum = new CR_LogEnum();
                for ($i = 0; $i < $num_items; ++$i) {
                    $j = 2 + $i * 2;
                    $name = $fields[$j + 0];
                    $value = $fields[$j + 1];
                    $enum->m_mNameToValue[$name] = intval($value, 0);
                    $enum->m_mValueToName[$value] = $name;
                }
                $this->m_enums[] = $enum;
            }
            fclose($fp);
        } else {
            return false;
        }

        $fp = fopen($prefix . "func_types" . $suffix, "r");
        if ($fp) {
            fgets($fp); // skip header
            while ($line = fgets($fp)) {
                $line = chop($line);
                $fields = explode("\t", $line);
                list($func_id, $return_type, $func_type, $ellipsis,
                     $param_count) = $fields;

                $function = new CR_LogFunc();
                $function->m_ellipsis = !!intval($ellipsis, 0);
                $function->m_return_type = intval($return_type, 0);
                $function->m_convention = intval($func_type, 0);

                $param_count = intval($param_count, 0);
                for ($i = 0; $i < $param_count; ++$i) {
                    $j = 5 + $i * 2;

                    $type_id = intval($fields[$j + 0], 0);
                    $name = $fields[$j + 1];

                    $param = new CR_FuncParam($type_id, $name);
                    $function->m_params[] = $param;
                }
                $this->m_funcs[] = $function;
            }
            fclose($fp);
        } else {
            return false;
        }

        $fp = fopen($prefix . "vars" . $suffix, "r");
        if ($fp) {
            fgets($fp); // skip header
            while ($line = fgets($fp)) {
                $line = chop($line);
                $fields = explode("\t", $line);
                list($var_id, $name, $type_id, $text, 
                     $extra, $value_type, $file, $lineno) = $fields;

                $var = new CR_LogVar(intval($type_id, 0));
                $var->m_typed_value->m_text = $text;
                $var->m_extra = $extra;
                if ($value_type == "i" && IsIntegralType($type_id)) {
                    $var->m_typed_value->m_value = intval($text, 0);
                    if (strpos($extra, "LL") !== FALSE || strpos($extra, "ll") !== FALSE) {
                        $var->m_typed_value->m_size = 8;
                    } else {
                        $var->m_typed_value->m_size = 4;
                    }
                } else if ($value_type == "f" && IsFloatingType($type_id)) {
                    $var->m_typed_value->m_value = floatval($text);
                    if ($extra == "L" || $extra == "l") {
                        $var->m_typed_value->m_size = 8;
                    } else if ($extra == "F" || $extra == "F") {
                        $var->m_typed_value->m_size = 4;
                    } else {
                        $var->m_typed_value->m_size = 8
                    }
                } else if ($value_type == "s" && IsStringType($type_id)) {
                    $var->m_typed_value->m_value = $text;
                    $var->m_typed_value->m_size = strlen($text) + 1;
                } else if ($value_type == "s" && IsWStringType($type_id)) {
                    $var->m_typed_value->m_value = $text;
                    $var->m_typed_value->m_size = (strlen($text) + 1) * 2;
                }
                $var->m_location = array($file, intval($lineno, 0));
                $this->m_vars[] = $var;
            }
            fclose($fp);
        } else {
            return false;
        }

        return true;
    }

    function SaveToFiles() {
        return false;   // NIY
    }

    function SizeOfType($tid) {
        return LogType($tid)->m_size;
    }

    function StringOfEnumTag($name) {
        $str = "enum ";
        if (strlen($name) > 0) {
            assert(strpos($name, "enum ") == 0);
            $str .= substr($name, 5);
            $str .= ' ';
        }
        return $str;
    }

    function StringOfEnum($name, $eid) {
        if ($eid == $cr_invalid_id) {
            return "";
        }
        $e = $this->m_enums[$eid];
        $str = StringOfEnumTag($name);
        if (strlen($e) > 0) {
            $str .= "{ ";
            $array = array();
            foreach ($e->m_mNameToValue as $key => $value) {
                $s = $key . " = " . $value;
            }
            $str += implode(", ", $array);
            $str += "} ";
        }
        return $str;
    }

    function StringOfStructTag($name, $s) {
        $str = "";
        if ($s->m_is_struct) {
            $str .= "struct ";
        } else {
            $str .= "union ";
        }

        $type = LogType($s->m_tid);
        if ($type->m_alignas && $type->m_alignas_explicit) {
            $str .= "_Alignas(";
            $str .= $type->m_alignas;
            $str .= ") ";
        }

        if (strlen($name) > 0) {
            if ($s->m_is_struct) {
                assert(strpos($name, "struct ") == 0);
                $str .= substr($name, 7);
            } else {
                assert(strpos($name, "union ") == 0);
                $str .= substr($name, 6);
            }
            $str .= ' ';
        }
        return $str;
    }

    function StringOfStruct($name, $sid) {
        $s = LogStruct($sid);
        $str = StringOfStructTag($name, $s);
        if (strlen($s) > 0) {
            $str .= "{ ";
            $siz = count($s->m_members);
            for ($i = 0; $i < $siz; ++$i) {
                $str .= StringOfType($s->m_members[$i]->m_type_id, 
                                     $s->m_members[$i]->m_name, false);
                if ($s->m_bits_list[i] != -1) {
                    $str .= " : ";
                    $str .= $s->m_bits_list[$i];
                }
                $str += "; ";
            }
            $str .= "} ";
        }
        return $str;
    }

    function StringOfType($tid, $name, $expand = true, $no_convension = false) {
        $type = LogType($tid);
        $type_name = NameFromTypeID($tid);
        if ($type->m_flags & TF_ALIAS) {
            if ($expand || strlen($type_name) == 0) {
                return StringOfType($type->m_sub_id, $name, false);
            } else {
                return $type_name . " " . $name;
            }
        }
        if ($type->m_flags & (TF_STRUCT | TF_UNION)) {
            if ($expand || strlen($type_name) == 0) {
                return StringOfStruct($type_name, $type->m_sub_id) . $name;
            } else {
                return $type_name . " " . $name;
            }
        }
        if ($type->m_flags & TF_ENUM) {
            if ($expand || strlen($type_name) == 0) {
                return StringOfEnum($type_name, $type->m_sub_id) . $name;
            } else {
                return $type_name . " " . $name;
            }
        }
        if ($type->m_flags & (TF_ARRAY | TF_VECTOR)) {
            if ($type->m_count) {
                $s = "[" . $type->m_count . "]";
                return StringOfType($type->m_sub_id, $name . $s, false);
            } else {
                return StringOfType($type->m_sub_id, $name . "[]", false);
            }
        }
        if ($type->m_flags & TF_FUNCTION) {
            $func = LogFunc($type->m_sub_id);
            $rettype = StringOfType($func->m_return_type, "", false);
            $paramlist = StringOfParamList($func->m_type_list, $func->m_name_list);
            $convension = "";
            if (!$no_convension) {
                if ($type->m_flags & TF_STDCALL) {
                    $convension = "__stdcall ";
                } else if ($type->m_flags & TF_FASTCALL) {
                    $convension = "__fastcall ";
                } else {
                    $convension = "__cdecl ";
                }
            }
            if ($func->m_ellipsis) {
                $paramlist .= ", ...";
            }
            return $rettype . $convension . $name . "(" . $paramlist . ")";
        }
        if ($type->m_flags & TF_POINTER) {
            $sub_id = $type->m_sub_id;
            $type2 = LogType($sub_id);
            if ($type2->m_flags & TF_FUNCTION) {
                if ($type->m_flags & TF_CONST) {
                    if ($type->m_flags & TF_STDCALL) {
                        return StringOfType($sub_id, "(__stdcall * const " . $name . ")", false, true);
                    } else if ($type->m_flags & TF_FASTCALL) {
                        return StringOfType($sub_id, "(__fastcall * const " . $name . ")", false, true);
                    } else {
                        return StringOfType($sub_id, "(__cdecl * const " . $name . ")", false, true);
                    }
                } else {
                    if ($type->m_flags & TF_STDCALL) {
                        return StringOfType($sub_id, "(__stdcall *" . $name . ")", false, true);
                    } else if ($type->m_flags & TF_FASTCALL) {
                        return StringOfType($sub_id, "(__fastcall *" . $name . ")", false, true);
                    } else {
                        return StringOfType($sub_id, "(__cdecl *" . $name . ")", false, true);
                    }
                }
            } else if ($type2->m_flags & TF_POINTER) {
                if ($type->m_flags & TF_CONST) {
                    return StringOfType($sub_id, "(* const " . $name . ")", false);
                } else {
                    return StringOfType($sub_id, "(*" . $name . ")", false);
                }
            } else if ($type2->m_flags & (TF_ARRAY | TF_VECTOR)) {
                if ($type->m_flags & TF_CONST) {
                    if ($type2->m_count) {
                        $s = "[" . $type2->m_count . "]";
                        return StringOfType($sub_id, "(* const " . $name . $s . ")", false);
                    } else {
                        return StringOfType($sub_id, "(* const " . $name . "[]" . ")", false);
                    }
                } else {
                    if ($type2->m_count) {
                        $s = "[" . $type2->m_count . "]";
                        return StringOfType($sub_id, "(*" . $name . $s . ")", false);
                    } else {
                        return StringOfType($sub_id, "(*" . $name . "[]" . ")", false);
                    }
                }
            } else {
                if ($type->m_flags & TF_CONST) {
                    return StringOfType($sub_id, "", false) . "* const " . $name;
                } else {
                    return StringOfType($sub_id, "", false) . "*" . $name;
                }
            }
        }
        if ($type->m_flags & TF_CONST) {
            return "const " . StringOfType($type->m_sub_id, $name, false);
        }
        if (strlen($type_name)) {
            return $type_name . " " . $name;
        }
        return "";
    }

    function StringOfParamList($type_list, $name_list) {
        assert(count($type_list) == count($name_list));
        $size = count($type_list);
        $str = "";
        if ($size > 0) {
            assert($type_list[0] != $cr_invalid_id);
            $str .= StringOfType($type_list[0], $name_list[0], false);
            for ($i = 1; $i < $size; ++$i) {
                $str .= ", ";
                assert($type_list[i] != $cr_invalid_id);
                $str .= StringOfType($type_list[i], $name_list[i], false);
            }
        } else {
            $str .= "void";
        }
        return $str;
    }

    function AddConstCharType() {
        $tid = TypeIDFromFlags(TF_CHAR);
        return AddConstType($tid);
    }

    function AddConstUnsignedCharType() {
        $tid = TypeIDFromFlags(TF_UNSIGNED | TF_CHAR);
        return AddConstType($tid);
    }

    function AddConstShortType() {
        $tid = TypeIDFromFlags(TF_SHORT);
        return AddConstType($tid);
    }

    function AddConstUnsignedShortType() {
        $tid = TypeIDFromFlags(TF_UNSIGNED | TF_SHORT);
        return AddConstType($tid);
    }

    function AddConstIntType() {
        $tid = TypeIDFromFlags(TF_INT);
        return AddConstType($tid);
    }

    function AddConstUnsignedIntType() {
        $tid = TypeIDFromFlags(TF_UNSIGNED | TF_INT);
        return AddConstType($tid);
    }

    function AddConstLongType() {
        $tid = TypeIDFromFlags(TF_LONG);
        return AddConstType($tid);
    }

    function AddConstUnsignedLongType() {
        $tid = TypeIDFromFlags(TF_UNSIGNED | TF_LONG);
        return AddConstType($tid);
    }
    
    function AddConstLongLongType() {
        $tid = TypeIDFromFlags(TF_LONGLONG);
        return AddConstType($tid);
    }
    
    function AddConstUnsignedLongLongType() {
        $tid = TypeIDFromFlags(TF_UNSIGNED | TF_LONGLONG);
        return AddConstType($tid);
    }
    
    function AddConstFloatType() {
        $tid = TypeIDFromFlags(TF_FLOAT);
        return AddConstType($tid);
    }
    
    function AddConstDoubleType() {
        $tid = TypeIDFromFlags(TF_DOUBLE);
        return AddConstType($tid);
    }
    
    function AddConstLongDoubleType() {
        $tid = TypeIDFromFlags(TF_LONG | TF_DOUBLE);
        return AddConstType($tid);
    }
    
    function AddConstStringType() {
        $tid = TypeIDFromFlags(TF_CHAR);
        $type = LogType($tid)
        $tid = AddConstType($tid);
        $tid = AddPointerType($tid, 0, $type->location());
        return true;
    }

    function AddConstWStringType() {
        $tid = TypeIDFromName("wchar_t");
        $type = LogType($tid)
        $tid = AddConstType($tid);
        $tid = AddPointerType($tid, 0, $type->location());
        return true;
    }
    
    function IsStringType($tid) {
        $tid = ResolveAlias($tid);
        $type1 = LogType($tid);
        if ($type1->m_flags & (TF_POINTER | TF_ARRAY)) {
            $tid2 = ResolveAlias($type1->m_sub_id);
            $type2 = LogType($tid2);
            if (($type2->m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
                $tid3 = ResolveAlias($type2->m_sub_id);
                $type3 = LogType($tid3);
                return ($type3->m_flags & TF_CHAR) != 0;
            } else if (type2->m_flags & TF_CHAR) {
                return true;
            }
        }
        return false;
    }
    
    function IsWStringType($tid) {
        $tid = ResolveAlias($tid);
        $type1 = LogType($tid);
        if ($type1->m_flags & (TF_POINTER | TF_ARRAY)) {
            $tid2 = ResolveAlias($type1->m_sub_id);
            $type2 = LogType($tid2);
            if (($type2->m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
                $tid3 = ResolveAlias($type2->m_sub_id);
                $type3 = LogType($tid3);
                return ($type3->m_flags & TF_SHORT) != 0;
            } else if (type2->m_flags & TF_SHORT) {
                return true;
            }
        }
        return false;
    }

    function IsFuncType($tid) {
        if ($tid == $cr_invalid_id) {
            return false;
        }
        $tid = ResolveAlias($tids);
        if (LogType($tid)->m_flags & TF_FUNCTION) {
            return true;
        }
        return false;
    }

    function IsPredefinedType($tid) {
        $type = LogType($tid);
        if ($type->m_flags & (TF_POINTER | TF_ARRAY | TF_CONST)) {
            return IsPredefinedType($type->m_sub_id);
        }
        $flags = (TF_ALIAS | TF_FUNCTION | TF_STRUCT | TF_ENUM |
                  TF_UNION | TF_VECTOR);
        if ($type->m_flags & $flags) {
            return false;
        }
        return true;
    }

    function IsIntegralType($tid) {
        if ($tid == $cr_invalid_id) {
            return false;
        }
        $tid = ResolveAlias($tid);
        $type = LogType($tid);
        $not_flags = (
            TF_DOUBLE | TF_FLOAT | TF_POINTER | TF_ARRAY | TF_VECTOR |
            TF_FUNCTION | TF_VA_LIST | TF_STRUCT | TF_UNION | TF_ENUM);
        if ($type->m_flags & $not_flags) {
            return false;
        }
        $flags =
            (TF_INT | TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG);
        if ($type->m_flags & $flags) {
            return true;
        }
        if (($type->m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            return IsIntegralType($type->m_sub_id);
        }
        return false;
    }

    function IsFloatingType($tid) {
        if ($tid == $cr_invalid_id) {
            return false;
        }
        $tid = ResolveAlias($tid);
        $type = LogType($tid);
        if ($type->m_flags & (TF_DOUBLE | TF_FLOAT)) {
            return true;
        }
        if (($type->m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            return IsFloatingType($type->m_sub_id);
        }
        return false;
    }

    function IsUnsignedType($tid) {
        if ($tid == $cr_invalid_id) {
            return false;
        }
        $tid = ResolveAlias($tid);
        $type = LogType($tid);
        if ($type->m_flags & TF_UNSIGNED) {
            return true;
        }
        if (($type->m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            return IsUnsignedType($type->m_sub_id);
        }
        return false;
    }

    function IsPointerType($tid) {
        if ($tid == $cr_invalid_id) {
            return false;
        }
        $tid = ResolveAlias($tid);
        $type = LogType($tid);
        if ($type->m_flags & TF_POINTER) {
            return true;
        }
        if (($type->m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            return IsUnsignedType($type->m_sub_id);
        }
        return false;
    }

    function ResolveAlias($tid) {
        if ($tid == $cr_invalid_id) {
            return $tid;
        }
        return _ResolveAliasRecurse($tid);
    }

    function _ResolveAliasRecurse($tid) {
        while ($this->m_types[$tid]->m_flags & TF_ALIAS) {
            $tid = $this->m_types[$tid]->m_sub_id;
        }
        return $tid;
    }

    function ResolveAliasAndCV($tid) {
        $tid = ResolveAlias($tid);
        if ($tid == $cr_invalid_id) {
            return $tid;
        }
        $type = LogType($tid);
        if (($type->m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            return ResolveAliasAndCV($type->m_sub_id);
        }
        return $tid;
    }

    function TypeIDFromFlags($flags) {
        $siz = count($this->m_types);
        for ($i = 0; $i < $siz; ++$i) {
            if (LogType($i)->m_flags == $flags) {
                return $i;
            }
        }
        return $cr_invalid_id;
    }

    function TypeIDFromName($name) {
        return isset($this->m_mNameToTypeID[$tid]) ? 
            $this->m_mNameToTypeID[$tid] : $cr_invalid_id;
    }

    function NameFromTypeID($tid) {
        return isset($this->m_mTypeIDToName[$tid]) ? 
            $this->m_mTypeIDToName[$tid] : "";
    }
    
    function GetStructMemberList($sid, &$members) {
        $members = array();
        $ls = LogStruct($sid);
        foreach ($ls->m_members as $mem) {
            if (strlen($mem->m_name) > 0) {
                $members[] = $mem;
            } else {
                $tid = ResolveAlias($mem->m_type_id);
                $type = LogType($tid);
                if ($type->m_flags & (TF_STRUCT | TF_UNION)) {
                    $children = array();
                    GetStructMemberList($type->m_sub_id, $children);
                    foreach ($children as $child) {
                        $child->m_bit_offset += $mem->m_bit_offset;
                    }
                    $members = array_merge($members, $children);
                }
            }
        }
    }

    function LogType($tid) {
        assert($tid < count($this->m_types));
        return $this->m_types[$tid];
    }

    function LogVar($vid) {
        assert($vid < count($this->m_vars));
        return $this->m_vars[$vid];
    }

    function MapNameToTypeID() {
        return $this->m_mNameToTypeID;
    }

    function MapTypeIDToName() {
        return $this->m_mTypeIDToName;
    }

    function MapNameToVarID() {
        return $this->m_mNameToVarID;
    }

    function MapVarIDToName() {
        return $this->m_mVarIDToName;
    }

    function LogVars() {
        return $this->m_vars;
    }

    function LogStruct($sid) {
        assert($sid < count($this->m_structs));
        return $this->m_structs[$sid];
    }

    function LogFunc($fid) {
        assert($fid < count($this->m_funcs));
        return $this->m_funcs[$fid];
    }

    function LogEnum($eid) {
        assert($eid < count($this->m_enums));
        return $this->m_enums[$eid];
    }

    function LogTypes() {
        return $this->m_types;
    }

    function LogStructs() {
        return $this->m_structs;
    }
}
