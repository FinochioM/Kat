#include "minimal_headers.h"
#include "klex.h"

#define next(ls)	(ls->current = zgetc(ls->z))

#define currIsNewline(ls)	(ls->current == '\n' || ls->current == '\r')

/* ORDER RESERVED */
const char *const katX_tokens [] = {
    "if", "else", "for", "while", "break", "continue",
    "return", "proc", "struct", "enum", "union", "defer", "using",
    "import", "package", "true", "false", "nil", "in", "or_return",
    "or_else", "cast", "auto_cast", "transmute", "distinct", "opaque",
    "where", "switch", "case", "fallthrough", "map", "context",
    "::", ":=", "->", "..", "..<",
    "==", "!=", "<=", ">=", "&&", "||", "<<", ">>",
    "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};

#define save_and_next(ls) (save(ls, ls->current), next(ls))

static l_noret lexerror (LexState *ls, const char *msg, int token);

static void save (LexState *ls, int c) {
  Mbuffer *b = ls->buff;
  if (katZ_bufflen(b) + 1 > katZ_sizebuffer(b)) {
    size_t newsize;
    if (katZ_sizebuffer(b) >= MAX_SIZE/2)
      lexerror(ls, "lexical element too long", 0);
    newsize = katZ_sizebuffer(b) * 2;
    katZ_resizebuffer(ls->K, b, newsize);
  }
  b->buffer[katZ_bufflen(b)++] = cast_char(c);
}

void katX_init (kat_State *K) {
  int i;
  for (i=0; i<NUM_RESERVED; i++) {
    TString *ts = katS_new(K, katX_tokens[i]);
    ts->extra = cast_char(i+1);  /* reserved word */
  }
}

const char *katX_token2str (LexState *ls, int token) {
  if (token < FIRST_RESERVED) {  /* single-byte symbols? */
    static char single[4];
    if (lisprint(token)) {
      sprintf(single, "'%c'", token);
      return single;
    }
    else  /* control character */
      return "'<?>'";
  }
  else {
    const char *s = katX_tokens[token - FIRST_RESERVED];
    if (token < TK_EOS)  /* fixed format (symbols and reserved words)? */
      return s;
    else  /* names, strings, and numerals */
      return s;
  }
}

static l_noret lexerror (LexState *ls, const char *msg, int token) {
  msg = katG_addinfo(ls->K, msg, ls->source, ls->linenumber);
  if (token)
    printf("%s near %s at line %d\n", msg, katX_token2str(ls, token), ls->linenumber);
  katD_throw(ls->K, KAT_ERRSYNTAX);
}

l_noret katX_syntaxerror (LexState *ls, const char *msg) {
  lexerror(ls, msg, ls->t.token);
}

TString *katX_newstring (LexState *ls, const char *str, size_t l) {
  return katS_newlstr(ls->K, str, l);
}

static void inclinenumber (LexState *ls) {
  int old = ls->current;
  next(ls);  /* skip '\n' or '\r' */
  if (currIsNewline(ls) && ls->current != old)
    next(ls);  /* skip '\n\r' or '\r\n' */
  ls->linenumber++;
}

void katX_setinput (kat_State *K, LexState *ls, ZIO *z, TString *source,
                    int firstchar) {
  ls->t.token = 0;
  ls->K = K;
  ls->current = firstchar;
  ls->lookahead.token = TK_EOS;  /* no look-ahead token */
  ls->z = z;
  ls->fs = NULL;
  ls->linenumber = 1;
  ls->lastline = 1;
  ls->source = source;
  ls->buff = &K->buff;
  katZ_resizebuffer(ls->K, ls->buff, 64);  /* initialize buffer */
}

static int check_next1 (LexState *ls, int c) {
  if (ls->current == c) {
    next(ls);
    return 1;
  }
  else return 0;
}

static int check_next2 (LexState *ls, const char *set) {
  if (ls->current == set[0] || ls->current == set[1]) {
    save_and_next(ls);
    return 1;
  }
  else return 0;
}

static int read_numeral (LexState *ls, SemInfo *seminfo) {
  do {
    save_and_next(ls);
  } while (lisdigit(ls->current) || ls->current == '.');
  
  save(ls, '\0');
  
  if (strchr(katZ_buffer(ls->buff), '.')) {
    seminfo->r = strtod(katZ_buffer(ls->buff), NULL);
    return TK_FLT;
  } else {
    seminfo->i = strtoll(katZ_buffer(ls->buff), NULL, 10);
    return TK_INT;
  }
}

static void read_string (LexState *ls, int del, SemInfo *seminfo) {
  save_and_next(ls);  /* keep delimiter */
  while (ls->current != del) {
    switch (ls->current) {
      case EOZ:
        lexerror(ls, "unfinished string", TK_EOS);
        break;
      case '\n':
      case '\r':
        lexerror(ls, "unfinished string", TK_STRING);
        break;
      case '\\': {  /* escape sequences */
        int c;
        save_and_next(ls);  /* keep '\\' */
        switch (ls->current) {
          case 'n': c = '\n'; goto read_save;
          case 't': c = '\t'; goto read_save;
          case 'r': c = '\r'; goto read_save;
          case '\\': c = '\\'; goto read_save;
          case '\"': c = '\"'; goto read_save;
          case '\'': c = '\''; goto read_save;
          read_save:
            next(ls);
            save(ls, c);
            break;
          default:
            save_and_next(ls);  /* keep char as-is */
            break;
        }
        break;
      }
      default:
        save_and_next(ls);
    }
  }
  save_and_next(ls);  /* skip delimiter */
  seminfo->ts = katX_newstring(ls, katZ_buffer(ls->buff) + 1,
                                   katZ_bufflen(ls->buff) - 2);
}

static int llex (LexState *ls, SemInfo *seminfo) {
  katZ_resetbuffer(ls->buff);
  for (;;) {
    switch (ls->current) {
      case '\n': case '\r': {  /* line breaks */
        inclinenumber(ls);
        break;
      }
      case ' ': case '\f': case '\t': case '\v': {  /* spaces */
        next(ls);
        break;
      }
      case '/': {  /* division or comment */
        next(ls);
        if (check_next1(ls, '/')) {  /* line comment? */
          while (!currIsNewline(ls) && ls->current != EOZ)
            next(ls);  /* skip until end of line */
          break;
        }
        else if (check_next1(ls, '*')) {  /* block comment? */
          for (;;) {
            if (ls->current == EOZ)
              lexerror(ls, "unfinished comment", TK_EOS);
            if (ls->current == '*') {
              next(ls);
              if (ls->current == '/') {
                next(ls);
                break;
              }
            }
            else if (currIsNewline(ls))
              inclinenumber(ls);
            else
              next(ls);
          }
          break;
        }
        else return '/';
      }
      case '=': {
        next(ls);
        if (check_next1(ls, '=')) return TK_EQ;  /* '==' */
        else return '=';
      }
      case '<': {
        next(ls);
        if (check_next1(ls, '=')) return TK_LE;  /* '<=' */
        else if (check_next1(ls, '<')) return TK_SHL;  /* '<<' */
        else return '<';
      }
      case '>': {
        next(ls);
        if (check_next1(ls, '=')) return TK_GE;  /* '>=' */
        else if (check_next1(ls, '>')) return TK_SHR;  /* '>>' */
        else return '>';
      }
      case '!': {
        next(ls);
        if (check_next1(ls, '=')) return TK_NE;  /* '!=' */
        else return '!';
      }
      case ':': {
        next(ls);
        if (check_next1(ls, ':')) return TK_COLON_COLON;  /* '::' */
        else if (check_next1(ls, '=')) return TK_COLON_EQ;  /* ':=' */
        else return ':';
      }
      case '-': {
        next(ls);
        if (check_next1(ls, '>')) return TK_ARROW;  /* '->' */
        else return '-';
      }
      case '&': {
        next(ls);
        if (check_next1(ls, '&')) return TK_AND_AND;  /* '&&' */
        else return '&';
      }
      case '|': {
        next(ls);
        if (check_next1(ls, '|')) return TK_OR_OR;  /* '||' */
        else return '|';
      }
      case '.': {
        save_and_next(ls);
        if (check_next1(ls, '.')) {
          if (check_next1(ls, '<'))
            return TK_DOT_DOT_LT;   /* '..<' */
          else return TK_DOT_DOT;   /* '..' */
        }
        else if (!lisdigit(ls->current)) return '.';
        else return read_numeral(ls, seminfo);
      }
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        return read_numeral(ls, seminfo);
      }
      case '"': case '\'': {  /* literal strings */
        read_string(ls, ls->current, seminfo);
        return TK_STRING;
      }
      case EOZ: {
        return TK_EOS;
      }
      default: {
        if (lislalpha(ls->current)) {  /* identifier or reserved word? */
          TString *ts;
          do {
            save_and_next(ls);
          } while (lislalnum(ls->current));
          ts = katX_newstring(ls, katZ_buffer(ls->buff),
                              katZ_bufflen(ls->buff));
          seminfo->ts = ts;
          if (isreserved(ts))  /* reserved word? */
            return ts->extra - 1 + FIRST_RESERVED;
          else {
            return TK_NAME;
          }
        }
        else {  /* single-char tokens */
          int c = ls->current;
          next(ls);
          return c;
        }
      }
    }
  }
}

void katX_next (LexState *ls) {
  ls->lastline = ls->linenumber;
  if (ls->lookahead.token != TK_EOS) {  /* is there a look-ahead token? */
    ls->t = ls->lookahead;  /* use this one */
    ls->lookahead.token = TK_EOS;  /* and discharge it */
  }
  else
    ls->t.token = llex(ls, &ls->t.seminfo);  /* read next token */
}

int katX_lookahead (LexState *ls) {
  ls->lookahead.token = llex(ls, &ls->lookahead.seminfo);
  return ls->lookahead.token;
}