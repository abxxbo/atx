/* includes */

// Since our compiler complains about
// getline() being implicit, we need
// to define some feature test macros
// Even if it doesn't complain, these
// are good for compability
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

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

/* defines */

// atx version
#define ATX_VER "0.0.1-beta"
#define ATX_TAB 2           // 8 < 2 in terms of tabs

// Get ctrl key equiv of a character
#define CTRL_KEY(n) ((n) & 0x1f)

// other
#define CLR_SCRN()  write(STDOUT_FILENO, "\x1b[2J", 4);
#define RESET_CUR() write(STDOUT_FILENO, "\x1b[H", 3);

// block some keys
enum ed_keys {
  BACKSPACE = 127,
  ARR_L = 1000,
  ARR_R,
  ARR_U,
  ARR_D,
  DEL_KEY
};

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

/* data */

struct edit_syntax {
  char* filetype;
  char** filematch;
  char** keywords;
  char* singline_comment_start;
  char* multiline_comment_start;
  char* multiline_comment_end;
  int flags;
};

typedef struct erow {
  int idx;
  int hl_open_comment;
  int size;
  int rsize;
  char* chars;
  char* render;
  unsigned char* hl;
} erow;

struct _edit_conf {
  // Cursor
  int cx, cy;
  // Tabs
  int rx;     // Tabs + cursor impl
  // Scrolling
  int rowoff; // vert
  int coloff; // horiz
  // Screen
  int screenrows;
  int screencols;
  int numrows;
  erow* row;
  // status bar
  char* filename;
  // is modified?
  int dirty;    // 0 = clean ; 1 = dirty
  // editor syntax
  struct edit_syntax* e_syntax;
  struct termios orig_termios;
};
struct _edit_conf E;

/* filetypes */
char* C_HL_extensions[] = { ".c", ".h", ".cpp", ".hpp", NULL };
char* C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return",
  "else", "struct", "union", "typedef", "static", "enum", "class",
  "case", "#include", "#define", "{", "}",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

struct edit_syntax HLDB[] = {
  {
    "c",
    C_HL_extensions,
    C_HL_keywords,
    "//", "/*", "*/",
    HL_NUM | HL_STR
  },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))
/* terminal */

void die(const char* fmt){
  CLR_SCRN();
  RESET_CUR();
  perror(fmt);
  exit(1);
}

