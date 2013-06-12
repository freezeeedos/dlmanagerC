



getlist(char *filename)
{
    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    FILE *listfile;
    FILE *pagefile;
    char *link;
    char *pagefilename;

    curl = curl_easy_init();
    prog.lastruntime = 0;
    prog.curl = curl;

    listfile = fopen(filename, "r");
    while(fgets(link, 10000, listfile) != EOF)
    {
	pagefilename = getfilename(link); //FONCTION A ECRIRE
	curl_easy_setopt(curl, CURLOPT_URL, link);

	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	pagefile = fopen(pagefilename, "wb");
	if (pagefile) {
	    curl_easy_setopt(curl, CURLOPT_FILE, pagefile);
	    curl_easy_perform(curl);
	    fclose(pagefile);
	}
    }

  /* cleanup curl stuff */
    curl_easy_cleanup(curl);
}