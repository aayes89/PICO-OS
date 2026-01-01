#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "vm.h"
#include "sys.h"

// ---------- ESTADO GLOBAL ----------
Lexer lx;
MiniCVM vm;

// ---------- EMISIÓN DE BYTECODE ----------
static void emit(uint32_t op){ vm.code[vm.code_size++] = op; }
static void emit_i(int32_t v){ vm.code[vm.code_size++] = (uint32_t)v; }

static int emit_jmp(OpCode op){
  emit(op);
  int pos = vm.code_size;
  emit_i(0);
  return pos;
}
static void patch(int pos,int dst){
	vm.code[pos] = dst; 
}

// ---------- UTIL ----------
static void syntax(const char *msg){
  printf("[syntax] %s @ %d\n", msg, lx.pos);
  exit(1);
}

static int sym_lookup(const char *name){
  for(int i=vm.sym.count-1;i>=0;i--)
    if(!strcmp(vm.sym.table[i].name,name) &&
       vm.sym.table[i].scope_level<=vm.sym.scope_level)
      return i;
  return -1;
}

static int sym_add(const char *name, ValueType t, SymKind k){
  if(vm.sym.count >= MAX_SYM) syntax("symbol overflow");
  Symbol *s = &vm.sym.table[vm.sym.count++];
  strncpy(s->name,name,sizeof(s->name)-1);
  s->type = t;
  s->kind = k;
  s->scope_level = vm.sym.scope_level;
  if (k == SYM_VAR_GLOBAL) {
	  s->index = vm.sym.global_count++;
  } else if (k == SYM_VAR_LOCAL) {
	  s->index = vm.frames[vm.fp].local_count++;
  } else if (k == SYM_PARAM) {
	  s->index = vm.frames[vm.fp].param_count++;
  } else if (k == SYM_FUNC ) {
	  s->index = vm.func_count++;  // or native index — adjust as needed
  } else if(k == SYM_NATIVE){
	/* el índice ya viene asignado externamente */  
  } else {
	  s->index = 0;
  }
  return vm.sym.count-1;
}

// ---------- LEXER ----------
// operadores compuestos (two-character tokens)
static inline void try_match2(char c, char next_expected, Token token_if_match)
{
    if (c == lx.src[lx.pos] && lx.src[lx.pos + 1] == next_expected)
    {
        lx.pos += 2;
        lx.tok = token_if_match;
        return;   // ¡importante! salimos si coincidió
    }
    // si no coincide, simplemente seguimos (no hacemos nada)
}

