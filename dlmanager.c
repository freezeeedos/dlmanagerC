//This program is a small download manager that will try to open
//a text editor and download the links you paste in it.
//Copyright (C) 2013  Quentin Gibert

//This program is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License along
//with this program; if not, write to the Free Software Foundation, Inc.,
//51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.        
          
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>

#include <curl/curl.h>

#define NTRYMAX 50


struct myprogress
{
    long startime;
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

static size_t write_data(void *ptr, 
                         size_t size, 
                         size_t nmemb, void *stream);
static int progress(struct myprogress *prog,
                    double dltotal, double dlnow,
                    double ultotal, double ulnow);
char *getfilename(CURL *curl, char *link);
int getlist(const char *filename);
int manage_ret(CURL *curl, int ret);
int getlink(char *link, 
            CURL *curl, int ntry);
int edit(const char *file);


int interv_count = 0;

int main(int argc, char *argv[])
{
    const char *listfilename = ".dlmanagerlist";
    int editval = 0;
    int getval = 0;
    
    
    editval = edit(listfilename);
    if(editval == -1)
    {
        fprintf(stderr, "unable to open file for edition.\n");
        return -1;
    }

    getval = getlist(listfilename);
    if(getval == -1)
    {
	fprintf(stderr, "Execution ended with errors\n");
	return -1;
    }
    
    fprintf(stdout,"\n");
    return 0;
}


static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = 0;
    written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

static int progress(struct myprogress *prog,
		    double dltotal, double dlnow,
		    double ultotal, double ulnow)
{
    double kbnow = 0;
    double kbtotal = 0;
    double mbnow = 0;
    double mbtotal = 0;
    double gbnow = 0;
    double gbtotal = 0;
    double dlremaining = 0;
    long startime = 0;
    long curtime = 0;
    long totaltime = 0;
    int eta = 0;
    int eta_sec = 0;
    int eta_min = 0;
    int eta_hour = 0;
    int percentage = 0;
    int kbrate = 0;
    int mbrate = 0;
    int rate = 0;

    startime = prog->startime;
    curtime = time(NULL);
    interv_count++;
    
    if(dltotal > 0)
	percentage = (dlnow/dltotal) * 100;
    
    kbnow = dlnow / 1024;
    kbtotal = dltotal / 1024;
    mbnow = kbnow / 1024;
    mbtotal = kbtotal / 1024;
    gbnow = mbnow / 1024;
    gbtotal = mbtotal / 1024;
    
    totaltime = curtime - startime;
    
    if(totaltime > 0)
    {
	rate = dlnow/totaltime;
	kbrate = kbnow/totaltime;
	mbrate = mbnow/totaltime;
    }

    dlremaining = dltotal - dlnow;
    
    if(rate > 0)
	eta = dlremaining / rate;
    
    eta_hour = (eta / 60) / 60;
    
    if((eta / 60) > 59)
    {
	eta_min = (eta / 60) - (eta_hour * 60);
    }
    else
    {
	eta_min = eta / 60;
    }

    if((eta > 59) && (rate > 0))
    {
	eta_sec = (dlremaining / rate) - ((eta_hour * 3600) + (eta_min * 60));
    }
    else
    {
	eta_sec = eta;
    }
    
 
    if((dlnow == 0) || (percentage == 100 ) || (interv_count > 1000))
    {
	
	interv_count = 0;
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
	    fprintf(stdout, " %5.1f/%5.1f B       ", (float)dlnow,(float)dltotal);
	if((kbtotal < 1024) && (dltotal > 1024))
	    fprintf(stdout, " %5.1f/%5.1f kB      ", (float)kbnow,(float)kbtotal);
	if((kbtotal > 1024) && (mbtotal < 1024))
	    fprintf(stdout, " %5.1f/%5.1f mB      ", (float)mbnow,(float)mbtotal);
	if(mbtotal > 1024)
	    fprintf(stdout, " %5.1f/%5.1f GB      ", (float)gbnow,(float)gbtotal);
	if(rate > 0)
	{
	    if(rate < 1024)
		fprintf(stdout, "%4d B/s", rate);
	    if(kbrate < 1024)
		fprintf(stdout, "%4d kB/s", kbrate);
	    if(kbrate > 1024)
		fprintf(stdout, "%5d mB/s", mbrate);
	    fprintf(stdout, "    eta: ");
	    fprintf(stdout, " %dh%dm%ds   ", eta_hour, eta_min, eta_sec);
	}
    }
//     printf(" Interv count=%d   ", interv_count);
    fprintf(stdout, "\r");
    return 0;
}


int getlist(const char *filename)
{
    CURL *curl;
    FILE *listfile;
    struct failures failed[1000];
    char line[1000];
    char *url;
    char *url_clean;
    int ret;
    int i = 0;
    int fail = 0;
    

    listfile = fopen(filename, "r");
    if(listfile == NULL)
    {
        perror("failed to open links list");
        return -1;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    while(fgets(line, 1000, listfile) != NULL)
    {
        for(i = 0;line[i] != '\0';i++)
	{
	    if((line[i] == '\r') || (line[i] == '\n'))
		line[i] = '\0';
	}
	
	i = strlen(line);
	
	if(i < 5)
	    continue;
	if((line[0] == ' ') || (line[0] == '\t'))
	    continue;
	
	url = line;
	url_clean = curl_easy_unescape(curl, url, 0, 0);
        i = 0;
	ret = getlink(url_clean, curl, i);
	if(ret == -1)
	{
            for(i++;i<NTRYMAX+1;i++)
	    {
                usleep(1000);
                fprintf(stderr, "[Try %d]\n", (i+1));
                ret = getlink(url_clean, curl, i);
                if(ret == 0)
                {
                    break;
                }
                if((ret == -1) && (i == NTRYMAX))
                {
                    failed[fail].link = strdup(line);
                    fail++;
                }
	    }
	}
    curl_free(url_clean);
    curl_easy_reset(curl);
    }
    
    
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    fclose(listfile);
    unlink(filename);
    
    if(fail != 0)
    {
        listfile = fopen(filename, "w");
        for(i=0;i<fail;i++)
        {
            if(i == 1)
                fprintf(stderr, "\n\n***FAILED DOWNLOADS:\n");
            
            fprintf(stderr, "failed: %s\n", failed[i].link);
            fprintf(listfile, "%s\n", failed[i].link);
	    free(failed[i].link);
        }
        fclose(listfile);
        return -1;
    }
    
    return 0;
}

int manage_ret(CURL *curl, int ret)
{
    int httpcode = 0;
    char *msg = NULL;

	switch(ret)
	{
	    case 78:
		msg = "Remote file not found";
		break;
	    case 60:
		msg = "Peer certificate cannot be authenticated with known CA certificates.";
		break;
	    case 51:
		msg = "The remote server's SSL certificate or SSH md5 fingerprint was deemed not OK.";
		break;
	    case 22:
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpcode);
		msg = "Web server returned";
		break;
	    case 3:
		msg = "Badly formatted URL.Ignoring...";
		break;
	    case 1:
		msg = "Unsupported protocol.Ignoring...";
		break;
	    default:
		break;
	}
	
	if(msg != NULL)
	{
	    fprintf(stderr, "%s", msg);
	    if(httpcode != 0)
		fprintf(stderr, " %d                    ", httpcode);
	    fprintf(stderr, "\n");
	    return 1;
	}
	
	return 0;
}

int getlink(char *link, CURL *curl, int ntry)
{
    FILE *pagefile;
    char *pagefilename;
    int ret;
    long dlenght = 0;
    curl_off_t existsize = 0;
    struct stat statbuf;
    struct myprogress prog; 
    
    interv_count = 0;

//     pagefilename = getfilename(curl, link);
    pagefilename = basename(link);
    pagefile = fopen("/dev/null", "w");

    if(ntry == 0)
        fprintf(stdout, "Getting '%s':\n", pagefilename);

    curl_easy_setopt(curl, CURLOPT_URL, link);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_RANGE, "0-1");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);

    
    ret = curl_easy_perform(curl);
