#include "Editor.h"

Editor::Editor()
  : cursor(0), dirty(false) {}

bool Editor::openFile(const char* path) {
  filePath = normalizePath(path);
  buffer = "";

  File f = LittleFS.open(filePath, "r");
  if (f) {
    while (f.available()) buffer += (char)f.read();
    f.close();
  }

  cursor = buffer.length();
  dirty = false;
  return true;
}
static inline void termClear() {
  Serial.print("\x1B[2J\x1B[H");  // clear + home
}
static inline void termGoto(size_t r, size_t c) {
  Serial.printf("\x1B[%u;%uH", (unsigned)(r + 1), (unsigned)(c + 1));
}
size_t Editor::lineCount() const {
  size_t n = 1;
  for (size_t i = 0; i < buffer.length(); i++)
    if (buffer[i] == '\n') n++;
  return n;
}
void Editor::computeCursorRC(size_t& row, size_t& col) const {
  row = 0;
  col = 0;
  for (size_t i = 0; i < cursor && i < buffer.length(); i++) {
    if (buffer[i] == '\n') {
      row++;
      col = 0;
    } else col++;
  }
}
size_t Editor::indexFromRowCol(size_t targetRow, size_t targetCol) {
  size_t r = 0, c = 0;
  for (size_t i = 0; i < buffer.length(); i++) {
    if (r == targetRow && c == targetCol) return i;
    if (buffer[i] == '\n') {
      r++;
      c = 0;
    } else c++;
  }
  return buffer.length();
}
size_t Editor::lineLen(size_t row) {
  size_t r = 0, len = 0;
  for (size_t i = 0; i < buffer.length(); i++) {
    if (r == row) {
      if (buffer[i] == '\n') break;
      len++;
    }
    if (buffer[i] == '\n') r++;
  }
  return len;
}
static inline bool isUtf8Lead(uint8_t b) {
  return (b & 0xE0) == 0xC0;  // 2-byte seq
}
static inline bool isUtf8Cont(uint8_t b) {
  return (b & 0xC0) == 0x80;
}
static inline size_t utf8CharLen(uint8_t lead) {
  if ((lead & 0x80) == 0x00) return 1;
  if ((lead & 0xE0) == 0xC0) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  if ((lead & 0xF8) == 0xF0) return 4;
  return 1;
}
size_t Editor::nextChar(size_t i) const {
  if (i >= buffer.length()) return i;
  return i + utf8CharLen((uint8_t)buffer[i]);
}
size_t Editor::prevChar(size_t i) const {
  if (i == 0) return 0;
  size_t j = i - 1;
  while (j > 0 && isUtf8Cont(buffer[j])) j--;
  return j;
}
void Editor::render() {
  termClear();

  // barra superior azul
  Serial.print("\x1B[44;37m nano-lite  ^X salir  ^O guardar ^K cortar ^U pegar \x1B[0m\n");

  size_t row = 0, colByte = 0;
  computeCursorRC(row, colByte);
  size_t col = visualColAt(cursor);

  // ajustar scroll
  if (row < scrollTop) scrollTop = row;
  if (row >= scrollTop + screenRows)
    scrollTop = row - screenRows + 1;

  // dibujar líneas visibles
  size_t r = 0;
  size_t i = 0;

  while (i < buffer.length() && r < scrollTop + screenRows) {

    uint8_t b = buffer[i];

    // carácter UTF-8 de 2 bytes
    if (isUtf8Lead(b) && i + 1 < buffer.length()) {
      if (i == cursor) Serial.print("\x1B[7m");
      Serial.write(b);
      Serial.write(buffer[i + 1]);
      if (i == cursor) Serial.print("\x1B[0m");
      i += 2;
      continue;
    }

    if (r >= scrollTop) {
      if (i == cursor) {
        Serial.print("\x1B[7m");
        Serial.write(buffer[i] ? buffer[i] : ' ');
        Serial.print("\x1B[0m");
      } else {
        Serial.write(buffer[i]);
      }
    }

    if (buffer[i] == '\n') r++;
    i++;
  }

  // cursor al final del archivo
  if (cursor == buffer.length()) Serial.print("\x1B[7m \x1B[0m");

  Serial.println();

  // barra inferior azul
  Serial.print("\x1B[44;37m");
  Serial.printf(" %s %s | lines:%u  pos:%u,%u ",
                filePath.c_str(),
                dirty ? "[MODIFIED]" : "",
                (unsigned)lineCount(),
                (unsigned)(row + 1),
                (unsigned)(col + 1));
  Serial.print("\x1B[0m\n");

  termGoto((row - scrollTop) + 1, col);
}
void Editor::save() {
  File f = LittleFS.open(filePath, "w");
  if (!f) return;
  f.print(buffer);
  f.close();
  dirty = false;
}
void Editor::insertChar(char c) {
  if (c == '\t') {
    for (uint8_t i = 0; i < TAB_WIDTH; i++) {
      buffer = buffer.substring(0, cursor) + ' ' + buffer.substring(cursor);
      cursor++;
    }
    dirty = true;
    return;
  }

  buffer = buffer.substring(0, cursor) + c + buffer.substring(cursor);
  cursor++;
  dirty = true;
  //String left = buffer.substring(0, cursor);
  //String right = buffer.substring(cursor);
  //buffer = left + c + right;
  //cursor++;
  //dirty = true;
}
void Editor::backspaceChar() {
  if (cursor == 0) return;

  size_t start = prevChar(cursor);
  //while (start > 0 && (buffer[start] & 0xC0) == 0x80) start--;  // UTF-8
  buffer.remove(start, cursor - start);
  cursor = start;
  dirty = true;
}
void Editor::deleteForward() {
  if (cursor >= buffer.length()) return;

  size_t end = nextChar(cursor);
  //while (end < buffer.length() && (buffer[end] & 0xC0) == 0x80) end++;
  buffer.remove(cursor, end - cursor);
  dirty = true;
}
String Editor::getLine(size_t row) {
  size_t start = indexFromRowCol(row, 0);
  size_t end = start;
  while (end < buffer.length() && buffer[end] != '\n') end++;
  if (end < buffer.length()) end++;  // incluye salto
  return buffer.substring(start, end);
}
void Editor::deleteLine(size_t row) {
  size_t start = indexFromRowCol(row, 0);
  size_t end = start;
  while (end < buffer.length() && buffer[end] != '\n') end++;
  if (end < buffer.length()) end++;
  buffer.remove(start, end - start);
  cursor = start;
  dirty = true;
}
void Editor::newline() {
  insertChar('\n');
}
/* void Editor::moveLeft() { // old
  if (cursor > 0) cursor--;
  rowGoal = 0;
}*/
void Editor::moveLeft() {
  if (cursor == 0) return;
  cursor = prevChar(cursor);
  rowGoalSet = false;
}
/*void Editor::moveRight() { // old
  if (cursor < buffer.length()) cursor++;
  rowGoal = 0;
}*/
void Editor::moveRight() {
  if (cursor >= buffer.length()) return;
  cursor = nextChar(cursor);
  rowGoalSet = false;
}
void Editor::moveUp() {
  size_t row, col;
  computeCursorRC(row, col);
  if (row == 0) return;

  if (!rowGoalSet) {
    rowGoal = col;
    rowGoalSet = true;
  }
  row--;
  size_t len = lineLen(row);
  size_t col2 = rowGoal > len ? len : rowGoal;
  cursor = indexFromRowCol(row, col2);
}
void Editor::moveDown() {
  size_t row, col;
  computeCursorRC(row, col);
  size_t total = lineCount();
  if (row + 1 >= total) return;

  if (!rowGoalSet) {
    rowGoal = col;
    rowGoalSet = true;
  }
  row++;
  size_t len = lineLen(row);
  size_t col2 = rowGoal > len ? len : rowGoal;
  cursor = indexFromRowCol(row, col2);
}
void Editor::pageDown() {
  size_t delta = screenRows - 2;
  scrollTop += delta;
  moveDown();  // ajusta cursor dentro del texto real
}
void Editor::pageUp() {
  size_t delta = screenRows - 2;
  scrollTop = (scrollTop > delta) ? scrollTop - delta : 0;
  moveUp();
}
size_t Editor::visualColAt(size_t index) const {
  size_t col = 0;
  for (size_t i = 0; i < index; i++) {
    uint8_t b = buffer[i];
    if ((b & 0xC0) != 0x80) col++;  // cuenta solo lead bytes
  }
  return col;
}
void Editor::handleKey(int ch) {
  // Ctrl-X — guardar y salir
  if (ch == 0x18) {
    save();
    termClear();
    Serial.print("\x1B[H");
    return;
  }

  // Ctrl-O — guardar
  if (ch == 0x0F) {
    save();
    return;
  }

  // Backspace / DEL
  if (ch == 0x08 || ch == 0x7F) {
    backspaceChar();
    return;
  }

  // Enter
  if (ch == '\r' || ch == '\n') {
    newline();
    return;
  }

  // Secuencias ESC
  if (ch == 0x1B) {
    int code = readEscSeq();

    switch (code) {
      case 'A':  // ↑
        moveUp();
        return;
      case 'B':  // ↓
        moveDown();
        return;
      case 'C':  // →
        moveRight();
        return;
      case 'D':  // ←
        moveLeft();
        return;
      case (('3' << 8) | '~'):  // Supr
        deleteForward();
        return;
      case (('5' << 8) | '~'):  // PgUp
        pageUp();
        return;
      case (('6' << 8) | '~'):  // PgDn
        pageDown();
        return;
      default:
        return;
    }
  }

  // UTF-8 (2+ bytes)
  if ((uint8_t)ch >= 0xC2) {
    while (!Serial.available()) {}
    uint8_t c2 = Serial.read();
    if ((c2 & 0xC0) == 0x80) {
      buffer = buffer.substring(0, cursor)
               + String((char)ch) + String((char)c2)
               + buffer.substring(cursor);
      cursor = nextChar(cursor);  // avanza exactamente 1 char lógico
      dirty = true;
      return;
    }
  }

  // Ctrl-K — cortar línea
  if (ch == 0x0B) {
    size_t row, col;
    computeCursorRC(row, col);
    killBuffer = getLine(row);
    deleteLine(row);
    return;
  }

  // Ctrl-U — pegar línea
  if (ch == 0x15 && killBuffer.length()) {
    buffer = buffer.substring(0, cursor) + killBuffer + buffer.substring(cursor);
    size_t k = 0;
    while (k < killBuffer.length()) k = nextChar(k);
    cursor += k;
    dirty = true;
    return;
  }

  // ASCII imprimible
  if (ch >= 0x20 && ch <= 0x7E) {
    insertChar((char)ch);
    return;
  }
}
int Editor::readEscSeq() {
  while (!Serial.available()) {}
  int a = Serial.read();
  if (a != '[') return 0;

  while (!Serial.available()) {}
  int b = Serial.read();

  // flechas simples
  if (b == 'A' || b == 'B' || b == 'C' || b == 'D')
    return b;

  // secuencias extendidas tipo 3~,5~,6~
  if (b >= '0' && b <= '9') {
    while (!Serial.available()) {}
    int tilde = Serial.read();
    return (b << 8) | tilde;
  }

  return 0;
}
void Editor::run() {
  render();
  for (;;) {
    if (!Serial.available()) continue;
    int ch = Serial.read();
    // Ctrl+X para salir (x2)
    if (ch == 0x18) {
      save();
      termClear();
      Serial.print("\x1B[H");
      return;
    }
    handleKey(ch);
    render();
  }
}