static void next_tok(void)
{
    // Skip whitespace
    while (isspace((unsigned char)lx.src[lx.pos]))
        lx.pos++;

    char c = lx.src[lx.pos];
    if (!c) {
        lx.tok = TK_END;
        return;
    }

    // Numbers
    if (isdigit((unsigned char)c)) {
        lx.val = 0;
        while (isdigit((unsigned char)lx.src[lx.pos])) {
            lx.val = lx.val * 10 + (lx.src[lx.pos++] - '0');
        }
        lx.tok = TK_NUM;
        return;
    }

    // Identifiers & Keywords
    if (isalpha((unsigned char)c) || c == '_') {
        int i = 0;
        while ((isalnum((unsigned char)lx.src[lx.pos]) || lx.src[lx.pos] == '_') && i < 31)
            lx.id[i++] = lx.src[lx.pos++];
        lx.id[i] = '\0';

        #define KW(x, t) if (!strcmp(lx.id, x)) { lx.tok = t; return; }
        KW("if",       KW_IF)
        KW("else",     KW_ELSE)
        KW("while",    KW_WHILE)
        KW("for",      KW_FOR)
        KW("return",   KW_RETURN)
        KW("break",    KW_BREAK)
        KW("continue", KW_CONTINUE)
        KW("func",     KW_FUNC)
        KW("var",      KW_VAR)
        KW("call",     KW_CALL)
        KW("int8",     KW_INT8)
        KW("int16",    KW_INT16)
        KW("int32",    KW_INT32)
        KW("bool",     KW_BOOL)
        KW("string",   KW_STRING)
        #undef KW

        lx.tok = TK_ID;
        return;
    }

    // String literal
    if (c == '"') {
        lx.pos++;  // skip opening "
        int i = 0;
        while (lx.src[lx.pos] && lx.src[lx.pos] != '"' && i < MAX_STRING - 1)
            lx.str[i++] = lx.src[lx.pos++];
        lx.str[i] = '\0';

        if (lx.src[lx.pos] == '"')
            lx.pos++;  // skip closing "
        // else → unterminated string (you could add error here)

        lx.tok = TK_STRING;
        return;
    }

    // Compound operators (longest match first)
    try_match2('+', '+', TK_INC);
    try_match2('-', '-', TK_DEC);
    try_match2('*', '=', TK_MUL_ASSIGN);
    try_match2('/', '=', TK_DIV_ASSIGN);
    try_match2('%', '=', TK_MOD_ASSIGN);
    try_match2('=', '=', TK_EQ);
    try_match2('!', '=', TK_NE);
    try_match2('<', '=', TK_LE);
    try_match2('>', '=', TK_GE);
    try_match2('&', '&', TK_AND);
    try_match2('|', '|', TK_OR);
	try_match2('+', '=', TK_ADD_ASSIGN);      // +=
	try_match2('-', '=', TK_SUB_ASSIGN);      // -=
	try_match2('<', '<', TK_SHL);             // <<
	try_match2('>', '>', TK_SHR);             // >>
	try_match2('&', '=', TK_AND_ASSIGN);      // &=
	try_match2('|', '=', TK_OR_ASSIGN);       // |=
	try_match2('^', '=', TK_XOR_ASSIGN);      // ^=

    // Single character tokens
    lx.pos++;
    switch (c) {
        case '+': lx.tok = TK_PLUS;      break;
        case '-': lx.tok = TK_MINUS;     break;
        case '*': lx.tok = TK_MUL;       break;
        case '/': lx.tok = TK_DIV;       break;
        case '%': lx.tok = TK_MOD;       break;
        case '<': lx.tok = TK_LT;        break;
        case '>': lx.tok = TK_GT;        break;
        case '!': lx.tok = TK_NOT;       break;
        case '=': lx.tok = TK_ASSIGN;    break;
        case '(': lx.tok = TK_LP;        break;
        case ')': lx.tok = TK_RP;        break;
        case '{': lx.tok = TK_LC;        break;
        case '}': lx.tok = TK_RC;        break;
        case '[': lx.tok = TK_LB;        break;
        case ']': lx.tok = TK_RB;        break;
        case ',': lx.tok = TK_COMMA;     break;
        case ';': lx.tok = TK_SEMI;      break;
        default:
            // Unknown character → you should report error
            // For now we just set TK_END (maybe add syntax error later)
            lx.tok = TK_END;
            break;
    }
}

