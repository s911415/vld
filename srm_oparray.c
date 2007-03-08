/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors:  Derick Rethans <derick@derickrethans.nl>                   |
   |           Andrei Zmievski <andrei@gravitonic.com>                    |
   |           Marcus B�rger <marcus.boerger@t-online.de>                 |
   +----------------------------------------------------------------------+
 */
/* $Id: srm_oparray.c,v 1.51 2007-03-08 18:47:14 helly Exp $ */

#include "php.h"
#include "zend_alloc.h"
#include "srm_oparray.h"
#include "ext/standard/url.h"
#include "set.h"
#include "php_vld.h"

ZEND_EXTERN_MODULE_GLOBALS(vld)

/* Input zend_compile.h
 * And replace [^(...)(#define )([^ \t]+).*$]
 * BY     [/=*  \1 *=/  { "\3", ALL_USED },] REMEMBER to remove the two '=' signs
 */
static const op_usage opcodes[] = {
	/*  0 */	{ "NOP", NONE_USED },
	/*  1 */	{ "ADD", ALL_USED },
	/*  2 */	{ "SUB", ALL_USED },
	/*  3 */	{ "MUL", ALL_USED },
	/*  4 */	{ "DIV", ALL_USED },
	/*  5 */	{ "MOD", ALL_USED },
	/*  6 */	{ "SL", ALL_USED },
	/*  7 */	{ "SR", ALL_USED },
	/*  8 */	{ "CONCAT", ALL_USED },
	/*  9 */	{ "BW_OR", ALL_USED },
	/*  10 */	{ "BW_AND", ALL_USED },
	/*  11 */	{ "BW_XOR", ALL_USED },
	/*  12 */	{ "BW_NOT", RES_USED | OP1_USED },
	/*  13 */	{ "BOOL_NOT", RES_USED | OP1_USED },
	/*  14 */	{ "BOOL_XOR", ALL_USED },
	/*  15 */	{ "IS_IDENTICAL", ALL_USED },
	/*  16 */	{ "IS_NOT_IDENTICAL", ALL_USED },
	/*  17 */	{ "IS_EQUAL", ALL_USED },
	/*  18 */	{ "IS_NOT_EQUAL", ALL_USED },
	/*  19 */	{ "IS_SMALLER", ALL_USED },
	/*  20 */	{ "IS_SMALLER_OR_EQUAL", ALL_USED },
	/*  21 */	{ "CAST", ALL_USED },
	/*  22 */	{ "QM_ASSIGN", RES_USED | OP1_USED },
	/*  23 */	{ "ASSIGN_ADD", ALL_USED },
	/*  24 */	{ "ASSIGN_SUB", ALL_USED },
	/*  25 */	{ "ASSIGN_MUL", ALL_USED },
	/*  26 */	{ "ASSIGN_DIV", ALL_USED },
	/*  27 */	{ "ASSIGN_MOD", ALL_USED },
	/*  28 */	{ "ASSIGN_SL", ALL_USED },
	/*  29 */	{ "ASSIGN_SR", ALL_USED },
	/*  30 */	{ "ASSIGN_CONCAT", ALL_USED },
	/*  31 */	{ "ASSIGN_BW_OR", ALL_USED },
	/*  32 */	{ "ASSIGN_BW_AND", ALL_USED },
	/*  33 */	{ "ASSIGN_BW_XOR", ALL_USED },
	/*  34 */	{ "PRE_INC", OP1_USED | RES_USED },
	/*  35 */	{ "PRE_DEC", OP1_USED | RES_USED },
	/*  36 */	{ "POST_INC", OP1_USED | RES_USED },
	/*  37 */	{ "POST_DEC", OP1_USED | RES_USED },
	/*  38 */	{ "ASSIGN", ALL_USED },
	/*  39 */	{ "ASSIGN_REF", SPECIAL },
	/*  40 */	{ "ECHO", OP1_USED },
	/*  41 */	{ "PRINT", RES_USED | OP1_USED },
	/*  42 */	{ "JMP", OP1_USED | OP1_OPLINE },
	/*  43 */	{ "JMPZ", OP1_USED | OP2_USED | OP2_OPLINE },
	/*  44 */	{ "JMPNZ", OP1_USED | OP2_USED | OP2_OPLINE },
	/*  45 */	{ "JMPZNZ", SPECIAL },
	/*  46 */	{ "JMPZ_EX", ALL_USED | OP2_OPLINE },
	/*  47 */	{ "JMPNZ_EX", ALL_USED | OP2_OPLINE },
	/*  48 */	{ "CASE", ALL_USED },
	/*  49 */	{ "SWITCH_FREE", RES_USED | OP1_USED },
	/*  50 */	{ "BRK", ALL_USED },
	/*  51 */	{ "CONT", ALL_USED },
	/*  52 */	{ "BOOL", RES_USED | OP1_USED },
	/*  53 */	{ "INIT_STRING", RES_USED },
	/*  54 */	{ "ADD_CHAR", ALL_USED },
	/*  55 */	{ "ADD_STRING", ALL_USED },
	/*  56 */	{ "ADD_VAR", ALL_USED },
	/*  57 */	{ "BEGIN_SILENCE", ALL_USED },
	/*  58 */	{ "END_SILENCE", ALL_USED },
	/*  59 */	{ "INIT_FCALL_BY_NAME", SPECIAL },
	/*  60 */	{ "DO_FCALL", SPECIAL },
	/*  61 */	{ "DO_FCALL_BY_NAME", SPECIAL },
	/*  62 */	{ "RETURN", OP1_USED },
	/*  63 */	{ "RECV", RES_USED | OP1_USED },
	/*  64 */	{ "RECV_INIT", ALL_USED },
	/*  65 */	{ "SEND_VAL", OP1_USED },
	/*  66 */	{ "SEND_VAR", OP1_USED },
	/*  67 */	{ "SEND_REF", ALL_USED },
	/*  68 */	{ "NEW", SPECIAL },
#if (PHP_MAJOR_VERSION < 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 1)
	/*  69 */	{ "JMP_NO_CTOR", SPECIAL },
#else
	/*  69 */   { "UNKNOWN", ALL_USED },
#endif
	/*  70 */	{ "FREE", OP1_USED },
	/*  71 */	{ "INIT_ARRAY", ALL_USED },
	/*  72 */	{ "ADD_ARRAY_ELEMENT", ALL_USED },
	/*  73 */	{ "INCLUDE_OR_EVAL", ALL_USED | OP2_INCLUDE },
	/*  74 */	{ "UNSET_VAR", ALL_USED },
#ifdef ZEND_ENGINE_2
	/*  75 */	{ "UNSET_DIM", ALL_USED },
	/*  76 */	{ "UNSET_OBJ", ALL_USED },
#else
	/*  75 */	{ "UNSET_DIM_OBJ", ALL_USED },
	/*  76 */	{ "ISSET_ISEMPTY", ALL_USED },
#endif
	/*  77 */	{ "FE_RESET", SPECIAL },
	/*  78 */	{ "FE_FETCH", ALL_USED | OP2_OPNUM },
	/*  79 */	{ "EXIT", ALL_USED },
	/*  80 */	{ "FETCH_R", RES_USED | OP1_USED | OP_FETCH },
	/*  81 */	{ "FETCH_DIM_R", ALL_USED },
	/*  82 */	{ "FETCH_OBJ_R", ALL_USED },
	/*  83 */	{ "FETCH_W", RES_USED | OP1_USED | OP_FETCH },
	/*  84 */	{ "FETCH_DIM_W", ALL_USED },
	/*  85 */	{ "FETCH_OBJ_W", ALL_USED },
	/*  86 */	{ "FETCH_RW", RES_USED | OP1_USED | OP_FETCH },
	/*  87 */	{ "FETCH_DIM_RW", ALL_USED },
	/*  88 */	{ "FETCH_OBJ_RW", ALL_USED },
	/*  89 */	{ "FETCH_IS", ALL_USED },
	/*  90 */	{ "FETCH_DIM_IS", ALL_USED },
	/*  91 */	{ "FETCH_OBJ_IS", ALL_USED },
	/*  92 */	{ "FETCH_FUNC_ARG", RES_USED | OP1_USED | OP_FETCH },
	/*  93 */	{ "FETCH_DIM_FUNC_ARG", ALL_USED },
	/*  94 */	{ "FETCH_OBJ_FUNC_ARG", ALL_USED },
	/*  95 */	{ "FETCH_UNSET", ALL_USED },
	/*  96 */	{ "FETCH_DIM_UNSET", ALL_USED },
	/*  97 */	{ "FETCH_OBJ_UNSET", ALL_USED },
	/*  98 */	{ "FETCH_DIM_TMP_VAR", ALL_USED },
	/*  99 */	{ "FETCH_CONSTANT", ALL_USED },
	/*  100 */	{ "DECLARE_FUNCTION_OR_CLASS", ALL_USED },
	/*  101 */	{ "EXT_STMT", ALL_USED },
	/*  102 */	{ "EXT_FCALL_BEGIN", ALL_USED },
	/*  103 */	{ "EXT_FCALL_END", ALL_USED },
	/*  104 */	{ "EXT_NOP", ALL_USED },
	/*  105 */	{ "TICKS", ALL_USED },
	/*  106 */	{ "SEND_VAR_NO_REF", ALL_USED },
#ifdef ZEND_ENGINE_2
	/*  107 */	{ "ZEND_CATCH", ALL_USED | EXT_VAL },
	/*  108 */	{ "ZEND_THROW", ALL_USED | EXT_VAL },
	
	/*  109 */	{ "ZEND_FETCH_CLASS", SPECIAL },
	
	/*  110 */	{ "ZEND_CLONE", ALL_USED },
	
	/*  111 */	{ "ZEND_INIT_CTOR_CALL", ALL_USED },
	/*  112 */	{ "ZEND_INIT_METHOD_CALL", ALL_USED },
	/*  113 */	{ "ZEND_INIT_STATIC_METHOD_CALL", ALL_USED },
	
	/*  114 */	{ "ZEND_ISSET_ISEMPTY_VAR", ALL_USED | EXT_VAL },
	/*  115 */	{ "ZEND_ISSET_ISEMPTY_DIM_OBJ", ALL_USED | EXT_VAL },
	
	/*  116 */	{ "ZEND_IMPORT_FUNCTION", ALL_USED },
	/*  117 */	{ "ZEND_IMPORT_CLASS", ALL_USED },
	/*  118 */	{ "ZEND_IMPORT_CONST", ALL_USED },
	
	/*  119 */	{ "119", ALL_USED },
	/*  120 */	{ "120", ALL_USED },
	
	/*  121 */	{ "ZEND_ASSIGN_ADD_OBJ", ALL_USED },
	/*  122 */	{ "ZEND_ASSIGN_SUB_OBJ", ALL_USED },
	/*  123 */	{ "ZEND_ASSIGN_MUL_OBJ", ALL_USED },
	/*  124 */	{ "ZEND_ASSIGN_DIV_OBJ", ALL_USED },
	/*  125 */	{ "ZEND_ASSIGN_MOD_OBJ", ALL_USED },
	/*  126 */	{ "ZEND_ASSIGN_SL_OBJ", ALL_USED },
	/*  127 */	{ "ZEND_ASSIGN_SR_OBJ", ALL_USED },
	/*  128 */	{ "ZEND_ASSIGN_CONCAT_OBJ", ALL_USED },
	/*  129 */	{ "ZEND_ASSIGN_BW_OR_OBJ", ALL_USED },
	/*  130 */	{ "ZEND_ASSIGN_BW_AND_OBJ", ALL_USED },
	/*  131 */	{ "ZEND_ASSIGN_BW_XOR_OBJ", ALL_USED },

	/*  132 */	{ "ZEND_PRE_INC_OBJ", ALL_USED },
	/*  133 */	{ "ZEND_PRE_DEC_OBJ", ALL_USED },
	/*  134 */	{ "ZEND_POST_INC_OBJ", ALL_USED },
	/*  135 */	{ "ZEND_POST_DEC_OBJ", ALL_USED },

	/*  136 */	{ "ZEND_ASSIGN_OBJ", ALL_USED },
	/*  137 */	{ "ZEND_OP_DATA", ALL_USED },
	
	/*  138 */	{ "ZEND_INSTANCEOF", ALL_USED },
	
	/*  139 */	{ "ZEND_DECLARE_CLASS", ALL_USED },
	/*  140 */	{ "ZEND_DECLARE_INHERITED_CLASS", ALL_USED },
	/*  141 */	{ "ZEND_DECLARE_FUNCTION", ALL_USED },
	
	/*  142 */	{ "ZEND_RAISE_ABSTRACT_ERROR", ALL_USED },
	
	/*  143 */	{ "ZEND_START_NAMESPACE", ALL_USED },
	
	/*  144 */	{ "ZEND_ADD_INTERFACE", ALL_USED },
	/*  145 */	{ "ZEND_VERIFY_INSTANCEOF", ALL_USED },
	/*  146 */	{ "ZEND_VERIFY_ABSTRACT_CLASS", ALL_USED },
	/*  147 */	{ "ZEND_ASSIGN_DIM", ALL_USED },
	/*  148 */	{ "ZEND_ISSET_ISEMPTY_PROP_OBJ", ALL_USED },
	/*  149 */	{ "ZEND_HANDLE_EXCEPTION", NONE_USED },
#ifdef ZEND_USER_OPCODE
	/*  150 */	{ "ZEND_USER_OPCODE", ALL_USED },
#ifdef ZEND_U_NORMALIZE
	/*  151 */  { "ZEND_U_NORMALIZE", RES_USED | OP1_USED },
#ifdef ZEND_JMP_SET
	/*  152 */	{ "ZEND_JMP_SET", ALL_USED | OP2_OPLINE },
#endif
#endif
#endif
#endif
};

