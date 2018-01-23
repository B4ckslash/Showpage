#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

typedef struct pagelist *ptr;
typedef struct pagelist{
                ptr prev;
                int ID;
                ptr next;
                } List;

int main(int argc, char *argv[])
{
    if(argc != 2 && argc != 4){
        fprintf(stdout, "Usage: %s [-o old Tab IDs] new Tab IDs (Tab IDs are to be formatted as a \
            comma separated string)\n", argv[0]);
    }else if (argc == 4){
        static char *old_tid;
        for(int i = 0; i < argc; i++){
            if(strcmp(argv[i], "-o") == 0){
                old_tid = argv[i+1];
            }
        }
        if(old_tid == NULL){
            fprintf(stdout, "Only use the -o option with a string of old tab IDs following it");
        }

        static char *id;
        id = strtok(old_tid, ",");
        FILE *pipe = popen("xargs chromix-too rm\0", "w");
        while(id != NULL){
            fprintf(pipe,"%s", id);
            id = strtok(NULL, ",");
        }
        pclose(pipe);
    }
   return 0; 
}
