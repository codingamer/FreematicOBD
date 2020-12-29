#ifndef __customized_DateAndTime_H
#define __customized_DateAndTime_H

time_t StringToDatetime(char *str);
void UpdateTime(void);
void SetSysInstance(FreematicsESP32*);
void GetCurrentTime(char*);
int GetShiftTime(char*,char*);
void GetCurrentGPSTime(char*);
#endif /* __customized_DateAndTime_H */
