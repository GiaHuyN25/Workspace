#ifndef __HTTP_SERVER_APP_H
#define __HTTP_SERVER_APP_H

#include <stdint.h>

typedef void (*http_post_callback_t) (char *dta, int len);
typedef void (*http_get_callback_t) (void);

void start_webserver(void);
void stop_webserver(void);

void get_data_response(char *data, int len);

void http_set_callback_slider(void *cb);
void http_set_callback_switch(void *cb);
void http_set_callback_wifi(void *cb);
void http_set_callback_get_data(void *cb);

#endif