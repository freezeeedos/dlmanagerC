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
            *prog, CURL *curl, int ntry);
int edit(const char *file);

#endif