// ---------- EXPRESIONES ----------
static void expr();
static void unary() {
  if (lx.tok == TK_MINUS || lx.tok == TK_NOT) {
    Token op = lx.tok;
    next_tok();
    unary();
    emit(op == TK_MINUS ? OP_NEG : OP_NOT);
    return;
  }
  if (lx.tok == TK_INC || lx.tok == TK_DEC) {
    Token op = lx.tok;
    next_tok();
    if (lx.tok != TK_ID) syntax("id expected after inc/dec");
    int si = sym_lookup(lx.id);
    if (si < 0) syntax("unknown id");
    emit(OP_PUSH_VAR);
    emit_i(vm.sym.table[si].index);
    emit(OP_PUSH_CONST);
    emit_i(1);
    emit(op == TK_INC ? OP_ADD : OP_SUB);
    emit(OP_STORE_VAR);
    emit_i(vm.sym.table[si].index);
    next_tok();
    return;
  }
  // Factor with function call support
  if (lx.tok == TK_ID) {
    char name[32];
    strncpy(name, lx.id, 31);
    next_tok();
    int si = sym_lookup(name);
    if (si < 0) syntax("unknown id");
    Symbol *s = &vm.sym.table[si];
    if (lx.tok == TK_LP) {  // Function or native call
      next_tok();
      int argc = 0;
      while (lx.tok != TK_RP) {
        expr();
        argc++;
        if (lx.tok == TK_COMMA) next_tok();
      }
      next_tok();
      if (s->kind == SYM_FUNC) {
        if (argc != vm.funcs[s->index].param_count) syntax("arg mismatch");
        emit(OP_CALL);
        emit_i(s->index);
      } else if (s->kind == SYM_NATIVE) {
        emit(OP_NATIVE_CALL);
        emit_i(s->index);
        emit_i(argc);
      } else {
        syntax("not callable");
      }
      return;
    } else {  // Variable
      emit(OP_PUSH_VAR);
      emit_i(s->index | (s->kind == SYM_VAR_LOCAL ? 0x80000000 : 0));
      return;
    }
  }
  if (lx.tok == TK_NUM) {
    emit(OP_PUSH_CONST);
    emit_i(lx.val);
    next_tok();
    return;
  }
  if (lx.tok == TK_STRING) {
    int sid = vm.string_count++;
    if (vm.string_count >= MAX_STR_POOL) syntax("string pool overflow");
    vm.string_pool[sid] = strdup(lx.str);
    emit(OP_PUSH_CONST);
    emit_i(sid | (1 << 30));  // Tag as string
    next_tok();
    return;
  }
  if (lx.tok == TK_LP) {
    next_tok();
    expr();
    if (lx.tok != TK_RP) syntax("expected )");
    next_tok();
    return;
  }
  syntax("bad factor");
}

static void term(){
  unary();
  while(lx.tok==TK_MUL||lx.tok==TK_DIV||lx.tok==TK_MOD){
    Token op = lx.tok;
    next_tok();
    unary();
    emit(op==TK_MUL?OP_MUL:op==TK_DIV?OP_DIV:OP_MOD);
  }
}

static void additive(){
  term();
  while(lx.tok==TK_PLUS||lx.tok==TK_MINUS){
    Token op = lx.tok;
    next_tok();
    term();
    emit(op==TK_PLUS?OP_ADD:OP_SUB);
  }
}

static void relational(){
  additive();
  while(lx.tok==TK_LT||lx.tok==TK_LE||lx.tok==TK_GT||lx.tok==TK_GE){
    Token op=lx.tok; next_tok();
    additive();
    emit(op == TK_LT ? OP_LT : op == TK_LE ? OP_LE : op == TK_GT ? OP_GT : OP_GE);
  }
}

static void equality(){
  relational();
  while(lx.tok==TK_EQ||lx.tok==TK_NE){
    Token op=lx.tok; next_tok();
    relational();
    emit(op==TK_EQ?OP_EQ:OP_NE);
  }
}

static void logic_and(){
  equality();
  while(lx.tok==TK_AND){
    int p = emit_jmp(OP_JMP_FALSE);
    equality();
    patch(p, vm.code_size);
  }
}

static void expr(){
  logic_and();
  while(lx.tok==TK_OR){
	next_tok();  
    int p = emit_jmp(OP_JMP_TRUE);
    logic_and();
    patch(p, vm.code_size);
  }
}

// ---------- BLOQUES Y SCOPE ----------
static void enter_scope(){ vm.sym.scope_level++; }
static void leave_scope(){
  while(vm.sym.count>0 &&
        vm.sym.table[vm.sym.count-1].scope_level==vm.sym.scope_level)
    vm.sym.count--;
  vm.sym.scope_level--;
}

static void stmt();

static void block(){
  if(lx.tok!=TK_LC) syntax("expected {");
  next_tok();
  enter_scope();
  while(lx.tok!=TK_RC && lx.tok!=TK_END)
    stmt();
  leave_scope();
  if(lx.tok==TK_RC) next_tok();
}

