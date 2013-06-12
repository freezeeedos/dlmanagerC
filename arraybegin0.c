#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main()
{
    char name[] = "aa";
    
    printf("%s\n", name);
    
    name[1] = '\0';
    printf("%s\n", name);
    
    return 0;
}