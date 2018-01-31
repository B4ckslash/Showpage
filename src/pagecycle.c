#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>

typedef struct pagelist *ptr;
typedef struct pagelist{
                ptr prev;
                int ID;
                ptr next;
                } List;
int running;

ptr init_plist(char *arg)
{
    int arr_size = 0;
    size_t arglen = strlen(arg);
    char *id;
    char *arg_cp = malloc(arglen);
    strcpy(arg_cp, arg);
    /*calling strtok twice, once to get the necessary array size and once to populate the array*/
    id = strtok(arg, ",");
    while(id != NULL){
        arr_size++;
        id = strtok(NULL, ",");
    }

    if(arr_size == 0)
        return NULL;

    int ids[arr_size];
    memset(ids, 0, arr_size*sizeof(int));
    id = strtok(arg_cp, ",");
    int i = 0;
    while(id != NULL){
        ids[i] = atoi(id);
        i++;
        id = strtok(NULL, ",");
    }

    List *start = malloc(sizeof(List));
    i = 1;
    start->ID = ids[0];
    List *cur = start;
    while(i <= arr_size){
        if(i < arr_size){
            cur->next = malloc(sizeof(List));
            cur->next->prev = cur;
            cur = cur->next;
            cur->ID = ids[i];
        }else{
            cur->next = start;
            start->prev = cur;
        }
        i++;
    }
    free(arg_cp);
    return start;
}

void free_list(ptr list)
{
    list->prev->next = NULL;
    while(list != NULL)
    {
        ptr tmp = list;
        list = tmp->next;
        free(tmp);
    }
}

void cycle(ptr node)
{
    while(running){
        FILE *fp = popen("xargs chromix-too focus \0", "w\0");
        fprintf(fp, "%d ", node->ID);
        node = node->next;
        pclose(fp);
        system("chromix-too reload -active\0");
        sleep(30);
    }
}

void term_handle(int sig)
{
    running = 0;
}

int main(int argc, char *argv[])
{
    if(signal(SIGTERM, term_handle) < 0)
    {
        perror("Could not register SIGTERM handler");
        return 1;
    }

    openlog("pagecycle", LOG_PID, LOG_LOCAL0);

    ptr strtPtr;
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
        while(id != NULL){
            FILE *pipe = popen("xargs chromix-too rm \0", "w\0");
            fprintf(pipe,"%s", id);
            id = strtok(NULL, ",");
            pclose(pipe);
        }
        strtPtr = init_plist(argv[3]);
    }else{
        syslog(LOG_INFO, "Initializing pagelist with IDs %s\n", argv[1]);
        strtPtr = init_plist(argv[1]);
    }

    if(strtPtr == NULL){
        syslog(LOG_ERR, "Could not initialize page list");
        return 1;
    }

    running = 1;
    cycle(strtPtr);
    free_list(strtPtr);
    closelog();

   return 0;
}
