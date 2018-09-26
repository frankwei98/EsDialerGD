//
// Created by ChileungL on 23/05/2018.
//

/* HTTP请求 */

#include <stdio.h>
#ifndef USE_CURL_CMD
#include <curl/curl.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "http_req.h"

typedef struct {
    size_t buf_len;
    size_t pos;
    u_char *buf;
} CURL_UD;

#ifdef USE_CURL_CMD
char *curl_cmdline = NULL;
#endif

size_t curl_recv_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    CURL_UD *ud = userdata;
    size_t len = size * nmemb;
    if (ud->pos + len < ud->buf_len) {
        memcpy(ud->buf + ud->pos, ptr, len);
        ud->pos += len;
    }
    return len;
}

int http_req_send(const char *url,
                  char headers[][HEADER_LEN],
                  const char *post_data,
                  const char *buf,
                  size_t buf_len,
                  int incl_hdr) {
    int ret = -1;
#ifndef USE_CURL_CMD
    CURL *curl = curl_easy_init();
    if (curl) {
        CURL_UD ud = {buf_len, 0, (u_char *) buf};
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, !incl_hdr);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ud);
        curl_easy_setopt(curl, CURLOPT_HEADER, incl_hdr);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_recv_cb);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2);
        struct curl_slist *curl_headers = NULL;
        if (headers)
            for (int i = 0; headers[i][0]; ++i) {
                curl_headers = curl_slist_append(curl_headers, headers[i]);
            }
        if (curl_headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
        if (post_data) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        ret = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
#else
    if (!curl_cmdline) curl_cmdline = malloc(16 * 1024);
    memset(curl_cmdline, 0, sizeof(16 * 1024));
    strcpy(curl_cmdline, "curl");
    if (headers)
        for (int i = 0; headers[i][0]; ++i) {
            sprintf(curl_cmdline, "%s -H '%s'", curl_cmdline, headers[i]);
        }
    if (post_data) sprintf(curl_cmdline, "%s -d '%s'", curl_cmdline, post_data);
    if (!incl_hdr) sprintf(curl_cmdline, "%s -L", curl_cmdline);
    else sprintf(curl_cmdline, "%s -i", curl_cmdline);
    sprintf(curl_cmdline, "%s '%s'", curl_cmdline, url);
    FILE *fp = popen(curl_cmdline, "r");
    if (!fp) return ret;
    /*
    char curl_ret[2048] = {0};
    while (fgets(curl_ret, sizeof(curl_ret), fp)) {
        curl_recv_cb(curl_ret, strlen(curl_ret), 1, &ud);
        memset(curl_ret, 0, sizeof(curl_ret));
    }
     */
    fread((void*)buf,buf_len,1,fp);
    pclose(fp);
    ret = 0;
#endif
    return ret;
}
