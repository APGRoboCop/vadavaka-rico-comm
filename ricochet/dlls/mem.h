 
/*

===== mem.h ========================================================

  Declarations of functions otherwise left out - desNotes

*/


void *memset(void *addr, int c, size_t n);
int stricmp(const char *str1, const char *str2);
char *strcat(char *str1, char *str2);
char *strstr(const char *str1, char *str2);
char *strncpy(const char *str1, const char *str2, size_t n);
int strncmp(const char *str1, const char *str2, size_t n);
int _strnicmp(const char *str1, const char * str2, size_t n);
char *strtok(char *str1, char *str2);

int isprint(int c);
int isspace(int c);
int isalnum(int c);
int isalpha(int c);
int isdigit(int c);
int toupper(int c);
int tolower(int c);
