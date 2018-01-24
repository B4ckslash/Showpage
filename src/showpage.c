#include <stdio.h>
#include <stdlib.h>
#include <libxml/xmlreader.h>
#include "ShowPageConf.h"
#include <unistd.h>
#include <signal.h>
#include <string.h>


int recvd_sig = 0;

void openPage(const xmlTextReaderPtr reader, FILE *fp)
{
    const unsigned char *content = xmlTextReaderReadString(reader);
    if(xmlTextReaderHasValue(reader) && *content && *content != '\n'){
        fprintf(stdout, "Read %s\n", content);
        fprintf(fp, "%s ", content);
    }
}

int read_xml()
{
    xmlTextReaderPtr reader;
    int ret;

    reader = xmlNewTextReaderFilename("/home/marcus/pages.xml");

    if (reader == NULL) {
        fprintf(stderr, "Cannot open document \n");
        return 1;
    }
    ret = xmlTextReaderRead(reader);
    FILE *fp = popen("xargs chromix-too open \0", "w\0");
    while(ret == 1){
        openPage(reader, fp);
        ret = xmlTextReaderRead(reader);
    }
    pclose(fp);
    return 0;
}

void signal_handler(int sig)
{
    recvd_sig = sig;
}


pid_t fork_cserver(int pipe[])
{
    pid_t pid_cserver;
    if((pid_cserver = fork()) < 0){
        perror("Could not fork process for chromix server");
        return pid_cserver;
    }else if(pid_cserver == 0){
        /*
         * Execution branch of child process for chromix-too-server
         *
         * The pipe declared earlier is used to redirect stdout of the child process
         * This is so that the opening of URLs can be held off until chromix has
         * established a connection with chromium.
         */
        fprintf(stdout, "Starting chromix-too-server with PID %d\n", getpid());
        fprintf(stdout, "Redirecting stdout of process %d to pipe to process %d\n", getpid(), getppid());

        /*close read end of pipe*/
        if(close(pipe[0]) < 0){
            perror("Could not close read end of pipe");
            return -1;
        }
        /*actually redirect stdout to pipe*/
        if(dup2(pipe[1], STDOUT_FILENO) < 0){
            perror("Could not redirect stdout to pipe");
            return -1;
        }

        char *cserver_path ="/usr/local/bin/chromix-too-server";
        char *const argv_cserver[] = {cserver_path, NULL};
        execv(cserver_path, argv_cserver);
    }

    return pid_cserver;
}

pid_t fork_chrm()
{
    pid_t pid_chrm;
    if((pid_chrm = fork()) < 0){
        perror("Could not fork process for chrome");
        return pid_chrm;
    }else if(pid_chrm == 0){
        fprintf(stdout, "Forking process for chrome with PID %d\n", getpid());
        char *chrm_path="/usr/bin/chromium-browser";
        char *const argv_chrm[] = {chrm_path, "--kiosk\0", NULL};
        execv(chrm_path, argv_chrm);
    }
    return pid_chrm;
}

pid_t fork_pagec()
{
    pid_t pid;
    FILE *fp = popen("chromix-too tid", "r");
    char IDs[50];
    char buf[2];
    while(fgets(buf, sizeof(buf), fp) != NULL){
        strcat(IDs, buf);
        strcat(IDs, ",");
        memset(buf, '\0', sizeof(buf));
    }
    if((pid = fork()) < 0){
        perror("Could not fork process for pagecycler");
        return 1;
    }else if(pid == 0){
        fprintf(stdout, "Forked process for pagecycler with PID %d and IDs %s\n", getpid(), IDs);
        char *path = "/home/marcus/git/showpage_c/build/pagecycle";
        char *const argv_pagec[] = {path, IDs, NULL};
        execvp(path, argv_pagec);
    }
    return pid;
}

int main(int argc, char *argv[])
{
    if(signal(SIGTERM, signal_handler) == SIG_ERR){
        perror("Failed to register signal handler!\0");
    }

    pid_t pid_cserver;
    pid_t pid_chrm;
    pid_t pid_pagec;
    /*declare pipe file descriptor for communication between child and parent */
    int pipefd[2];
    pipe(pipefd);

    pid_cserver = fork_cserver(pipefd);
    if(pid_cserver < 0)
            goto term_routine;

    /*immediately close fd for the write end of the pipe if parent*/
    if(close(pipefd[1]) < 0){
        perror("Could not close write end of pipe");
        goto term_routine;
    }

    pid_chrm = fork_chrm();
    if(pid_chrm < 0)
            goto term_routine;

    /* TODO: Add pagecycling functionality and spawn the cycler as a child process(preferably after pages have been opened)*/

    /*Wait till chromix server connects so that the open command of chromix-too does not fail*/
    {
        char buf[250];
        int websck_conn = 0;
        FILE *pipe_fp = fdopen(pipefd[0], "r");
        while(!websck_conn){
            if(fgets(buf, sizeof(buf), pipe_fp) != NULL){
                if(strstr(buf, "websocket client connected") != NULL){
                    websck_conn = 1;
                }
            }
            memset(buf,0, sizeof(buf));
        sleep(1);
        }
    }

    if(read_xml())
            goto term_routine;

    pid_pagec = fork_pagec();
    if(pid_pagec < 0)
        goto term_routine;

    while(!recvd_sig){
        sleep(1);
    }

term_routine:
        kill(pid_chrm, SIGTERM);
        kill(pid_cserver, SIGTERM);
        kill(pid_pagec, SIGTERM);

    return 0;
}
