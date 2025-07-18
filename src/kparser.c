/*
** Kat Parser
** Based on Lua's lparser.c
*/

#include "minimal_headers.h"
#include "klex.h"
#include "kparser.h"

/* Maximum recursion depth */
#define MAXCCALLS    200

/* Forward declarations */
static void statement (LexState *ls);
static void expr (LexState *ls, expdesc *v);

/* Error handling */
static l_noret error_expected (LexState *ls, int token) {
  (void)token;
  katX_syntaxerror(ls, "expected token");
}

/* Test whether next token is 'c'; if so, skip it */
static int testnext (LexState *ls, int c) {
  if (ls->t.token == c) {
    katX_next(ls);
    return 1;
  }
  else return 0;
}

/* Check that next token is 'c' */
static void check (LexState *ls, int c) {
  if (ls->t.token != c)
    error_expected(ls, c);
}

/* Check that next token is 'c' and skip it */
static void checknext (LexState *ls, int c) {
  check(ls, c);
  katX_next(ls);
}

/* Check that current token is a name and return it */
static TString *str_checkname (LexState *ls) {
  TString *ts;
  check(ls, TK_NAME);
  ts = ls->t.seminfo.ts;
  katX_next(ls);
  return ts;
}

/* AST node creation */
static ASTNode *new_node (kat_State *K, ASTNodeType type) {
  (void)K;
  ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
  node->type = type;
  node->line = 0;
  node->next = NULL;
  return node;
}

/* Initialize expression descriptor */
static void init_exp (expdesc *e, ExpKind k) {
  e->k = k;
}

/* Expression parsing */
static void primaryexp (LexState *ls, expdesc *v) {
  switch (ls->t.token) {
    case TK_NAME: {
      init_exp(v, KNAME);
      v->u.name = str_checkname(ls);
      return;
    }
    case TK_NIL: {
      init_exp(v, KNIL);
      katX_next(ls);
      return;
    }
    case TK_TRUE: {
      init_exp(v, KTRUE);
      katX_next(ls);
      return;
    }
    case TK_FALSE: {
      init_exp(v, KFALSE);
      katX_next(ls);
      return;
    }
    case TK_INT: {
      init_exp(v, KINT);
      v->u.val.ival = ls->t.seminfo.i;
      katX_next(ls);
      return;
    }
    case TK_FLT: {
      init_exp(v, KFLT);
      v->u.val.fval = ls->t.seminfo.r;
      katX_next(ls);
      return;
    }
    case TK_STRING: {
      init_exp(v, KSTR);
      v->u.val.sval = ls->t.seminfo.ts;
      katX_next(ls);
      return;
    }
    case '(': {
      katX_next(ls);
      expr(ls, v);
      checknext(ls, ')');
      return;
    }
    default: {
      katX_syntaxerror(ls, "unexpected symbol");
    }
  }
}

/* Binary operators precedence (higher number = higher precedence) */
static const struct {
  lu_byte left;  /* left priority for each binary operator */
  lu_byte right; /* right priority */
} priority[] = {
  {10, 10}, {10, 10},           /* '+' '-' */
  {11, 11}, {11, 11},           /* '*' '/' */
  {14, 13},                     /* '^' (right associative) */
  {6, 6}, {6, 6}, {6, 6},       /* '==' '!=' '<' */
  {6, 6}, {6, 6}, {6, 6},       /* '<=' '>' '>=' */
  {5, 5},                       /* 'and' */
  {4, 4},                       /* 'or' */
  {3, 3},                       /* '..' (right associative) */
};

#define UNARY_PRIORITY  12  /* priority for unary operators */

/* Convert token to binary operator */
static int getbinopr (int op) {
  switch (op) {
    case '+': return 0;
    case '-': return 1;
    case '*': return 2;
    case '/': return 3;
    case '%': return 4;
    case TK_EQ: return 5;
    case TK_NE: return 6;
    case '<': return 7;
    case TK_LE: return 8;
    case '>': return 9;
    case TK_GE: return 10;
    case TK_AND_AND: return 11;
    case TK_OR_OR: return 12;
    default: return -1;  /* not a binary operator */
  }
}

