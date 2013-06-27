#ifndef _DLMANAGER_
#define _DLMANAGER_

#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3
#define NTRYMAX 50


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

static size_t write_data(void *ptr, 
                         size_t size, 
                         size_t nmemb, void *stream);
static int progress(void *p,
                    double dltotal, double dlnow,
                    double ultotal, double ulnow);
char *getfilename(CURL *curl, char *link);
int getlist(const char *filename);
int getlink(char *link, 
            struct myprogress 
            prog, CURL *curl, int ntry);
int edit(const char *file);

#endif