inline int vld_dump_zval_null(zvalue_value value)
{
	return fprintf (stderr, "null");
}

inline int vld_dump_zval_long(zvalue_value value)
{
	return fprintf (stderr, "%ld", value.lval);
}

inline int vld_dump_zval_double(zvalue_value value)
{
	return fprintf (stderr, "%g", value.dval);
}

inline int vld_dump_zval_string(zvalue_value value)
{
	char *new_str;
	int new_len, len;

	new_str = php_url_encode(value.str.val, value.str.len, &new_len);
	len = fprintf (stderr, "'%s'", new_str);
	efree(new_str);
	return len;
}

inline int vld_dump_zval_array(zvalue_value value)
{
	return fprintf (stderr, "<array>");
}

inline int vld_dump_zval_object(zvalue_value value)
{
	return fprintf (stderr, "<object>");
}

inline int vld_dump_zval_bool(zvalue_value value)
{
	return fprintf (stderr, value.lval ? "true" : "false");
}

inline int vld_dump_zval_resource(zvalue_value value)
{
	return fprintf (stderr, "<resource>");
}

inline int vld_dump_zval_constant(zvalue_value value)
{
	return fprintf (stderr, "<const>");
}

inline int vld_dump_zval_constant_array(zvalue_value value)
{
	return fprintf (stderr, "<const array>");
}


