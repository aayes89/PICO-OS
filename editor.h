#pragma once
#include <Arduino.h>
#include "fs.h"
#include "io.h"

class Editor {
public:
  Editor();
  bool openFile(const char* path);
  void run();

private:
  String filePath;
  String buffer;
  String killBuffer; // para cortes
  size_t cursor;  // índice absoluto en buffer
  bool dirty;
  size_t screenRows = 30;  // alto útil del área de texto
  size_t screenCols = 80;  // opcional si quieres limitar ancho
  size_t rowGoal = 0;      // columna deseada al mover ↑↓
  bool rowGoalSet = false;
  size_t scrollTop = 0;    // primera línea visible
  const uint8_t TAB_WIDTH = 4;

  void render();
  void save();
  void handleKey(int ch);
  void insertChar(char c); 
  String getLine(size_t row);
  void deleteLine(size_t row);
  void backspaceChar();
  void deleteForward();
  void newline();
  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();
  void pageDown();
  void pageUp();
  void computeCursorRC(size_t& row, size_t& col) const;  
  size_t prevChar(size_t i) const;
  size_t nextChar(size_t i) const; 
  size_t visualColAt(size_t index) const;
  size_t lineCount() const;
  size_t indexFromRowCol(size_t targetRow, size_t targetCol);
  size_t lineLen(size_t row); 
  int readEscSeq();
   
 
};