// ---------- DECLARACIONES ----------
static ValueType parse_type(){
  switch(lx.tok){
    case KW_INT8:  next_tok(); return T_I8;
    case KW_INT16: next_tok(); return T_I16;
    case KW_INT32: next_tok(); return T_I32;
    case KW_BOOL:  next_tok(); return T_BOOL;
    case KW_STRING:next_tok(); return T_STRING;
    default: syntax("type expected");
  }
  return T_VOID;
}

static void var_decl(){
  ValueType t = parse_type();
  if(lx.tok!=TK_ID) syntax("id expected");
  char name[32]; 
  strncpy(name,lx.id,31); 
  next_tok();
  int si = sym_add(name, t,vm.sym.scope_level == 0 ? SYM_VAR_GLOBAL : SYM_VAR_LOCAL);
  
  if(lx.tok==TK_ASSIGN){
    next_tok();
    expr();
    emit(OP_STORE_VAR);
	emit_i(vm.sym.table[si].index | (vm.sym.table[si].kind == SYM_VAR_LOCAL ? 0x80000000 : 0));
  }
  if(lx.tok==TK_SEMI) next_tok(); else syntax(";");
}

static void func_decl() {
  next_tok(); // consume func
  ValueType ret_type = T_VOID;  // Default void
  if (lx.tok == KW_INT32 || lx.tok == KW_BOOL) {  // Optional return type
    ret_type = parse_type();
  }
  if (lx.tok != TK_ID) syntax("func name");
  char fname[32]; 
  strncpy(fname, lx.id, 31); 
  next_tok();
  
  int fi = vm.func_count;
  sym_add(fname, ret_type, SYM_FUNC);
  Function *f = &vm.funcs[fi];
  strncpy(f->name, fname, sizeof(f->name) - 1);
  f->code_start = vm.code_size;
  f->ret_type = ret_type;

  if (lx.tok != TK_LP) syntax("(");
  next_tok();
  vm.fp++;  // Push new frame for params/locals
  vm.frames[vm.fp].local_count = 0;
  vm.frames[vm.fp].param_count = 0;
  vm.frames[vm.fp].func_index = fi;
  
  enter_scope();
  int argc = 0;
  while (lx.tok != TK_RP) {
    ValueType t = parse_type();
    if (lx.tok != TK_ID) syntax("param id");
    sym_add(lx.id, t, SYM_PARAM);
    argc++;
    next_tok();
    if (lx.tok == TK_COMMA) next_tok();
  }
  f->param_count = argc;
  next_tok();  // Consume )
  
  block();
  leave_scope();
  vm.fp--;
  
  emit(OP_RET);
}

// ---------- STATEMENTS ----------
static void while_stmt(){
    next_tok(); // consume while
    int loop_start = vm.code_size;

    expr();
    int jfalse = emit_jmp(OP_JMP_FALSE);

    Frame *f = &vm.frames[vm.fp];
    int prev_loop_start = f->loop_start;
    int prev_loop_end   = f->loop_end;

    f->loop_start = loop_start;
    f->loop_end   = 0;       // temporal, se parchea después
    int break_stack[MAX_STACK];  // para múltiples breaks
    int break_count = 0;

    // bloque del loop
    while(lx.tok != TK_END && lx.tok != TK_RC){
        if(lx.tok == KW_BREAK){
            next_tok(); if(lx.tok == TK_SEMI) next_tok();
            int br_pos = emit_jmp(OP_JMP);
            break_stack[break_count++] = br_pos;
        } else if(lx.tok == KW_CONTINUE){
            next_tok(); if(lx.tok == TK_SEMI) next_tok();
            emit(OP_JMP);
            emit_i(loop_start);
        } else {
            stmt();
        }
    }

    int loop_end = vm.code_size;
    patch(jfalse, loop_end);
    for(int i=0;i<break_count;i++) patch(break_stack[i], loop_end);

    // jump al inicio del loop
    emit(OP_JMP); emit_i(loop_start);

    f->loop_start = prev_loop_start;
    f->loop_end   = prev_loop_end;
}