int vld_dump_zval (zval val)
{
	switch (val.type) {
		case IS_NULL:           return vld_dump_zval_null (val.value);
		case IS_LONG:           return vld_dump_zval_long (val.value);
		case IS_DOUBLE:         return vld_dump_zval_double (val.value);
		case IS_STRING:         return vld_dump_zval_string (val.value);
		case IS_ARRAY:          return vld_dump_zval_array (val.value);
		case IS_OBJECT:         return vld_dump_zval_object (val.value);
		case IS_BOOL:           return vld_dump_zval_bool (val.value);
		case IS_RESOURCE:       return vld_dump_zval_resource (val.value);
		case IS_CONSTANT:       return vld_dump_zval_constant (val.value);
		case IS_CONSTANT_ARRAY: return vld_dump_zval_constant_array (val.value);
	}
	return fprintf(stderr, "<unknown>");
}


int vld_dump_znode (int *print_sep, znode node, zend_uint base_address TSRMLS_DC)
{
	int len = 0;

	if (node.op_type != IS_UNUSED && print_sep) {
		if (*print_sep) {
			len += fprintf (stderr, ", ");
		}
		*print_sep = 1;
	}
	switch (node.op_type) {
		case IS_UNUSED:
			VLD_PRINT(3, " IS_UNUSED ");
			break;
		case IS_CONST: /* 1 */
			VLD_PRINT(3, " IS_CONST (%d) ", node.u.var / sizeof(temp_variable));
			vld_dump_zval (node.u.constant);
			break;
#ifdef ZEND_ENGINE_2
		case IS_TMP_VAR: /* 2 */
			VLD_PRINT(3, " IS_TMP_VAR ");
			len += fprintf (stderr, "~%d", node.u.var / sizeof(temp_variable));
			break;
		case IS_VAR: /* 4 */
			VLD_PRINT(3, " IS_VAR ");
			len += fprintf (stderr, "$%d", node.u.var / sizeof(temp_variable));
			break;
#if (PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1)
		case IS_CV:  /* 16 */
			VLD_PRINT(3, " IS_CV ");
			len += fprintf (stderr, "!%d", node.u.var);
			break;
#endif
		case VLD_IS_OPNUM:
			len += fprintf (stderr, "->%d", node.u.opline_num);
			break;
		case VLD_IS_OPLINE:
			len += fprintf (stderr, "->%d", (node.u.opline_num - base_address) / sizeof(zend_op));
			break;
		case VLD_IS_CLASS:
			len += fprintf (stderr, ":%d", node.u.var / sizeof(temp_variable));
			break;
#else
		case IS_TMP_VAR: /* 2 */
			len += fprintf (stderr, "~%d", node.u.var);
			break;
		case IS_VAR: /* 4 */
			len += fprintf (stderr, "$%d", node.u.var);
			break;           
		case VLD_IS_OPNUM:
		case VLD_IS_OPLINE:
			len += fprintf (stderr, "->%d", node.u.opline_num);
			break;
		case VLD_IS_CLASS:
			len += fprintf (stderr, ":%d", node.u.var);
			break;
#endif
		default:
			return 0;
	}
	return len;
}

