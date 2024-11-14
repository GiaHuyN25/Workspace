#ifndef REALTIME_H
#define REALTIME_H
#include "esp_err.h"
#include "hal/gpio_types.h"

typedef enum{
    VALID_TIME = 0,
    MONTH_ERR = 1,
    DAY_ERR = 2,
    HOUR_ERR = 3,
    MIN_ERR = 4,
    SEC_ERR = 5,
}time_err_t;

typedef struct realtime{
    int year, month, day, hour, min, sec;
}realtime_t;

int month_day(realtime_t t);
int time_check(realtime_t t);
void time_normalize(realtime_t *t, time_err_t err);
void day_modify(realtime_t *t);
void time_modify(realtime_t *t);
void time_print(realtime_t t);

#endif