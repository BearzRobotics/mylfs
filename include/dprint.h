// dprint.h
#ifndef DPRINT_H
#define DPRINT_H



void info(const char *fmt, ...);
void warn(const char *fmt, ...);
void passed(const char *fmt, ...);
void skip(const char *fmt, ...);
void failed(const char *fmt, ...);
void header(const char *fmt, ...);
#endif