static zend_uint vld_get_special_flags(zend_op *op, zend_uint base_address)
{
	zend_uint flags = 0;

	switch (op->opcode) {
		case ZEND_FE_RESET:
			flags = ALL_USED;
#if (PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1)
			flags |= OP2_OPNUM;
#elif (PHP_MAJOR_VERSION > 4) || (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION > 3) || (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION == 3 && PHP_RELEASE_VERSION >= 11)
			flags |= NOP2_OPNUM;
#endif
			break;

		case ZEND_ASSIGN_REF:
			flags = OP1_USED | OP2_USED;
			if (op->result.op_type != IS_UNUSED) {
				flags |= RES_USED;
			}
			break;

		case ZEND_DO_FCALL_BY_NAME:
		case ZEND_DO_FCALL:
			flags = ALL_USED | EXT_VAL;
			op->op2.op_type = IS_CONST;
			op->op2.u.constant.type = IS_LONG;
			break;

		case ZEND_INIT_FCALL_BY_NAME:
			flags = OP2_USED;
			if (op->op1.op_type != IS_UNUSED) {
				flags |= OP1_USED;
			}
			break;

		case ZEND_JMPZNZ:
			flags = OP1_USED | OP2_USED | EXT_VAL;
			op->result = op->op1;
			op->op2.op_type = VLD_IS_OPNUM;
			break;

#if (PHP_MAJOR_VERSION < 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 1)
		case ZEND_JMP_NO_CTOR:
			flags = OP2_USED;
			if (op->op1.op_type != IS_UNUSED) {
				flags |= OP1_USED;
			}
			op->op2.op_type = VLD_IS_OPNUM;
			break;
#endif

#ifdef ZEND_ENGINE_2
		case ZEND_FETCH_CLASS:
			flags = RES_USED|OP2_USED;
			op->result.op_type = VLD_IS_CLASS;
			break;
#endif

		case ZEND_NEW:
			flags = RES_USED|OP1_USED;
#ifdef ZEND_ENGINE_2
			op->op1.op_type = VLD_IS_CLASS;
#endif
			break;
	}

	return flags;
}

