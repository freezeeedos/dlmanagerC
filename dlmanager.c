// Copyright 2013 Quentin Gibert.
// You may use this work without restrictions, as long as this notice is included.
// The work is provided "as is" without warranty of any kind, neither express nor implied.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <curl/curl.h>
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3
#define NTRYMAX 5


struct myprogress {
    double lastruntime;
    CURL *curl;
    char *filename;
};

struct txteditors
{
    char *editor;
};

struct failures
{
    char *link;
};

struct completed
{
    char *link;
};

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
static int progress(void *p,
                    double dltotal, double dlnow,
                    double ultotal, double ulnow);
char *getfilename(CURL *curl, char *link);
int getlist(const char *filename);
int getlink(char *link, struct myprogress prog, CURL *curl, int ntry);
int edit(const char *file);


int main(int argc, char *argv[])
{
    const char *listfilename = "/tmp/dlmanagerlist";
    int editval = 0;
    int getval = 0;
    
    
    editval = edit(listfilename);
    if(editval == -1)
    {
	fprintf(stderr, "unable to open file for edition.\n");
	return -1;
    }

    getval = getlist(listfilename);
//     unlink(listfilename);
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
//display directly the appropriate unit
    if(kbtotal < 1000)
        fprintf(stdout, "] %f/%f kB ", kbnow,kbtotal);
    if(kbtotal > 1000)
        fprintf(stdout, "] %f/%f mB ", mbnow,mbtotal);
    fprintf(stdout, "\r");
    return 0;
}

char *getfilename(CURL *curl, char *link)
{
    int i = 0;
    int j = 0;
    int k = -1;
//     int s = 0;
    int mark = 0;
    int newlenght = 0;
    char name[1000];
    char *nameret = NULL;
   
    for(i=0;link[i] != '\0';i++)
    {
	if(link[i] == '/')
	{
// 	    s++;
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
//     nameret = name;
    nameret = curl_easy_unescape( curl , name , 0 , 0 );
    return nameret;
}

int getlist(const char *filename)
{
    CURL *curl;
    FILE *listfile;
    struct myprogress prog;
    struct failures failed[1000];
    struct completed complete[1000];
    char line[1000];
    char *url;
    int ret;
    int i = 0;
    int ok = 0;
    int fail = 0;
    

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
        i = 0;
	ret = getlink(url, prog, curl, i);
	if(ret == -1)
	{
            for(i=1;i<NTRYMAX+1;i++)
	    {
	        sleep(1);
                fprintf(stderr, "[Try %d]\r", (i+1));
                ret = getlink(url, prog, curl, i);
                if(ret == 0)
                {
                    break;
                }
                if((ret == -1) && (i == NTRYMAX))
                {
                    fail++;
                    failed[fail].link = line;
                }
	    }
	}
	else
        {
            ok++;
            complete[ok].link = line;          
        }
    }
    curl_easy_cleanup(curl);
    fclose(listfile);
    unlink(filename);
    
    if(fail != 0)
    {
        listfile = fopen(filename, "w");
        for(i=1;i<fail+1;i++)
        {
            if(i == 1)
                fprintf(stderr, "\n\n***FAILED DOWNLOADS:\n");
            
            fprintf(stderr, "failed: %s", failed[i].link);
            fprintf(listfile, "%s", failed[i].link);
        }
        fclose(listfile);
        return -1;
    }
    
    return 0;
}

int getlink(char *link, struct myprogress prog, CURL *curl, int ntry)
{
    FILE *pagefile;
    char *pagefilename;
    int i;
    int ret;
    double existsize = 0;
    double dlenght = 0;
    struct stat statbuf;
    
    
    pagefilename = getfilename(curl, link);
    prog.filename = pagefilename;

    if(ntry == 0)
        fprintf(stdout, "Getting '%s':\n", pagefilename);
    curl_easy_setopt(curl, CURLOPT_URL, link);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    if((stat(pagefilename, &statbuf) == 0))
    {
        existsize = statbuf.st_size;
        double kbsize = existsize / 1000;
        double mbsize = kbsize / 1000;
        
        pagefile = fopen(pagefilename, "a+");
        if(kbsize < 1000)
            printf("downloaded: %f kB\n", kbsize);
        if(kbsize > 1000)
            printf("downloaded: %f mB\n", mbsize);
        
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM , statbuf.st_size);
    }
    else
    {
//         existsize = 0;
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM , 0);
        pagefile = fopen(pagefilename, "w");
    }
    if (pagefile) 
    {
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
        ret = curl_easy_perform(curl);
        
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_UPLOAD, &dlenght);
//         printf("lenght: %f\n", dlenght);
        if((existsize != 0) && (dlenght == 0))
        {
            fprintf(stderr, "file already complete\n");
            fclose(pagefile);
            return 0;
        }
        
	if(ret != 0)
	{
            if(ntry == NTRYMAX)
            {
                perror("Download failed");
//                 fprintf(stdout, "\n");
                fclose(pagefile);
                unlink(pagefilename);
            }
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
