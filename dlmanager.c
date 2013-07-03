// Copyright 2013 Quentin Gibert.
// You may use this work without restrictions, as long as this notice is included.
// The work is provided "as is" without warranty of any kind, neither express nor implied.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include "dlmanager.h"

int main(int argc, char *argv[])
{
    const char *listfilename = "dlmanagerlist";
    int editval = 0;
    int getval = 0;
    
    
    editval = edit(listfilename);
    if(editval == -1)
    {
        fprintf(stderr, "unable to open file for edition.\n");
        return -1;
    }

    getval = getlist(listfilename);
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
    double kbnow = 0;
    double kbtotal = 0;
    double mbnow = 0;
    double mbtotal = 0;
    double gbnow = 0;
    double gbtotal = 0;
    char *filename = myp->filename;
    int percentage = 0;
    int dashes = 0;
    int i = 0;
    

    
    percentage = (dlnow/dltotal) * 100;
    kbnow = dlnow / 1024;
    kbtotal = dltotal / 1024;
    mbnow = kbnow / 1024;
    mbtotal = kbtotal / 1024;
    gbnow = mbnow / 1024;
    gbtotal = mbtotal / 1024;
    
    fflush(stdout);
    if(percentage < 0)
        percentage = 0;
    if( percentage < 10)
	fprintf(stdout, "  %d", percentage);
    if((percentage > 9) && (percentage < 100))
        fprintf(stdout, " %d", percentage);
    if(percentage > 99)
        fprintf(stdout, "%d", percentage);
    fprintf(stdout, "%% ");
//display directly the appropriate unit
    if(dltotal < 1024)
        fprintf(stdout, " %5.1f/%5.1f B ", (float)dlnow,(float)dltotal);
    if((kbtotal < 1024) && (dltotal > 1024))
        fprintf(stdout, " %5.1f/%5.1f kB", (float)kbnow,(float)kbtotal);
    if((kbtotal > 1024) && (mbtotal < 1024))
        fprintf(stdout, " %5.1f/%5.1f mB", (float)mbnow,(float)mbtotal);
    if(mbtotal > 1024)
        fprintf(stdout, " %5.1f/%5.1f GB", (float)gbnow,(float)gbtotal);
    fprintf(stdout, "\r");
    return 0;
}

char *getfilename(CURL *curl, char *link)
{
    int i = 0;
    int j = 0;
    int k = -1;
    int mark = 0;
    int newlenght = 0;
    char name[1000];
    char *nameret = NULL;
   
    for(i=0;link[i] != '\0';i++)
    {
	if(link[i] == '/')
	{
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
                usleep(1000);
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
    curl_off_t existsize = 0;
    long dlenght = 0;
    struct stat statbuf;
    
    
    pagefilename = getfilename(curl, link);
    prog.filename = pagefilename;

    if(ntry == 0)
        fprintf(stdout, "Getting '%s':\n", pagefilename);

    curl_easy_setopt(curl, CURLOPT_URL, link);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    if((stat(pagefilename, &statbuf) == 0))
    {
        existsize = statbuf.st_size;
        float kbsize = existsize / 1024;
        float mbsize = kbsize / 1024;
        float gbsize = mbsize / 1024;
        
        pagefile = fopen(pagefilename, "a+");
        if(ntry == 0)
        {
            printf("Already downloaded: ");
            if((float)existsize < 1024)
                printf("%5.1f B\n", (float)existsize);
            if((kbsize < 1024) && (existsize > 1024))
                printf("%5.1f kB\n", kbsize);
            if((kbsize > 1024) && (mbsize < 1024))
                printf("%5.1f mB\n", mbsize);
            if(mbsize > 1024)
                printf("%5.1f GB\n", gbsize);
        }
        
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE , existsize);
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM , 0);
        pagefile = fopen(pagefilename, "w");
    }
    if (pagefile) 
    {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
        ret = curl_easy_perform(curl);
        //printf("%d\n", ret);
        switch(ret)
        {
            case 3:
                fprintf(stderr, "Badly formatted URL.Ignoring...\n");
                return 0;
                break;
            case 1:
                fprintf(stderr, "Unsupported protocol.Ignoring...\n");
                return 0;
                break;
            default:
                break;
        }
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_UPLOAD, &dlenght);

        if((existsize != 0) && (dlenght == 0) && (ret == 33))
        {
            fprintf(stdout, "\nfile already complete\n");
            fclose(pagefile);
            return 0;
        }
        
	if(ret != 0)
	{
            if(ntry == NTRYMAX)
            {
                perror("Download failed");
                fclose(pagefile);
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
    char my_cmd[256];
    struct txteditors editors_a[5];
    struct stat statbuf;
    
    
    //if list does not exist, create empty file
    if(stat(file, &statbuf) == -1)
    {
        FILE *f;
        f = fopen(file, "w");
        fclose(f);
    }

    editors_a[0].editor = "nano";
    editors_a[1].editor = "pico";
    editors_a[2].editor = "vim";
    editors_a[3].editor = "vi";
    editors_a[4].editor = "emacs";
    
    for(i=0;i<5;i++)
    {
        sprintf(my_cmd, "%s %s", editors_a[i].editor, file);
	
	retval = system(my_cmd);
	if(retval != -1)
	    break;
    }
    if(retval == -1)
    {
        fprintf(stderr, "None of the text editors I tried seem to be present on this system.\n");
	return -1;
    }
    return 0;
}
