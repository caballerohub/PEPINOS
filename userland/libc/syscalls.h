
void console_write(char*);
void exit(int);
int open(char*);
int read(int, char*, int);
void close(int);
int console_read(char*);
void chdir(char*);
void pwd(char*);
int exec(char*, char**);
void* sbrk(int);
int wait(int*);
int kill(int, int);
int sigaction(int, void*);
