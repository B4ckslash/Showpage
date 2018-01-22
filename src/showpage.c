#include <stdio.h>
#include <stdlib.h>
#include <libxml/xmlreader.h>
#include "ShowPageConf.h"
#include <unistd.h>
#include <signal.h>
#include <string.h>

/*typedef struct pagelist *ptr;
struct pagelist{
                ptr prev;
                int ID;
                ptr next;
                } start;*/

int recvd_sig = 0;

void openPage(const xmlTextReaderPtr reader, FILE *fp)
{
    const unsigned char *content = xmlTextReaderReadString(reader);
    if(xmlTextReaderHasValue(reader) && *content && *content != '\n'){
        fprintf(stdout, "Read %s\n", content);
        fprintf(fp, "%s ", content);
    }
}

void signal_handler(int sig)
{
    recvd_sig = sig;
}


int main(int argc, char *argv[])
{
    if(signal(SIGTERM, signal_handler) == SIG_ERR){
        perror("Failed to register signal handler!\0");
    }

    pid_t pid_cserver;
    pid_t pid_chrm;
    pid_t pid_pagec;
    /*if(argc != 2){
        fprintf(stdout, "Usage: %s (start | stop | restart)\n", argv[0]);
        return 0;
    }*/
    if((pid_cserver = fork()) < 0){
        perror("Could not fork process for chromix server");
        return 1;
    }else if(pid_cserver == 0){
        fprintf(stdout, "Starting chromix-too-server with PID %d\n", getpid());
        char *cserver_path ="/usr/local/bin/chromix-too-server";
        char *const argv_cserver[] = {cserver_path, NULL};
        execv(cserver_path, argv_cserver);
    }

    if((pid_chrm = fork()) < 0){
        perror("Could not fork process for chrome");
        kill(pid_cserver, SIGTERM);
        return 1;
    }else if(pid_chrm == 0){
        fprintf(stdout, "Forking process for chrome with PID %d\n", getpid());
        char *chrm_path="/usr/bin/chromium-browser";
        char *const argv_chrm[] = {chrm_path, "--kiosk\0", NULL};
        execv(chrm_path, argv_chrm);
    }

    /* TODO: Add pagecycling functionality and spawn the cycler as a child process(preferably after pages have been opened)*/

    char buf[LINE_MAX];
    int websck_conn = 0;
    while(!websck_conn){
        /*TODO: fgets cannot read stdout, find another way*/
        if(fgets(buf, sizeof(buf), stdout) != NULL){
            if(strstr(buf, "websocket client connected") != NULL){
                websck_conn = 1;
            }
        }
        fprintf(stdout, "%s", buf);
        memset(buf,0, sizeof(buf));
    sleep(1);
    }

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

    while(!recvd_sig){
        sleep(1);
    }

    kill(pid_chrm, SIGTERM);
    kill(pid_cserver, SIGTERM);

    /*    fprintf(stdout, "%s Version %d.%d\n",
            argv[0],
            ShowPage_VERSION_MAJOR,
            ShowPage_VERSION_MINOR);*/
    return 0;
}
