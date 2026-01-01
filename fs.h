#pragma once
#include <LittleFS.h>

extern bool fsDirty;
extern FSInfo fs_info;

void initFS();
void markDirty();
void flushFS();
bool makeDirSafe(const String& p);
String normalizePath(const char* path);
bool existsFileOrDir(const String& p);
bool isDir(const String& p);
String basenameOf(const String& p);
String parentOf(const String& p);
bool removeRecursive(const String& p);
bool copyRecursive(const String& src, const String& dst);
void appendToFile(const String& path, const String& text);
void treeRecursive(const String& path, String prefix);
void findRecursive(const String& path, const String& target);
