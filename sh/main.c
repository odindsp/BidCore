
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

size_t recv_post(void *buf, size_t size, size_t nmemb, void *stream)
{
    fprintf(stdout, "recv_post\n");
    return size * nmemb;
}

int send_post(char *url, char *buf)
{

    CURL *curl = curl_easy_init();
    if(curl == NULL)
	return -1;

    curl_easy_setopt(curl, CURLOPT_POST, 1); 
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1); 
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buf);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "x-openrtb-version: 2.3.1");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, recv_post);
    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, recv_buf);

    CURLcode ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return ret;
}

int main()
{

    char buf[4096] = {0};
    FILE *fp = fopen("./buf", "r");
    fread(buf, 1, 4096, fp);
    // fprintf(stdout, "%s\n", buf);

    int ret = send_post("http://127.0.0.1/e", buf);
    // int ret = send_post("http://115.182.205.166/o", buf);
    fprintf(stdout, "ret=%d\n", ret);

    return 0;
}