/* Convert token to unary operator */
static int getunopr (int op) {
  switch (op) {
    case '!': return 0;
    case '-': return 1;
    case '+': return 2;
    default: return -1;  /* not a unary operator */
  }
}

/* Subexpression parsing */
static void subexpr (LexState *ls, expdesc *v, int limit) {
  int uop = getunopr(ls->t.token);
  if (uop != -1) {
    katX_next(ls);
    subexpr(ls, v, UNARY_PRIORITY);
    expdesc *operand = (expdesc *)malloc(sizeof(expdesc));
    *operand = *v;
    init_exp(v, KUNOP);
    v->u.binop.left = operand;
    v->u.binop.op = uop;
  }
  else {
    primaryexp(ls, v);
  }
  
  int op = getbinopr(ls->t.token);
  while (op != -1 && priority[op].left > limit) {
    expdesc v2;
    katX_next(ls);
    
    expdesc *left = (expdesc *)malloc(sizeof(expdesc));
    *left = *v;
    
    subexpr(ls, &v2, priority[op].right);
    expdesc *right = (expdesc *)malloc(sizeof(expdesc));
    *right = v2;
    
    init_exp(v, KBINOP);
    v->u.binop.left = left;
    v->u.binop.right = right;
    v->u.binop.op = op;
    
    op = getbinopr(ls->t.token);
  }
}

static void expr (LexState *ls, expdesc *v) {
  subexpr(ls, v, 0);
}

/* Parse function call */
static void funcargs (LexState *ls, expdesc *f) {
  expdesc args;
  
  switch (ls->t.token) {
    case '(': {
      katX_next(ls);
      if (ls->t.token == ')')
        init_exp(&args, KVOID);
      else {
        expr(ls, &args);
      }
      checknext(ls, ')');
      break;
    }
    default: {
      katX_syntaxerror(ls, "function arguments expected");
    }
  }
  
  expdesc *func = (expdesc *)malloc(sizeof(expdesc));
  expdesc *arglist = (expdesc *)malloc(sizeof(expdesc));
  *func = *f;
  *arglist = args;
  
  init_exp(f, KCALL);
  f->u.call.func = func;
  f->u.call.args = arglist;
}

/* Parse suffixed expression (calls, indexing) */
static void suffixedexp (LexState *ls, expdesc *v) {
  primaryexp(ls, v);
  for (;;) {
    switch (ls->t.token) {
      case '(': {  /* function call */
        funcargs(ls, v);
        break;
      }
      default: return;
    }
  }
}

/* Statement parsing */

/* Parse assignment statement */
static void assignment (LexState *ls, TString *name) {
  expdesc e;
  
  if (testnext(ls, TK_COLON_EQ)) {
    /* x := expr */
    expr(ls, &e);
    printf("Assignment: %.*s := <expr>\n", (int)name->len, name->contents);
  }
  else if (testnext(ls, ':')) {
    /* x : type = expr */
    expdesc type_expr;
    expr(ls, &type_expr);  /* parse type */
    checknext(ls, '=');
    expr(ls, &e);  /* parse value */
    printf("Typed assignment: %.*s : <type> = <expr>\n", (int)name->len, name->contents);
  }
  else if (testnext(ls, TK_COLON_COLON)) {
    /* x :: expr (constant) */
    expr(ls, &e);
    printf("Constant: %.*s :: <expr>\n", (int)name->len, name->contents);
  }
  else {
    katX_syntaxerror(ls, "expected ':=', ':', or '::'");
  }
}

/* Parse if statement */
static void ifstat (LexState *ls, int line) {
  (void)line;
  expdesc cond;
  katX_next(ls);
  expr(ls, &cond);
  checknext(ls, '{');
  printf("If statement with condition\n");
  
  while (ls->t.token != '}' && ls->t.token != TK_EOS) {
    statement(ls);
  }
  checknext(ls, '}');
  
  if (testnext(ls, TK_ELSE)) {
    if (ls->t.token == TK_IF) {
      ifstat(ls, ls->linenumber);
    }
    else {
      checknext(ls, '{');
      printf("Else body\n");
      while (ls->t.token != '}' && ls->t.token != TK_EOS) {
        statement(ls);
      }
      checknext(ls, '}');
    }
  }
}

