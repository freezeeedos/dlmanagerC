#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



char *getfilename(char *link)
{
    int i;
    int j;
    int k = -1;
    int s = 0;
    int mark;
    int newlenght;
    char name[1000];
    char *nameret;
   
    for(i=0;link[i] != '\0';i++)
    {
	if(link[i] == '/')
	{
	    s++;
	    mark = i;
	}
    }
    printf("%d\n", i);
    printf("%d\n", s);
    printf("marker at %d\n", mark);
    newlenght = i - mark;
    printf("Newlenght is %d\n", newlenght);
    for(j=mark+1;j<i;j++)
    {
	k++;
	printf("name[%d] = link[%d]\n", k, j);
	name[k] = link[j];
    }
    name[k+1] = '\0';
    printf("%s\n", name);
    nameret = name;
    return nameret;
}

int main()
{
    char *url = "https://dl.dropboxusercontent.com/u/19711116/TT.tar.gz";
    char *name = NULL;
    
    name = getfilename(url);
    printf("name is '%s'\n", name);
    return 0;
}
