#include <stdio.h>
#include <stdlib.h>
#include <libxml/xmlreader.h>
#include "ShowPageConf.h"
#include <unistd.h>
#include <string.h>

typedef struct pagelist *ptr;
struct pagelist{
                ptr prev;
                int ID;
                ptr next;
                } start;


void openPage(const xmlTextReaderPtr reader)
{
    const unsigned char *content = xmlTextReaderReadString(reader);
    if(xmlTextReaderHasValue(reader) && *content && *content != '\n'){
        fprintf(stdout, "Read %s\n", content);
    }
}


int main(int argc, char *argv[])
{
    if(argc != 2){
        fprintf(stdout, "Usage: %s (start | stop | restart)\n", argv[0]);
        return 0;
    }
    xmlTextReaderPtr reader;
    int ret;

    reader = xmlNewTextReaderFilename("/home/marcus/pages.xml");

    if (reader == NULL) {
        fprintf(stderr, "Cannot open document \n");
        return 1;
    }
    ret = xmlTextReaderRead(reader);
    while(ret == 1){
        openPage(reader);
        ret = xmlTextReaderRead(reader);
    }


    /*    fprintf(stdout, "%s Version %d.%d\n",
            argv[0],
            ShowPage_VERSION_MAJOR,
            ShowPage_VERSION_MINOR);*/
    return 0;
}
