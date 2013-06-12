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
int getlist(const char *filename);
int edit(const char *file);

int main(int argc, char *argv[])
{
    const char *listfilename = "/tmp/dlmanagerlist";
    int editval;
    
    curl_global_init(CURL_GLOBAL_ALL);
    
    editval = edit(listfilename);
    if(editval == -1)
    {
	fprintf(stderr, "unable to open file for edition.\n");
	return -1;
    }

    getlist(listfilename);
    unlink(listfilename);
    fprintf(stdout,"\n");
    return 0;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = 0;
  written = fwrite(ptr, size, nmemb, (FILE *)stream);
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
    if( percentage > 0)
    {
	fprintf(stdout, " %d", percentage);
	fprintf(stdout, "%% [");
	for(dashes = 0;dashes+1<percentage;dashes++)
	    fprintf(stdout,"-");
	for(i=0;i<100-percentage;i++)
	    fprintf(stdout, " ");
	fprintf(stdout, "]");
    }
    fprintf(stdout, "\r");
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

int getlist(const char *filename)
{
    CURL *curl;
    FILE *listfile;
    FILE *pagefile;
    char *link = malloc(10000);
    char *pagefilename;
    struct myprogress prog;
    

    listfile = fopen(filename, "r");
    if(listfile == NULL)
    {
        perror("failed to open links list");
        exit(EXIT_FAILURE);
    }

    curl = curl_easy_init();

    prog.lastruntime = 0;
    prog.curl = curl;
    while(fgets(link, 10000, listfile) != NULL)
    {
	pagefilename = getfilename(link);
	prog.filename = pagefilename; 
	fprintf(stdout, "Getting '%s':\n", pagefilename);
	curl_easy_setopt(curl, CURLOPT_URL, link);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	pagefile = fopen(pagefilename, "wb");
	if (pagefile) {
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
	    if(curl_easy_perform(curl) != 0)
                perror("Download failed");
	    fclose(pagefile);
	}
        fprintf(stdout, "\n");
    }
    curl_easy_cleanup(curl);
    
    return 0;
}

int edit(const char *file)
{
    int i;
    int retval;
    char *my_cmd = NULL;
    struct txteditors editors_a[5];

    editors_a[0].editor = "nano";
    editors_a[1].editor = "pico";
    editors_a[2].editor = "vim";
    editors_a[3].editor = "vi";
    editors_a[4].editor = "emacs";
    
    for(i=0;i<5;i++)
    {
	my_cmd = strdup(editors_a[i].editor);
	strcat(my_cmd, " ");
	strcat(my_cmd, file);
	
//	fprintf(stderr, "Trying %s\n", editors_a[i].editor);
	retval = system(my_cmd);
	if(retval != -1)
	    break;
    }
    if(retval == -1)
    {
        fprintf(stderr, "None of the listed text editors seem to be present on this system.\n");
	return -1;
    }
    return 0;
}
