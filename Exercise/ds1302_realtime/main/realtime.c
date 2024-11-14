#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include "realtime.h"

int month_day(realtime_t t)
{
    switch (t.month)
    {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        return 31;
    case 4: case 6: case 9: case 11:
        return 30;
    case 2:{
        if (((t.year % 4 == 0) && (t.year % 100 != 0)) || (t.year % 400 == 0))
        {
            return 29;
        }
        return 28;
    }
    default: return 30;
    }
}

int time_check(realtime_t t)
{
    if (t.sec >= 60)
    {
        return SEC_ERR;
    }
    else if (t.min >= 60)
    {
        return MIN_ERR;

    }
    else if (t.hour >= 24)
    {
        return HOUR_ERR;    
    }
    else if (t.month > 12)
    {
        return MONTH_ERR;
    }
    else if (t.day > month_day(t))
    {
        return DAY_ERR;
    }
    else
    {
    return VALID_TIME;
    }
}

void time_normalize(realtime_t *t, time_err_t err)
{
    switch (err)
    {
    case SEC_ERR:
    {
        t -> min += t -> sec / 60;
        t -> sec = t -> sec % 60;
        break;
    }

    case MIN_ERR:
    {
        t -> hour += t -> min / 60;
        t -> min = t -> min % 60;
        break;
    }

    case HOUR_ERR:
    {
        t -> day += t -> hour / 24;
        t -> hour = t -> hour % 24;
        break;
    }

    case DAY_ERR:
    {
        day_modify(t);
        break;
    }

    case MONTH_ERR:
            t->year += t->month / 12;
            t->month = t->month % 12;
            if (t->month == 0) {
                t->month = 12;
                t->year -= 1;
            }
            break;

    case VALID_TIME: break;
    }
}

void day_modify(realtime_t *t)
{
    int plus_month = 0;
    while (t -> day > month_day(*t))
    {
        switch (t -> month)
        {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12:
            plus_month += 1;
            t -> day = t -> day - 31;
            break;
        
        case 4: case 6: case 9: case 11:
            plus_month += 1;
            t -> day = t -> day - 30;
            break;

        case 2:
            {
            plus_month += 1;
            if (((t->year % 4 == 0) && (t->year % 100 != 0)) || (t->year % 400 == 0))
            {
            t -> day = t -> day - 29;
            }else{
            t -> day = t -> day - 28;
            }
            break;
            }
        }
    }
    t -> month += plus_month;
}

void time_modify(realtime_t *t)
{
    while(time_check(*t) != 0)
    {
        time_normalize(t, time_check(*t));
    }
}

void time_print(realtime_t t)
{
    ESP_LOGI("TIME PRINT", "Day %d/%d/%d, %d:%d:%d", t.day, t.month, t.year, t.hour, t.min, t.sec);
}