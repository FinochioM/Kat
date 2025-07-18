#ifndef kparser_h
#define kparser_h

#include "minimal_headers.h"
#include "klex.h"

/* Expression kinds */
typedef enum {
  KVOID,     /* no expression */
  KNIL,      /* nil constant */
  KTRUE,     /* true constant */
  KFALSE,    /* false constant */
  KINT,      /* integer constant */
  KFLT,      /* float constant */  
  KSTR,      /* string constant */
  KNAME,     /* identifier */
  KCALL,     /* function call */
  KBINOP,    /* binary operation */
  KUNOP      /* unary operation */
} ExpKind;

/* Expression descriptor */
typedef struct expdesc {
  ExpKind k;
  union {
    struct {  /* for binary/unary ops */
      struct expdesc *left, *right;
      int op;
    } binop;
    struct {  /* for function calls */
      struct expdesc *func;
      struct expdesc *args;
    } call;
    struct {  /* for constants */
      kat_Integer ival;
      kat_Number fval;
      TString *sval;
    } val;
    TString *name;  /* for identifiers */
  } u;
} expdesc;

/* AST node types */
typedef enum {
  AST_PROGRAM,
  AST_DECL,
  AST_STMT_ASSIGN,
  AST_STMT_IF,
  AST_STMT_FOR,
  AST_STMT_WHILE,
  AST_STMT_RETURN,
  AST_STMT_BLOCK,
  AST_EXPR,
  AST_PROC_DECL,
  AST_STRUCT_DECL
} ASTNodeType;

/* AST Node */
typedef struct ASTNode {
  ASTNodeType type;
  int line;
  union {
    struct {  /* declarations */
      TString *name;
      struct ASTNode *type_expr;
      struct ASTNode *init_expr;
      int is_constant;  /* :: vs : */
    } decl;
    struct {  /* statements */
      struct ASTNode *left;  /* for assignments */
      struct ASTNode *right;
      struct ASTNode *condition;  /* for if/while */
      struct ASTNode *body;
      struct ASTNode *else_body;
    } stmt;
    struct {  /* expressions */
      expdesc desc;
    } expr;
    struct {  /* procedure declarations */
      TString *name;
      struct ASTNode *params;
      struct ASTNode *return_type;
      struct ASTNode *body;
    } proc;
    struct {  /* struct declarations */
      TString *name;
      struct ASTNode *fields;
    } struct_decl;
  } u;
  struct ASTNode *next;  /* for lists */
} ASTNode;

/* Function state - simplified */
typedef struct FuncState {
  struct FuncState *prev;  /* enclosing function */
  struct LexState *ls;     /* lexical state */
  ASTNode *ast;            /* current AST being built */
  int level;               /* scope level */
} FuncState;

/* Parser functions */
LUAI_FUNC ASTNode *katY_parser (kat_State *K, ZIO *z, TString *source, int firstchar);
LUAI_FUNC void katY_freeAST (kat_State *K, ASTNode *node);

#endif