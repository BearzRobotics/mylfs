// dprint.h
#ifndef DPRINT_H
#define DPRINT_H



void passed(const char *mesg);
void built(int count, const char *mesg);
void failed(const char *mesg);
void skip(const char *mesg);
void warn(const char *mesg);
void header(const char *mesg);
#endif