/* Parse while statement */
static void whilestat (LexState *ls, int line) {
  (void)line;
  expdesc cond;
  katX_next(ls);
  expr(ls, &cond);
  checknext(ls, '{');
  printf("While statement with condition\n");
  
  while (ls->t.token != '}' && ls->t.token != TK_EOS) {
    statement(ls);
  }
  checknext(ls, '}');
}

/* Parse for statement */
static void forstat (LexState *ls, int line) {
  (void)line;
  katX_next(ls);
  
  TString *var = str_checkname(ls);
  checknext(ls, TK_IN);
  expdesc iter;
  expr(ls, &iter);
  checknext(ls, '{');
  
  printf("For loop: %.*s in <expr>\n", (int)var->len, var->contents);
  
  while (ls->t.token != '}' && ls->t.token != TK_EOS) {
    statement(ls);
  }
  checknext(ls, '}');
}

/* Parse return statement */
static void retstat (LexState *ls) {
  expdesc e;
  katX_next(ls);  /* skip RETURN */
  
  if (ls->t.token != ';' && ls->t.token != '}' && ls->t.token != TK_EOS) {
    expr(ls, &e);  /* parse return value */
    printf("Return statement with value\n");
  }
  else {
    printf("Return statement (void)\n");
  }
  testnext(ls, ';');  /* optional semicolon */
}

/* Parse procedure declaration */
static void procstat (LexState *ls, int line) {
  (void)line;
  TString *name;
  katX_next(ls);
  
  if (ls->t.token == TK_NAME) {
    name = str_checkname(ls);
    printf("Named procedure: %.*s\n", (int)name->len, name->contents);
  }
  
  checknext(ls, '(');
  checknext(ls, ')');
  
  if (testnext(ls, TK_ARROW)) {
    expdesc ret_type;
    expr(ls, &ret_type);
    printf("Return type specified\n");
  }
  
  checknext(ls, '{');
  printf("Procedure body:\n");
  
  while (ls->t.token != '}' && ls->t.token != TK_EOS) {
    statement(ls);
  }
  checknext(ls, '}');
}

/* Parse statement */
static void statement (LexState *ls) {
  int line = ls->linenumber;
  
  switch (ls->t.token) {
    case ';': {  /* empty statement */
      katX_next(ls);
      break;
    }
    case TK_IF: {
      ifstat(ls, line);
      break;
    }
    case TK_WHILE: {
      whilestat(ls, line);
      break;
    }
    case TK_FOR: {
      forstat(ls, line);
      break;
    }
    case TK_RETURN: {
      retstat(ls);
      break;
    }
    case TK_PROC: {
      procstat(ls, line);
      break;
    }
    case TK_NAME: {
      TString *name = str_checkname(ls);
      assignment(ls, name);
      break;
    }
    default: {
      /* Expression statement */
      expdesc e;
      expr(ls, &e);
      printf("Expression statement\n");
      break;
    }
  }
}

/* Parse statement list */
static void statlist (LexState *ls) {
  while (ls->t.token != TK_EOS) {
    if (ls->t.token == TK_RETURN) {
      statement(ls);
      return;  /* 'return' must be last statement */
    }
    statement(ls);
  }
}

/* Main parsing function */
ASTNode *katY_parser (kat_State *K, ZIO *z, TString *source, int firstchar) {
  LexState lexstate;
  FuncState funcstate;
  
  /* Initialize lexer */
  katX_setinput(K, &lexstate, z, source, firstchar);
  
  /* Initialize parser state */
  funcstate.prev = NULL;
  funcstate.ls = &lexstate;
  funcstate.level = 0;
  lexstate.fs = &funcstate;
  
  /* Create root AST node */
  ASTNode *root = new_node(K, AST_PROGRAM);
  funcstate.ast = root;
  
  katX_next(&lexstate);  /* read first token */
  printf("=== Parsing Kat source ===\n");
  statlist(&lexstate);  /* parse main body */
  check(&lexstate, TK_EOS);
  printf("=== Parsing complete ===\n");
  
  return root;
}

/* Free AST (simplified) */
void katY_freeAST (kat_State *K, ASTNode *node) {
  if (node) {
    katY_freeAST(K, node->next);
    free(node);
  }
}