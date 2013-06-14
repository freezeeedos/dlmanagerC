#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3

struct myprogress {
    double lastruntime;
    CURL *curl;
    char *filename;
};

struct txteditors
{
    char *editor;
};

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
static int progress(void *p,
                    double dltotal, double dlnow,
                    double ultotal, double ulnow);
char *getfilename(char *link);
int getlist(const char *filename);
int getlink(char *link, struct myprogress prog, CURL *curl);
int edit(const char *file);





int main(int argc, char *argv[])
{
    const char *listfilename = "/tmp/dlmanagerlist";
    int editval = 0;
    
    
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
    double kbnow = 0;
    double kbtotal = 0;
    double mbnow = 0;
    double mbtotal = 0;
    char *filename = myp->filename;
    int percentage = 0;
    int dashes = 0;
    int i = 0;

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);
    if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
	myp->lastruntime = curtime;
    }
    
    percentage = (dlnow/dltotal) * 100;
    kbnow = dlnow / 1000;
    kbtotal = dltotal / 1000;
    mbnow = kbnow / 1000;
    mbtotal = kbtotal / 1000;
    
    fflush(stdout);
    if(percentage < 0)
        percentage = 0;
    if( percentage < 10)
	fprintf(stdout, "  %d", percentage);
    if((percentage > 9) && (percentage < 100))
        fprintf(stdout, " %d", percentage);
    if(percentage > 99)
        fprintf(stdout, "%d", percentage);
    fprintf(stdout, "%% [");
    for(dashes = 0;dashes+1<percentage;dashes++)
        fprintf(stdout,"-");
    for(i=0;i<100-percentage;i++)
        fprintf(stdout, " ");
    if(kbtotal < 1000)
        fprintf(stdout, "] %f/%f kB", kbnow,kbtotal);
    if(kbtotal > 1000)
        fprintf(stdout, "] %f/%f mB", mbnow,mbtotal);
    fprintf(stdout, "\r");
    return 0;
}

char *getfilename(char *link)
{
    int i = 0;
    int j = 0;
    int k = -1;
    int s = 0;
    int mark = 0;
    int newlenght = 0;
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
    newlenght = i - mark;
    for(j=mark+1;j<i;j++)
    {
	k++;
	name[k] = link[j];
    }
    name[k] = '\0';
    nameret = name;
    return nameret;
}

int getlist(const char *filename)
{
    CURL *curl;
    FILE *listfile;
    struct myprogress prog;
    char line[1000];
    char *url;
    int ret;
    int i;
    

    listfile = fopen(filename, "r");
    if(listfile == NULL)
    {
        perror("failed to open links list");
        exit(EXIT_FAILURE);
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    prog.lastruntime = 0;
    prog.curl = curl;
    while(fgets(line, 1000, listfile) != NULL)
    {
	url = line;
	ret = getlink(url, prog, curl);
	if(ret == -1)
	{
            for(i=0;i<50;i++)
	    {
	        sleep(1);
                fprintf(stderr, "[Try %d]\n", (i+1));
                ret = getlink(url, prog, curl);
                if(ret == 0)
                    break;
	    }
	}
    }
    curl_easy_cleanup(curl);
    return 0;
}

int getlink(char *link, struct myprogress prog, CURL *curl)
{
    FILE *pagefile;
    char *pagefilename;
    int i;
    
    pagefilename = getfilename(link);
    prog.filename = pagefilename; 
    fprintf(stdout, "Getting '%s':\n", pagefilename);
    curl_easy_setopt(curl, CURLOPT_URL, link);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    pagefile = fopen(pagefilename, "w");
    if (pagefile) 
    {
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
	if(curl_easy_perform(curl) != 0)
	{
	    perror("Download failed");
            fclose(pagefile);
            unlink(pagefilename);
	    return -1;
	}
	
	fclose(pagefile);
    }
    fprintf(stdout, "\n");
    return 0;
}

int edit(const char *file)
{
    int i = 0;
    int retval = 0;
    char *my_cmd;
    struct txteditors editors_a[5];

    editors_a[0].editor = "nano";
    editors_a[1].editor = "pico";
    editors_a[2].editor = "vim";
    editors_a[3].editor = "vi";
    editors_a[4].editor = "emacs";
    
    for(i=0;i<5;i++)
    {
	//my_cmd = malloc(sizeof(editors_a[i].editor)+sizeof(" "+1)+sizeof(file+1));
	my_cmd = strdup(editors_a[i].editor);
	strcat(my_cmd, " ");
	strcat(my_cmd, file);
	
//	fprintf(stderr, "Trying %s\n", editors_a[i].editor);
	retval = system(my_cmd);
	if(retval != -1)
	    break;
	//free(my_cmd);
    }
    if(retval == -1)
    {
        fprintf(stderr, "None of the listed text editors seem to be present on this system.\n");
	return -1;
    }
    return 0;
}
