#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// #include <process.h>

int edit(const char *file);

struct txteditors
{
    const char *editor;
};


int main()
{
    const char *file = "/tmp/dlmanagerlist";
    int editval;
    
    editval = edit(file);
    if(editval == -1)
    {
	fprintf(stderr, "unable to open file for edition.\n");
	return -1;
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