#pragma once
#include <stdint.h>
#include <stddef.h>

#define MAX_STACK     32
#define MAX_VARS      32
#define MAX_CODE      128
#define MAX_FUNCS     64
#define MAX_SCOPE     32
#define MAX_SYM       128
#define MAX_STRING    64
#define MAX_ARRAY     16
#define MAX_PARAM     8
#define MAX_STR_POOL  16
#define MAX_FRAMES    32

// ---------------- Tipos -----------------

typedef enum {
  T_VOID,
  T_I8,
  T_I16,
  T_I32,
  T_BOOL,
  T_STRING,
  T_ARRAY
} ValueType;

typedef struct {
  ValueType type;
  union {
    int8_t   i8;
    int16_t  i16;
    int32_t  i32;
    uint8_t  boolean;
    char     str[MAX_STRING];
    struct {
      int32_t array[MAX_ARRAY];
      int length;
    } arr;
  };
} Value;

typedef struct {
    Value *items;
    int length;
} Array;

// -------- Tokens --------
typedef enum {
  TK_END, TK_NUM, TK_ID, TK_STRING,
  TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_MOD, 
  TK_ADD, TK_SUB,
  TK_INC, TK_DEC, TK_NEG,
  TK_EQ, TK_NE, TK_LT, TK_LE, TK_GT, TK_GE,
  TK_AND, TK_OR, TK_NOT,
  TK_NAND, TK_NOR, TK_XOR, TK_SHL, TK_SHR,
  TK_ASSIGN, TK_ADD_ASSIGN, TK_SUB_ASSIGN, TK_MUL_ASSIGN, TK_DIV_ASSIGN, TK_MOD_ASSIGN, TK_AND_ASSIGN, TK_OR_ASSIGN, TK_XOR_ASSIGN,
  TK_LP, TK_RP,
  TK_LC, TK_RC,
  TK_LB, TK_RB,
  TK_COMMA, TK_SEMI,
  KW_IF, KW_ELSE, KW_WHILE, KW_FOR,
  KW_RETURN, KW_CONST,
  KW_BREAK, KW_CONTINUE,
  KW_INT8, KW_INT16, KW_INT32,
  KW_BOOL, KW_STRING,
  KW_VAR,
  KW_FUNC,
  KW_CALL
} Token;

// -------- Bytecode --------
typedef enum {
  OP_NOP,
  OP_PUSH_CONST,
  OP_PUSH_VAR,
  OP_STORE_VAR,
  OP_ARR_LOAD,
  OP_ARR_STORE,
  OP_NATIVE_CALL,
  OP_CALL,
  OP_RET,
  OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
  OP_XOR, OP_SHL, OP_SHR,
  OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE,
  OP_NOT, OP_NEG,
  OP_AND, OP_OR,
  OP_JMP,
  OP_JMP_FALSE,
  OP_JMP_TRUE,
  OP_BREAK,
  OP_CONTINUE
} OpCode;

// -------- Function Metadata --------
typedef struct {
  char name[32];
  int code_start;
  int param_count;
  ValueType ret_type;
} Function;

// -------- Lexer --------
typedef struct {
  const char *src;
  int pos;
  Token tok;
  int32_t val;
  char id[32];
  char str[MAX_STRING];
} Lexer;

// -------- Símbolos --------
typedef enum {
  SYM_VAR_GLOBAL,
  SYM_VAR_LOCAL,
  SYM_PARAM,
  SYM_FUNC,
  SYM_NATIVE
} SymKind;

typedef struct {
  char name[32];
  ValueType type;
  SymKind kind;
  int index; // vars/frame index
  int scope_level;
} Symbol;

typedef struct {
  Symbol table[MAX_SYM];
  int count;
  int scope_level;
  int global_count;
} SymTable;

// -------- Frame --------
typedef struct {
  Value locals[MAX_VARS];
  int local_count;
  int param_count;
  int ret_sp;
  int ret_ip;
  int func_index;
  int loop_start;
  int loop_end;
} Frame;

// -------- VM --------
typedef struct {
  Value globals[MAX_VARS];

  Value stack[MAX_STACK];
  int sp;

  uint32_t code[MAX_CODE];
  int code_size;
  int ip;

  Frame frames[MAX_FRAMES];
  int fp;

  Function funcs[MAX_FUNCS];
  int func_count;

  const char *string_pool[MAX_STR_POOL];
  int string_count;

  SymTable sym;
} MiniCVM;

extern Lexer lx;
extern MiniCVM vm;