#define NUM_KNOWN_OPCODES (sizeof(opcodes)/sizeof(opcodes[0]))

void vld_dump_op(int nr, zend_op * op_ptr, zend_uint base_address, int notdead TSRMLS_DC)
{
	static uint last_lineno = -1;
	int print_sep = 0, len;
	char *fetch_type = "";
	zend_uint flags;
	zend_op op = op_ptr[nr];
	
	if (op.opcode >= NUM_KNOWN_OPCODES) {
		flags = ALL_USED;
	} else {
		flags = opcodes[op.opcode].flags;
	}

	if (op.lineno == 0)
		return;

	if (flags == SPECIAL) {
		flags = vld_get_special_flags(&op, base_address);
	} else {
		if (flags & OP1_OPLINE) {
			op.op1.op_type = VLD_IS_OPLINE;
		}
		if (flags & OP2_OPLINE) {
			op.op2.op_type = VLD_IS_OPLINE;
		}
	}
	if (flags & OP1_OPNUM) {
		op.op1.op_type = VLD_IS_OPNUM;
	}
	if (flags & OP2_OPNUM) {
		op.op2.op_type = VLD_IS_OPNUM;
	}

	if (flags & OP_FETCH) {
#ifdef ZEND_ENGINE_2
		switch (op.op2.u.EA.type) {
			case ZEND_FETCH_GLOBAL:
				fetch_type = "global";
				break;
			case ZEND_FETCH_LOCAL:
				fetch_type = "local";
				break;
			case ZEND_FETCH_STATIC:
				fetch_type = "static";
				break;
			case ZEND_FETCH_STATIC_MEMBER:
				fetch_type = "static member";
				break;
			default:
				fetch_type = "unknown";
				break;
		}
#else 
		if (op.op2.u.fetch_type == ZEND_FETCH_GLOBAL) {
			fetch_type = "global";
		} else if (op.op2.u.fetch_type == ZEND_FETCH_STATIC) {
			fetch_type = "static";
		}
#endif
	}

	if (op.lineno == last_lineno) {
		fprintf(stderr, "     ");
	} else {
		fprintf(stderr, "%4d ", op.lineno);
		last_lineno = op.lineno;
	}

	if (op.opcode >= NUM_KNOWN_OPCODES) {
		fprintf(stderr, "%5d%c <%03d>%-23s %-14s ", nr, notdead ? ' ' : '*', op.opcode, "", fetch_type);
	} else {
		fprintf(stderr, "%5d%c %-28s %-14s ", nr, notdead ? ' ' : '*', opcodes[op.opcode].name, fetch_type);
	}

	if (flags & EXT_VAL) {
		fprintf(stderr, "%3ld  ", op.extended_value);
	} else {
		fprintf(stderr, "     ");
	}

	if ((flags & RES_USED) && !(op.result.u.EA.type & EXT_TYPE_UNUSED)) {
		VLD_PRINT(3, " RES[ ");
		len = vld_dump_znode (NULL, op.result, base_address TSRMLS_CC);
		VLD_PRINT(3, " ]");
		fprintf(stderr, "%*s", 8-len, " ");
	} else {
		fprintf(stderr, "        ");
	}
	if (flags & OP1_USED) {
		VLD_PRINT(3, " OP1[ ");
		vld_dump_znode (&print_sep, op.op1, base_address TSRMLS_CC);
		VLD_PRINT(3, " ]");
	}		
	if (flags & OP2_USED) {
		VLD_PRINT(3, " OP2[ ");
		if (flags & OP2_INCLUDE) {
			if (VLD_G(verbosity) < 3 && print_sep) {
				fprintf(stderr, ", ");
			}
			switch (Z_LVAL(op.op2.u.constant)) {
				case ZEND_INCLUDE_ONCE:
					fprintf(stderr, "INCLUDE_ONCE");
					break;
				case ZEND_REQUIRE_ONCE:
					fprintf(stderr, "REQUIRE_ONCE");
					break;
				case ZEND_INCLUDE:
					fprintf(stderr, "INCLUDE");
					break;
				case ZEND_REQUIRE:
					fprintf(stderr, "REQUIRE");
					break;
				case ZEND_EVAL:
					fprintf(stderr, "EVAL");
					break;
				default:
					fprintf(stderr, "!!ERROR!!");
					break;
			}
		} else {
			vld_dump_znode (&print_sep, op.op2, base_address TSRMLS_CC);
		}
		VLD_PRINT(3, " ]");
	}
	if (flags & NOP2_OPNUM) {
		zend_op next_op = op_ptr[nr+1];
		next_op.op2.op_type = VLD_IS_OPNUM;
		vld_dump_znode (&print_sep, next_op.op2, base_address TSRMLS_CC);
	}
	fprintf (stderr, "\n");
}