static void if_stmt(){
  next_tok();
  expr();
  int jf = emit_jmp(OP_JMP_FALSE);
  block();
  if(lx.tok==KW_ELSE){
    int je = emit_jmp(OP_JMP);
    patch(jf, vm.code_size);
    next_tok();
    block();
    patch(je, vm.code_size);
  } else {
    patch(jf, vm.code_size);
  }
}

static void return_stmt(){
  next_tok();
  if(lx.tok!=TK_SEMI){
    expr();
  }else{
	  //emit(OP_PUSH_CONST);
	  //emit_i(0);
  }
  emit(OP_RET);
  if(lx.tok==TK_SEMI) next_tok();
}

static void assign_or_expr_stmt(){
  if(lx.tok!=TK_ID){ expr(); if(lx.tok==TK_SEMI) next_tok(); return; }
  char name[32]; strncpy(name,lx.id,31);
  next_tok();
  int si = sym_lookup(name);
  if(si<0) syntax("unknown id");
  if(lx.tok==TK_ASSIGN){
    next_tok();
    expr();
    emit(OP_STORE_VAR);
    emit_i(vm.sym.table[si].index | (vm.sym.table[si].kind == SYM_VAR_LOCAL ? 0x80000000 : 0));
  }
  if(lx.tok==TK_SEMI) next_tok();
}

static void stmt(){
  switch(lx.tok){
    case KW_IF: if_stmt(); return;
    case KW_WHILE: while_stmt(); return;
    case KW_RETURN: return_stmt(); return;
	case KW_BREAK: {
            if(vm.fp < 0 || vm.frames[vm.fp].loop_end == 0) syntax("break outside loop");
            emit(OP_JMP);
            emit_i(vm.frames[vm.fp].loop_end);
            next_tok(); if(lx.tok==TK_SEMI) next_tok();
            return;
        }
        case KW_CONTINUE: {
            if(vm.fp < 0 || vm.frames[vm.fp].loop_start == 0) syntax("continue outside loop");
            emit(OP_JMP);
            emit_i(vm.frames[vm.fp].loop_start);
            next_tok(); if(lx.tok==TK_SEMI) next_tok();
            return;
        }
    case KW_INT8: 
	case KW_INT16: 
	case KW_INT32:
    case KW_BOOL: 
	case KW_STRING: var_decl(); return;
    case KW_FUNC: func_decl(); return;
    case TK_LC: block(); return;
	case TK_ID:
	  // Check if assign or expr (call)
      /*if (lx.src[lx.pos] == '=') {
        stmt();
      } else {*/
        expr();
        if (lx.tok == TK_SEMI) next_tok();
      //}
      return;
    default: syntax("bad stmt"); return;
  }
}

// ---------- EJECUCIÓN ----------
static int32_t value_to_i32(Value v) {
  switch (v.type) {
    case T_I8: return v.i8;
	case T_I16: return v.i16;
	case T_I32: return v.i32;
    case T_BOOL: return v.boolean;
    default: return 0;  // Error handling TBD
  }
}

static Value i32_to_value(int32_t i, ValueType t) {
    Value v;
    v.type = t;
    switch(t) {
        case T_I8:   v.i8 = (int8_t)i; break;
        case T_I16:  v.i16 = (int16_t)i; break;
        case T_I32:  v.i32 = i; break;
        case T_BOOL: v.boolean = i ? 1 : 0; break;
        default: v.i32 = i; break; // fallback
    }
    return v;
}

static int native_count = sizeof(native_table)/sizeof(NativeEntry);
  
void register_native(){ 
 for (int i = 0; i < native_count; i++) {
	 if(vm.sym.count >= MAX_SYM) return;
    Symbol *s = &vm.sym.table[vm.sym.count++];	
    strncpy(s->name, native_table[i].name, 31);
	s->name[31] = '\0';
    s->type = T_VOID;  // Assume void return for now
    s->kind = SYM_NATIVE;
    s->index = i;
    s->scope_level = 0;
  }
}

