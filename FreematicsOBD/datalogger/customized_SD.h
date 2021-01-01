#include "customized_OBD.h"

#ifndef __customized_SD_H
#define __customized_SD_H

int getFileID(File& root);
void appendFile(fs::FS &fs, const char * path, const char * message);
void init_SD_Card(ObdInfoPid*,int);
char* getFileName(void);

#endif /* __customized_SD_H */