void vld_analyse_branch(zend_op_array *opa, unsigned int position, vld_set *set TSRMLS_DC);

void vld_dump_oparray(zend_op_array *opa TSRMLS_DC)
{
	unsigned int i;
	vld_set *set;
	zend_uint base_address = (zend_uint) &(opa->opcodes[0]);

	set = vld_set_create(opa->size);
	vld_analyse_branch(opa, 0, set TSRMLS_CC);

	fprintf (stderr, "filename:       %s\n", opa->filename);
	fprintf (stderr, "function name:  %s\n", ZSTRCP(opa->function_name));
	fprintf (stderr, "number of ops:  %d\n", opa->last);
#ifdef IS_CV /* PHP >= 5.1 */
	fprintf (stderr, "compiled vars:  ");
	for (i = 0; i < opa->last_var; i++) {
		fprintf (stderr, "!%d = $%s%s", i, ZSTRCP(opa->vars[i].name), ((i + 1) == opa->last_var) ? "\n" : ", ");
	}
	if (!opa->last_var) {
		fprintf(stderr, "none\n");
	}
#endif

	fprintf(stderr, "line     #  op                           fetch          ext  return  operands\n");
	fprintf(stderr, "-------------------------------------------------------------------------------\n");
	for (i = 0; i < opa->last; i++) {
		vld_dump_op(i, opa->opcodes, base_address, vld_set_in(set, i) TSRMLS_CC);
	}
	fprintf(stderr, "\n");
	vld_set_free(set);
}