void minic_run(const char *src){
  memset(&vm,0,sizeof(vm));
  register_native();
  lx.src = src; lx.pos=0;
  next_tok();

  vm.fp = 0;  // Root frame
  vm.frames[0].local_count = 0;
  vm.frames[0].param_count = 0;
  
  while(lx.tok!=TK_END) stmt();

  // Find and call 'main' if exists
  int main_idx = sym_lookup("main");
  if (main_idx >= 0 && vm.sym.table[main_idx].kind == SYM_FUNC) {
    emit(OP_CALL);
    emit_i(vm.sym.table[main_idx].index);
  }

  vm.ip = 0;
  vm.sp = 0;
  vm.fp = -1;

  while(vm.ip < vm.code_size){
    OpCode op = (OpCode)vm.code[vm.ip++];
    switch(op){
      case OP_NOP: break;
      case OP_PUSH_CONST:{
        int32_t val = (int32_t)vm.code[vm.ip++];
        Value *dest = &vm.stack[vm.sp++];
        if (val & (1 << 30)) {  // String tag
          int sid = val & ~(1 << 30);
          dest->type = T_STRING;
          strncpy(dest->str, vm.string_pool[sid], MAX_STRING - 1);
        } else {
          dest->type = T_I32;
          dest->i32 = val;
        }
        break;
      }
	  case OP_PUSH_VAR: {
        int idx = (int32_t)vm.code[vm.ip++];
        Value *src;
        if (idx & 0x80000000) {  // Local
          idx &= ~0x80000000;
          src = &vm.frames[vm.fp].locals[idx];
        } else {  // Global
          src = &vm.globals[idx];
        }
        vm.stack[vm.sp++] = *src;
        break;
      }
      case OP_STORE_VAR: {
        int idx = (int32_t)vm.code[vm.ip++];
        Value *dest;
        if (idx & 0x80000000) {  // Local
          idx &= ~0x80000000;
          dest = &vm.frames[vm.fp].locals[idx];
        } else {  // Global
          dest = &vm.globals[idx];
        }
        *dest = vm.stack[--vm.sp];
        break;
      }
      case OP_ADD: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(value_to_i32(a) + value_to_i32(b), a.type);
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_SUB: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(value_to_i32(a) - value_to_i32(b), a.type);
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_MUL: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(value_to_i32(a) * value_to_i32(b), a.type);
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_DIV: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(value_to_i32(a) / value_to_i32(b), a.type);
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_MOD: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(value_to_i32(a) % value_to_i32(b), a.type);
        vm.stack[vm.sp++] = res;
        break;
      }
	  case OP_EQ: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res;
        res.type = T_BOOL;
        res.boolean = (value_to_i32(a) == value_to_i32(b));
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_NE: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res;
        res.type = T_BOOL;
        res.boolean = (value_to_i32(a) != value_to_i32(b));
        vm.stack[vm.sp++] = res;
        break;
      }
	  case OP_LT: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res;
        res.type = T_BOOL;
        res.boolean = (value_to_i32(a) < value_to_i32(b));
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_GT: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res;
        res.type = T_BOOL;
        res.boolean = (value_to_i32(a) > value_to_i32(b));
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_LE: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res;
        res.type = T_BOOL;
        res.boolean = (value_to_i32(a) <= value_to_i32(b));
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_GE: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res;
        res.type = T_BOOL;
        res.boolean = (value_to_i32(a) >= value_to_i32(b));
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_NOT: {
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(!value_to_i32(a), a.type);
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_NEG: {
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(-value_to_i32(a), a.type);
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_AND: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(value_to_i32(a) && value_to_i32(b), T_BOOL);
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_OR: {
        Value b = vm.stack[--vm.sp];
        Value a = vm.stack[--vm.sp];
        Value res = i32_to_value(value_to_i32(a) || value_to_i32(b), T_BOOL);
        vm.stack[vm.sp++] = res;
        break;
      }
      case OP_JMP: vm.ip = (int)vm.code[vm.ip]; break;
      case OP_JMP_FALSE: {
        int tgt = (int)vm.code[vm.ip++];
        Value cond = vm.stack[--vm.sp];
        if (!value_to_i32(cond)) vm.ip = tgt;
        break;
      }
      case OP_JMP_TRUE: {
        int tgt = (int)vm.code[vm.ip++];
        Value cond = vm.stack[--vm.sp];
        if (value_to_i32(cond)) vm.ip = tgt;
        break;
      }
      case OP_CALL: {
        int func_idx = (int)vm.code[vm.ip++];
        Function *f = &vm.funcs[func_idx];
        vm.fp++;
        vm.frames[vm.fp].ret_ip = vm.ip;
        vm.frames[vm.fp].ret_sp = vm.sp;  // Pop params after return
		//vm.sp -= f->param_count;
        vm.frames[vm.fp].func_index = func_idx;
        // Move params to locals (params are first locals)
        for (int i = 0; i < f->param_count; i++) {
          vm.frames[vm.fp].locals[i] = vm.stack[vm.sp - f->param_count + i];
        }
        vm.sp -= f->param_count;
        vm.frames[vm.fp].local_count = f->param_count;  // Locals start after params
        vm.ip = f->code_start;
        break;
      }
      case OP_RET: {
        Value ret_val = vm.stack[--vm.sp];  // Assume return value on stack
        vm.sp = vm.frames[vm.fp].ret_sp;
        vm.ip = vm.frames[vm.fp].ret_ip;
        vm.stack[vm.sp++] = ret_val;  // Push return value
        vm.fp--;
        break;
      }
      case OP_NATIVE_CALL: {
        int native_idx = (int)vm.code[vm.ip++];
        int argc = (int)vm.code[vm.ip++];
        NativeEntry *ne = &native_table[native_idx];
        int32_t args[MAX_PARAM];
        for (int i = argc - 1; i >= 0; i--) {
          Value arg = vm.stack[--vm.sp];
          if (arg.type == T_STRING) {
            // For strings, pass tagged SID
            for (int j = 0; j < vm.string_count; j++) {
              if (!strcmp(vm.string_pool[j], arg.str)) {
                args[i] = j | (1 << 30);
                break;
              }
            }
          } else {
            args[i] = value_to_i32(arg);
          }
        }
        int32_t ret = ne->fn(args, argc);
        vm.stack[vm.sp++] = i32_to_value(ret, T_I32);
        break;
      }
      
      case OP_ARR_LOAD: {
		Value idx_val = vm.stack[--vm.sp];
		Value arr_val = vm.stack[--vm.sp];
		if(arr_val.type != T_ARRAY) {
			syntax("not an array"); 
			break;
		}
		int idx = value_to_i32(idx_val);
		if(idx < 0 || idx >= arr_val.arr.length){
			syntax("index out of bounds"); 
			break;
		}
		Value result;
		result.type = T_I32;
		result.i32 = arr_val.arr.array[idx];
		vm.stack[vm.sp++] = result;
		break;
	}
	case OP_ARR_STORE: {
		Value value_to_store = vm.stack[vm.sp];
		Value idx_val = vm.stack[--vm.sp];
		Value *arr_ref = &vm.stack[vm.sp - 1];
		
		if(arr_ref->type != T_ARRAY){
			syntax("not an array");
			break;
		}
		int idx = value_to_i32(idx_val);
		if(idx < 0 || idx >= arr_ref->arr.length){
			syntax("index out of bounds");
			break;
		}
		arr_ref->arr.array[idx] = value_to_i32(value_to_store);
		break;
	}

	// Stubs for break, continue - implement as needed
    case OP_BREAK:
    case OP_CONTINUE:
        break;
      default: printf("Unknown opcode %d\n", op); return;
    }
  }
}