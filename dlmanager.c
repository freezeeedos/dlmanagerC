/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2012, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     1

struct myprogress {
  double lastruntime;
  CURL *curl;
  char *filename;
};

struct txteditors
{
    const char *editor;
};

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
static int progress(void *p,
                    double dltotal, double dlnow,
                    double ultotal, double ulnow);
char *getfilename(char *link);
int getlist(const char *filename, CURL *curl);
int edit(const char *file);

int main(int argc, char *argv[])
{
    CURL *curl;
    const char *listfilename = "/tmp/dlmanagerlist";
    int editval;
    
    editval = edit(listfilename);
    if(editval == -1)
    {
	fprintf(stderr, "unable to open file for edition.\n");
	return -1;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    getlist(listfilename, curl);
    curl_easy_cleanup(curl);
    fprintf(stdout,"\n");
    return 0;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}


static int progress(void *p,
		    double dltotal, double dlnow,
		    double ultotal, double ulnow)
    {
    struct myprogress *myp = (struct myprogress *)p;
    CURL *curl = myp->curl;
    double curtime = 0;
    char *filename = myp->filename;
    int percentage;
    int dashes;
    int i;

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

    /* under certain circumstances it may be desirable for certain functionality
	to only run every N seconds, in order to do this the transaction time can
	be used */
    if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
	myp->lastruntime = curtime;
// 	fprintf(stdout, "TOTAL TIME: %f \n", curtime);
    }
    percentage = (dlnow/dltotal) * 100;
//     fprintf(stdout, "UP: %g of %g  DOWN: %g of %g\n",
// 	    ulnow, ultotal, dlnow, dltotal);
    fprintf(stderr, "\r");
    if( percentage > 0)
    {
	fprintf(stderr, "%d", percentage);
	fprintf(stderr, "%% [");
	for(dashes = 0;dashes+1<percentage;dashes++)
	    fprintf(stderr,"-");
	for(i=0;i<100-percentage;i++)
	    fprintf(stderr, " ");
	fprintf(stderr, "]");
    }
    fflush(stdout);
    return 0;
}

char *getfilename(char *link)
{
    int i;
    int j;
    int k = -1;
    int s = 0;
    int mark;
    int newlenght;
    char name[1000];
    char *nameret = NULL;
   
    for(i=0;link[i] != '\0';i++)
    {
	if(link[i] == '/')
	{
	    s++;
	    mark = i;
	}
    }
//     printf("%d\n", i);
//     printf("%d\n", s);
//     printf("marker at %d\n", mark);
    newlenght = i - mark;
//     printf("Newlenght is %d\n", newlenght);
    for(j=mark+1;j<i;j++)
    {
	k++;
// 	printf("name[%d] = link[%d]\n", k, j);
	name[k] = link[j];
    }
    name[k] = '\0';
//     printf("%s\n", name);
    nameret = name;
    return nameret;
}

int getlist(const char *filename, CURL *curl)
{
//     CURL *curl;
//     curl_global_init(CURL_GLOBAL_ALL);
    FILE *listfile;
    FILE *pagefile;
    char link[10000];
    char *pagefilename;
    struct myprogress prog;

    curl = curl_easy_init();
    prog.lastruntime = 0;
    prog.curl = curl;

    listfile = fopen(filename, "r");
    while(fgets(link, 10000, listfile) != NULL)
    {
	pagefilename = getfilename(link);
	prog.filename = pagefilename; 
	fprintf(stdout, "Getting '%s':\n", pagefilename);
	curl_easy_setopt(curl, CURLOPT_URL, link);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	pagefile = fopen(pagefilename, "wb");
	if (pagefile) {
	    curl_easy_setopt(curl, CURLOPT_FILE, pagefile);
	    curl_easy_perform(curl);
	    fclose(pagefile);
	}
    }

    return 0;
}

int edit(const char *file)
{
    int i;
    int retval;
    char *my_cmd = NULL;
    struct txteditors editors_a[2];

    editors_a[0].editor = "nano";
    editors_a[1].editor = "vim";
    
    for(i=0;i<2;i++)
    {
	my_cmd = strdup(editors_a[i].editor);
	strcat(my_cmd, " ");
	strcat(my_cmd, file);
	
	fprintf(stderr, "Trying %s\n", editors_a[i].editor);
	retval = system(my_cmd);
	if(retval != -1)
	    break;
    }
    if(retval == -1)
	return -1;
    return 0;
}