void opt_set_nop (zend_op_array *opa, int nr)
{
	opa->opcodes[nr].opcode = ZEND_NOP;
}

zend_brk_cont_element* vld_find_brk_cont(zval *nest_levels_zval, int array_offset, zend_op_array *op_array)
{
	int nest_levels;
	zend_brk_cont_element *jmp_to;

	nest_levels = nest_levels_zval->value.lval;

	do {
		jmp_to = &op_array->brk_cont_array[array_offset];
		array_offset = jmp_to->parent;
	} while (--nest_levels > 0);
	return jmp_to;
}

int vld_find_jump(zend_op_array *opa, unsigned int position, unsigned int *jmp1, unsigned int *jmp2)
{
	zend_uint base_address = (zend_uint) &(opa->opcodes[0]);

	zend_op opcode = opa->opcodes[position];
	if (opcode.opcode == ZEND_JMP) {
#ifdef ZEND_ENGINE_2
		*jmp1 = (opcode.op1.u.opline_num - base_address) / sizeof(zend_op);
#else
		*jmp1 = opcode.op1.u.opline_num;
#endif
		return 1;
	} else if (
		opcode.opcode == ZEND_JMPZ ||
		opcode.opcode == ZEND_JMPNZ ||
		opcode.opcode == ZEND_JMPZ_EX ||
		opcode.opcode == ZEND_JMPNZ_EX
	) {
		*jmp1 = position + 1;
#ifdef ZEND_ENGINE_2
		*jmp2 = (opcode.op2.u.opline_num - base_address) / sizeof(zend_op);
#else
		*jmp2 = opcode.op1.u.opline_num;
#endif
		return 1;
	} else if (opcode.opcode == ZEND_JMPZNZ) {
		*jmp1 = opcode.op2.u.opline_num;
		*jmp2 = opcode.extended_value;
		return 1;
	} else if (opcode.opcode == ZEND_BRK || opcode.opcode == ZEND_CONT) {
		zend_brk_cont_element *el;

		if (opcode.op2.op_type == IS_CONST) {
			el = vld_find_brk_cont(&opcode.op2.u.constant, opcode.op1.u.opline_num, opa);
			*jmp1 = opcode.opcode == ZEND_BRK ? el->brk : el->cont;
			return 1;
		}
	} else if (opcode.opcode == ZEND_FE_RESET || opcode.opcode == ZEND_FE_FETCH) {
		*jmp1 = position + 1;
		*jmp2 = opcode.op2.u.opline_num;
		return 1;
	}
	return 0;
}

