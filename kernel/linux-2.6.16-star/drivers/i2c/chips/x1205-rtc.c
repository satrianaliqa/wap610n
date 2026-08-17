/*******************************************************************************
 *
 *
 *   Copyright(c) 2003 - 2006 Star Semiconductor Corporation, All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *   more details.
 *
 *   You should have received a copy of the GNU General Public License along with
 *   this program; if not, write to the Free Software Foundation, Inc., 59
 *   Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   The full GNU General Public License is included in this distribution in the
 *   file called LICENSE.
 *
 *   Contact Information:
 *   Technology Support <tech@starsemi.com>
 *   Star Semiconductor 4F, No.1, Chin-Shan 8th St, Hsin-Chu,300 Taiwan, R.O.C
 *
 ********************************************************************************/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/mc146818rtc.h>
#include <linux/init.h>
#include <linux/device.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/rtc.h>

#include <asm/mach/time.h>

#include <linux/x1205.h>





static int i2c_x1205_rtc_read_time(struct rtc_time *tm)
{
        unsigned long time;

	tm->tm_year+=100; // only 0-99
	x1205_do_command(X1205_CMD_GETDATETIME, tm);
	//printk("X1205_CMD_GETDATETIME: sec: %d\n", tm->tm_sec);
        return 0;
}

static inline int i2c_x1205_rtc_set_time(struct rtc_time *tm)
{
	//printk("x1205_do_command(X1205_CMD_SETTIME, tm);\n");
	
	//tm->tm_year-=100; // only 0-99
#if 0
	printk("in i2c_x1205_rtc_set_time %s: secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__FUNCTION__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);
#endif
	//x1205_do_command(X1205_CMD_SETTIME, tm);
	x1205_do_command(X1205_CMD_SETDATETIME, tm);

        return 0;
}

static inline int i2c_x1205_rtc_set_alarm(struct rtc_wkalrm *alrm)
{
        return 0;
}

static inline int i2c_x1205_rtc_read_alarm(struct rtc_wkalrm *alrm)
{
        return 0;
}

static int i2c_x1205_set_rtc(void)
{
        unsigned long record;

        return 1;
}




static struct rtc_ops i2c_x1205_rtc_ops = {
        .owner          = THIS_MODULE,
        .read_time      = i2c_x1205_rtc_read_time,
        .set_time       = i2c_x1205_rtc_set_time,
        .read_alarm     = i2c_x1205_rtc_read_alarm,
        .set_alarm      = i2c_x1205_rtc_set_alarm,
};

extern int (*set_rtc)(void);


int i2c_x1205_rtc_init(void)
{
        int ret;

        ret = register_rtc(&i2c_x1205_rtc_ops);
	if (ret) return ret;

        //set_rtc = i2c_x1205_set_rtc;

#if 0
        i2c_x1205_rtc_hw_init();


        // set RTC clock from system time
        i2c_x1205_set_rtc();

        ret = register_rtc(&i2c_x1205_rtc_ops);
        if (ret) return ret;

        set_rtc = i2c_x1205_set_rtc;

        RTC_SECOND_ALARM_REG    = 0;
        RTC_MINUTE_ALARM_REG    = 0;
        RTC_HOUR_ALARM_REG      = 0;

        // enable RTC
        HAL_RTC_ENABLE();
#endif
        return 0;
}

//module_init(i2c_x1205_rtc_init);