void disable_raw() {
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void _enable_raw() {
  if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disable_raw);

  struct termios raw = E.orig_termios;
  
  // flags
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  
  // Timeout for read
  raw.c_cc[VMIN]  = 0;
  raw.c_cc[VTIME] = 1;
  
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int get_cur_pos(int *rows, int *cols){
  char buf[32];
  unsigned int i = 0;

  if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  
  while(i < sizeof(buf) - 1){
    if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if(buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if(buf[0] != '\x1b' || buf[1] != '[') return -1;
  if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}

int get_win_size(int* rows, int* colums){
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return get_cur_pos(rows, colums);
  }
  *colums = ws.ws_col;
  *rows   = ws.ws_row;
  return 0;
}

/* syntax highlighting */
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

/* row ops */
void _update_row(erow * row){
  int tabs = 0;
  for(int j = 0; j < row->size; j++) if(row->chars[j] == '\t') tabs++;
  free(row->render);
  row->render = malloc(row->size + tabs*(ATX_TAB - 1) + 1);

  int idx = 0;
  for(int j = 0; j < row->size; j++){
    if(row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while(idx % ATX_TAB != 0) row->render[idx++] = ' ';
    } else row->render[idx++] = row->chars[j];
  }
  row->render[idx] = '\0';
  row->rsize = idx;

  update_syntax(row);
}

void _appnd_row(int at, char* s, size_t len){
  if(at < 0 || at > E.numrows) return;

  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  for(int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

  E.row[at].idx = at;

  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  
  E.row[at].rsize  = 0;
  E.row[at].render = NULL;
  E.row[at].hl = NULL;
  E.row[at].hl_open_comment = 0;
  _update_row(&E.row[at]);

  E.numrows++;
  E.dirty++;
}

void free_row(erow *row){
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void del_row(int at){
  if(at < 0 || at >= E.numrows) return;
  free_row(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  for(int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void insertchar(erow* row, int at, int c){
  if(at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  _update_row(row);
  E.dirty++;
}

void appnd_to_end(erow* row, char* s, size_t len){
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  _update_row(row);
  E.dirty++;
}

void del_chr(erow *row, int at){
  if(at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  _update_row(row);
  E.dirty++;
}

/* editor operations */
void insert_char_ed(int c){
  if(E.cy == E.numrows) {
    _appnd_row(E.numrows, "", 0);
  }  
  insertchar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

void insert_nline() {
  if(E.cx == 0){
    _appnd_row(E.cy, "", 0);
  } else {
    erow* row = &E.row[E.cy];
    _appnd_row(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    _update_row(row);
  }
  E.cy++;
  E.cx = 0;
}

void del_char_ed() {
  if(E.cy == E.numrows) return;
  if(E.cx == 0 && E.cy == 0) return;
  erow* row = &E.row[E.cy];
  if(E.cx > 0) {
    del_chr(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    appnd_to_end(&E.row[E.cy - 1], row->chars, row->size);
    del_row(E.cy);
    E.cy--;
  }
}

/* file io */
char* rows_to_str(int* buflen){
  int totlen = 0;
  for(int j = 0; j < E.numrows; j++) totlen += E.row[j].size + 1;
  *buflen = totlen;

  char* buf = malloc(totlen);
  char* p = buf;
  for(int j = 0; j < E.numrows; j++){
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

void open_file(char* file) {
  free(E.filename);
  E.filename = strdup(file);
  
  sel_syn();

  FILE* fp = fopen(file, "r");
  if(!fp){
    // Create the file
    FILE* fp2 = fopen(file, "a");
    fclose(fp2);
    fp = fopen(file, "r");
  }

  char* line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while((linelen = getline(&line, &linecap, fp)) != -1){
    while(linelen > 0 && (line[linelen - 1] == '\n' ||
                          line[linelen - 1] == '\r'))
      linelen--;
    _appnd_row(E.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  E.dirty = 0;
}

void save_file() {
  if(E.filename == NULL) return;
  sel_syn();

  int len;
  char* buf = rows_to_str(&len);
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  ftruncate(fd, len);
  write(fd, buf, len);
  close(fd);
  E.dirty = 0;
  free(buf);
}

/* append buffer */
#define ABUF_INIT {NULL, 0}
struct abuf {
  char* b;
  int len;
};

void ab_append(struct abuf* ab, const char* fmt, int len){
  char* new = realloc(ab->b, ab->len + len);
  if(new == NULL) return;
  memcpy(&new[ab->len], fmt, len);
  ab->b = new;
  ab->len += len;
}

void ab_free(struct abuf *ab){
  free(ab->b);
}

/* output */
void edit_scroll(){
  E.rx = E.cx;
  // Vert
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }

  // Horiz
  if(E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if(E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void _draw_rows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "ATX editor v%s", ATX_VER);
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          ab_append(ab, "~", 1);
          padding--;
        }
        while (padding--) ab_append(ab, " ", 1);
        ab_append(ab, welcome, welcomelen);
      } else {
        ab_append(ab, "~", 1);
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff;
      if(len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      char* c = &E.row[filerow].render[E.coloff];
	  	unsigned char* hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      for(int j = 0; j<len; j++){
        if(hl[j] == HL_NORMAL){
          if(current_color != -1){
            ab_append(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          ab_append(ab, &c[j], 1);
        } else {
          int color = syntax_to_cl(hl[j]);
          if(color != current_color){
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
            ab_append(ab, buf, clen);
          }
          ab_append(ab, &c[j], 1);
				}
			}
      ab_append(ab, "\x1b[39m", 5);
    }

    ab_append(ab, "\x1b[K", 3);
    ab_append(ab, "\r\n", 2);
  }
}

// status bar!
void status_bar(struct abuf* ab){
  ab_append(ab, "\x1b[7m", 4); // invert colors
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "File: %s %s",
        E.filename ? E.filename : "Untitled", E.dirty ? "(modified)" : "");
  
  
  int rlen;
  if(E.numrows != 404){
    rlen = snprintf(rstatus, sizeof(rstatus), "Line %d of %d (ft: %s)",
      E.cy + 1, E.numrows, E.e_syntax ? E.e_syntax->filetype : "no ft");
  } else {
    rlen = snprintf(rstatus, sizeof(rstatus), "Line %d of [LINES NOT FOUND] (ft: %s)", E.cy + 1, E.e_syntax ? E.e_syntax->filetype : "no ft");
  }
  if(len > E.screencols) len = E.screencols;
  ab_append(ab, status, len);
  while(len < E.screencols) {
    if(E.screencols - len == rlen) {
      ab_append(ab, rstatus, rlen);
      break;
    } else {
      ab_append(ab, " ", 1);
      len++;
    }
  }
  ab_append(ab, "\x1b[m", 3); // revert to normal
}

void _refresh() {
  edit_scroll();
  struct abuf ab = ABUF_INIT;

  ab_append(&ab, "\x1b[?25l", 6);
  ab_append(&ab, "\x1b[H", 3);

  _draw_rows(&ab);
  status_bar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
  ab_append(&ab, buf, strlen(buf));

  ab_append(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}

/* input */
int read_key() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      switch (seq[1]) {
        case 'A': return ARR_U;
        case 'B': return ARR_D;
        case 'C': return ARR_R;
        case 'D': return ARR_L;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}


void editor_mv_cur(int key) {
  erow* row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  switch (key) {
    case ARR_L:
      if(E.cx != 0) E.cx--;
      else if(E.cy > 0) {
        E.cy--;
        E.cx = E.row[E.cy].size;
      }
      break;
    case ARR_R:
      if(row && E.cx < row->size) E.cx++;
      else if(row && E.cx == row->size){
        E.cy++;
        E.cx = 0;
      }
      break;
    case ARR_U:
      if(E.cy != 0) E.cy--;
      break;
    case ARR_D:
      if(E.cy < E.numrows) E.cy++;
      break;
  }

  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) E.cx = rowlen;
}
void process_key(){
  int c = read_key();

  switch (c) {
    case '\r':
      insert_nline();
      break;
    case BACKSPACE:
    case DEL_KEY:
      if(c == DEL_KEY) editor_mv_cur(ARR_R);
      del_char_ed();
      break;

    // exit
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    // save file
    case CTRL_KEY('s'):
      save_file();
      break;
    
    // other
    case ARR_U:
    case ARR_D:
    case ARR_L:
    case ARR_R:
      editor_mv_cur(c);
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;
    default:
      insert_char_ed(c);
      break;
  }
}

/* init */
void _init_editor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.e_syntax = NULL;
  if(get_win_size(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
  E.screenrows -= 1;
}

int main(int argc, char** argv) {
  if(strcmp(argv[1], "--help") == 0){
    printf("Ctrl+S -- Save\nCtrl+Q -- Quit\n");
    exit(0);
  }
  // Enter raw mode
  _enable_raw();
  // Initialize editor
  _init_editor();

  if(argc >= 2){
    open_file(argv[1]);
  }

  while(1){
    _refresh();     // Clears buffer
    process_key();  // Repositions cursor
  }
  return 0;
}