void vld_analyse_branch(zend_op_array *opa, unsigned int position, vld_set *set TSRMLS_DC)
{
	int jump_pos1 = -1;
	int jump_pos2 = -1;

	VLD_PRINT(1, "Branch analysis from position: %d\n", position);
	/* First we see if the branch has been visited, if so we bail out. */
	if (vld_set_in(set, position)) {
		return;
	}
	/* Loop over the opcodes until the end of the array, or until a jump point has been found */
	VLD_PRINT(2, "Add %d\n", position);
	vld_set_add(set, position);
	while (position < opa->size - 1) {

		/* See if we have a jump instruction */
		if (vld_find_jump(opa, position, &jump_pos1, &jump_pos2)) {
			VLD_PRINT(1, "Jump found. Position 1 = %d", jump_pos1);
			if (jump_pos2 != -1) {
				VLD_PRINT(1, ", Position 2 = %d\n", jump_pos2);
			} else {
				VLD_PRINT(1, "\n");
			}
			vld_analyse_branch(opa, jump_pos1, set TSRMLS_CC);
			if (jump_pos2 != -1) {
				vld_analyse_branch(opa, jump_pos2, set TSRMLS_CC);
			}
			break;
		}
#ifdef ZEND_ENGINE_2
		/* See if we have a throw instruction */
		if (opa->opcodes[position].opcode == ZEND_THROW) {
			VLD_PRINT(1, "Throw found at %d\n", position);
			/* Now we need to go forward to the first
			 * zend_fetch_class/zend_catch combo */
			while (position < opa->size - 1) {
				if (opa->opcodes[position].opcode == ZEND_CATCH) {
					VLD_PRINT(1, "Found catch at %d\n", position);
					position--;
					break;
				}
				position++;
				VLD_PRINT(2, "Skipping %d\n", position);
			}
			position--;
		}
#endif
		/* See if we have an exit instruction */
		if (opa->opcodes[position].opcode == ZEND_EXIT) {
			VLD_PRINT(1, "Exit found\n");
			break;
		}
		/* See if we have a return instruction */
		if (opa->opcodes[position].opcode == ZEND_RETURN) {
			VLD_PRINT(1, "Return found\n");
			break;
		}

		position++;
		VLD_PRINT(2, "Add %d\n", position);
		vld_set_add(set, position);
	}
}