//         printf("CURL: %d\n", ret);
    fclose(pagefile);
    
    if(manage_ret(curl, ret) == 1)
	return 0;
    
    curl_easy_reset(curl);
//     prog->filename = pagefilename_clean;
    
    if((stat(pagefilename, &statbuf) == 0))
    {
        existsize = statbuf.st_size;
        float kbsize = statbuf.st_size / 1024;
        float mbsize = kbsize / 1024;
        float gbsize = mbsize / 1024;
	
        pagefile = fopen(pagefilename, "a+");
        if(ntry == 0)
        {
            printf("Already downloaded: ");
            if(statbuf.st_size < 1024)
                printf("%5.1f B\n", (float)statbuf.st_size);
            if((kbsize < 1024) && (statbuf.st_size > 1024))
                printf("%5.1f kB\n", kbsize);
            if((kbsize > 1024) && (mbsize < 1024))
                printf("%5.1f mB\n", mbsize);
            if(mbsize > 1024)
                printf("%5.1f GB\n", gbsize);
        }
        
    }

    if(existsize > 0)
    {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE , existsize);
    }
    else
    {
//         curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE , 0);
        pagefile = fopen(pagefilename, "wb");
    }
    
    prog.startime = time(NULL);
    
    curl_easy_setopt(curl, CURLOPT_URL, link);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
    
    curl_easy_setopt(curl, CURLOPT_RANGE, NULL);
    ret = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_UPLOAD, &dlenght);

    if((existsize > 0) && (dlenght == 0) && (ret == 33))
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
        sprintf(my_cmd, "%s %s 2>/dev/null", editors_a[i].editor, file);
	
	retval = system(my_cmd);
	if(retval == 0)
	    break;
    }
    if(retval != 0)
    {
        fprintf(stderr, "None of the text editors I tried seem to be present on this system.\n");
	return -1;
    }
    return 0;
}
