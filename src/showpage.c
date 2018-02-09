#include <stdio.h>
#include <stdlib.h>
#include <libxml2/libxml/xmlreader.h>
#include "ShowPageConf.h"
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>


int recvd_sig = 0;
char *cur_IDs;

void openPage(const xmlTextReaderPtr reader, FILE *fp)
{
    const unsigned char *content = xmlTextReaderReadString(reader);
    if(xmlTextReaderHasValue(reader) && *content != '\n'){
        syslog(LOG_INFO, "Read %s\n", content);
        fprintf(fp, "%s ", content);
    }
}

int read_xml()
{
    xmlTextReaderPtr reader;
    int ret;

    char *home_path = getenv("HOME");
    char *pxml_path = malloc(strlen(home_path)*2);
    strcpy(pxml_path, home_path);
    strcat(pxml_path, "/pages.xml\0");

    reader = xmlNewTextReaderFilename(pxml_path);

    if (reader == NULL) {
        syslog(LOG_ERR, "Cannot open document \n");
        return 1;
    }
    ret = xmlTextReaderRead(reader);
    FILE *fp = popen("xargs chromix-too open \0", "w\0");
    while(ret == 1){
        openPage(reader, fp);
        ret = xmlTextReaderRead(reader);
    }
    pclose(fp);
    free(pxml_path);
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
         * The pipe param is used to redirect stdout of the child process
         * This is so that the opening of URLs can be held off until chromix has
         * established a connection with chromium.
         */
        syslog(LOG_INFO, "Starting chromix-too-server with PID %d\n", getpid());
        syslog(LOG_INFO, "Redirecting stdout of process %d to pipe to process %d\n", getpid(), getppid());

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

        char *cserver_path ="/usr/bin/chromix-too-server";
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
       syslog(LOG_INFO, "Forking process for chrome with PID %d\n", getpid());
        char *chrm_path="/usr/bin/chromium-browser";
        char *const argv_chrm[] = {chrm_path, "--kiosk\0", NULL};
        execv(chrm_path, argv_chrm);
    }
    return pid_chrm;
}

pid_t fork_pagec(const char *old_IDs)
{
    pid_t pid;
    if(!system("chromix-too rm \"chrome://\""))
        syslog(LOG_INFO, "Removed all chrome:// URLs\n");
    FILE *fp = popen("chromix-too tid", "r");
    char IDs[50];
    memset(IDs, 0, sizeof(IDs));
    char buf[3];
    while(fgets(buf, sizeof(buf), fp) != NULL){
        char *pos;
        while((pos=strchr(buf, '\n')) != NULL)
            *pos = '\0';
        if(strstr(old_IDs, buf) == NULL){
            strcat(IDs, buf);
            strcat(IDs, ",\0");
        }
        memset(buf, 0, sizeof(buf));
    }
    pclose(fp);

    if((pid = fork()) < 0){
        perror("Could not fork process for pagecycler");
        return 1;
    }else if(pid == 0){
        syslog(LOG_INFO, "Forked process for pagecycler with PID %d and IDs %s\n", getpid(), IDs);
        char *path = "build/pagecycle";
        char *const argv_pagec[] = {path, IDs, NULL};
        execvp(path, argv_pagec);
    }

    memset(cur_IDs, 0, sizeof(char)*100);
    cur_IDs = IDs;
    return pid;
}

int main(int argc, char *argv[])
{
    openlog("showpage", LOG_PID, LOG_LOCAL0);
    if(signal(SIGTERM, signal_handler) == SIG_ERR){
        perror("Failed to register signal handler!\0");
        goto term_routine;
    }

    cur_IDs = malloc(100*sizeof(char));

    pid_t pid_cserver;
    pid_t pid_chrm;
    pid_t pid_pagec;
    /*declare pipe file descriptor for communication between child and parent */
    int pipefd[2];
    if(pipe(pipefd)){
        perror("Failed to open pipe");
        goto term_routine;
    }

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

    pid_pagec = fork_pagec("");
    if(pid_pagec < 0)
        goto term_routine;

sig_loop:
    while(!recvd_sig){
        sleep(1);
    }
    switch(recvd_sig){
        case SIGHUP:
            kill(pid_pagec, SIGTERM);
            if(read_xml())
                goto term_routine;
            pid_pagec = fork_pagec(cur_IDs);
            if(pid_pagec < 0)
                goto term_routine;
            goto sig_loop;
        case SIGTERM:
            goto term_routine;
        default:
            syslog(LOG_ERR, "Failed to handle signal: signal %d is not supported!", recvd_sig);
            goto sig_loop;
    }
term_routine:
        memset(cur_IDs, 0, sizeof(char)*100);
        free(cur_IDs);
        kill(pid_chrm, SIGTERM);
        kill(pid_cserver, SIGTERM);
        kill(pid_pagec, SIGTERM);
        closelog();

    return 0;
}
