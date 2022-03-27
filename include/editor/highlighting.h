#pragma once

#include <stdio.h>

#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

struct _edit_conf E;

// highlighting
enum highlight_ed {
  HL_NORMAL = 0,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_TYPES,     // int, bool, etc (set to NULL if none)
  HL_KEYWORDS,
  HL_STRING,
  HL_NUMBER
};

// highlighting
#define HL_NUM (1<<0)
#define HL_STR (1<<1)

// C/C++
char* C_HL_extensions[] = { ".c", ".h", ".cpp", ".hpp", NULL };
char* C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return",
  "else", "struct", "union", "typedef", "static", "enum", "class",
  "case", "#include", "#define", "{", "}",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

// KessC
char* KESS_C_HL_ext[] = { ".kessc", NULL };
char* KESS_C_HL_keywords[] = {
	"printf",

	"uint8|", NULL
};

// Assembly

/// Intel
char* ASMI_HL_extensions[] = { ".asm", NULL };
char* ASMI_HL_keywords[] = {
  "mov", "bytes", "jmp", "jnz", "je", "jne", "jg", "jl",
  "lgdt", "or", "test", "cpuid", "add", "sub", "mul",

  "ax|", "bx|", "cx|", "dx|", "ah|", "bh|", "ch|", "dh|", "al|", "bl",
  "cl", "dl", "eax|", "ecx|", "edx|", "ebx|", "esp|", "ebp|", "esi|", "edi|",
  "rax|", "rcx|", "rdx|", "rbx|", "rsp|", "rbp|", "rsi|", "rdi|", NULL
};

/// AT&T (ew)


// Python
char* PY_HL_extensions[] = { ".py", NULL };
char* PY_HL_keywords[] = {
  "and", "as", "assert", "async", "await", "break", "class", "continue",
  "def", "del", "elif", "else", "except", "finally", "for", "from", "global",
  "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass",
  "raise", "return", "try", "while", "with", "yield",

  "False|", "None|", "True|", "int|", "float|", "tuple|", "dict|", "str|",
  NULL
};

// markdown
char* MD_HL_extensions[] = { ".md", NULL };
char* MD_HL_keywords[]   = {
	"#", "##", "###", "####", "#####", "-", "1.", NULL
};

// DB of syntax highlighting
struct edit_syntax HLDB[] = {
  // Systems languages
  {
    "c",
    C_HL_extensions,
    C_HL_keywords,
    "//", "/*", "*/",
    HL_NUM | HL_STR
  },
	{
		"kessc",
		KESS_C_HL_ext,
		KESS_C_HL_keywords,
		"//", "/*", "*/",
		HL_NUM | HL_STR
	},
  {
    "intel-asm",
    ASMI_HL_extensions,
    ASMI_HL_keywords,
    ";;", ";;", ";;",
    HL_NUM | HL_STR
  },
  // Higher level languages
  {
    "py",
    PY_HL_extensions,
    PY_HL_keywords,
    "#", "\"\"\"", "\"\"\"",
    HL_NUM | HL_STR
  },
	// documentation languages
	{
		"md",
		MD_HL_extensions,
		MD_HL_keywords,
		"//",						// tbh idk this
		HL_NUM | HL_STR
	}
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

// ....

int is_seperator(int c){
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void update_syntax(erow* row){
  row->hl = realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize);

  if(E.e_syntax == NULL) return;

  char** keywords = E.e_syntax->keywords;

  char* scs = E.e_syntax->singline_comment_start;
  char* mcs = E.e_syntax->multiline_comment_start;
  char* mce = E.e_syntax->multiline_comment_end;
  int scs_len = scs ? strlen(scs) : 0;
  int mcs_len = mcs ? strlen(mcs) : 0;
  int mce_len = mce ? strlen(mce) : 0;

  int prev_sep  = 1;
  int in_string = 0;
  int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);

  int i = 0;
  while(i < row->rsize){
    char c = row->render[i];
    unsigned char prev_hl = (i > 0) ? row->hl[i-1] : HL_NORMAL;
    // strings
    if(E.e_syntax->flags & HL_STR){
      if(in_string){
        row->hl[i] = HL_STRING;
        if(c == in_string) in_string = 0;
        i++;
        prev_sep = 1;
        continue;
      } else {
        if(c == '"' || c == '\''){
          in_string = c;
          row->hl[i] = HL_STRING;
          i++;
          continue;
        }
      }
    }

    // comments
    if(scs_len && !in_string && !in_comment) {
      if(!strncmp(&row->render[i], scs, scs_len)) {
        memset(&row->hl[i], HL_COMMENT, row->rsize - i);
        break;
      }
    }

    // multi-line comments
    if(mcs_len && mce_len && !in_string) {
      if(in_comment) {
        row->hl[i] = HL_MLCOMMENT;
        if(!strncmp(&row->render[i], mce, mce_len)) {
          memset(&row->hl[i], HL_MLCOMMENT, mce_len);
          i += mce_len;
          in_comment = 0;
          prev_sep = 1;
          continue;
        } else {
          i++;
          continue;
        }
      } else if(!strncmp(&row->render[i], mcs, mcs_len)) {
        memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
        i += mcs_len;
        in_comment = 1;
        continue;
      }
    }

    // numbers
    if(E.e_syntax->flags & HL_NUM){
      if(isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) {
        row->hl[i] = HL_NUMBER;
        i++;
        prev_sep = 0;
        continue;
      }
    }

    // keywords
    if(prev_sep){
      int j;
      for(j = 0; keywords[j]; j++){
        int klen = strlen(keywords[j]);
        int kw2  = keywords[j][klen - 1] == '|';
        if(kw2) klen--;

        if(!strncmp(&row->render[i], keywords[j], klen) &&
            is_seperator(row->render[i + klen])){
          memset(&row->hl[i], kw2 ? HL_KEYWORDS : HL_TYPES, klen);
          i += klen;
          break;
        }
      }
      if(keywords[j] != NULL) {
        prev_sep = 0;
        continue;
      }
    }

    prev_sep = is_seperator(c);
    i++;
  }

  int changed = (row->hl_open_comment != in_comment);
  row->hl_open_comment = in_comment;
  if(changed && row->idx + 1 < E.numrows) update_syntax(&E.row[row->idx+1]);
}

int syntax_to_cl(int hl){
  switch(hl) {
    case HL_NUMBER: return 31;   // red
    case HL_KEYWORDS: return 32; // magenta
    case HL_TYPES: return 33;    // yellow
    case HL_STRING: return 35;   // magenta
    case HL_MLCOMMENT:
    case HL_COMMENT: return 36;  // cyan
    default: return 37;
  }
}

void sel_syn(){
  E.e_syntax = NULL;
  if(E.filename == NULL) return;
  char* ext = strrchr(E.filename, '.');
  for(unsigned int j = 0; j < HLDB_ENTRIES; j++){
    struct edit_syntax* s = &HLDB[j];
    unsigned int i = 0;
    while(s->filematch[i]){
      int is_ext = (s->filematch[i][0] == '.');
      if((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
          (!is_ext && strstr(E.filename, s->filematch[i]))) {
        E.e_syntax = s;
        return;
      }
      i++;
    }
  }
}
