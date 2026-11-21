/**
 * This is the driver for the gpios on the Metalink Dongle.
 *
 * Portions of this driver were modified from the scull sample char driver,
 * from the book: Linux Device Drivers, 2nd Edition,
 * which is distributed with the following copyrights:
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * This driver is:
 * Copyright (C) 2006 Metalink Ltd.
 * and must be distributed under the GPL:
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 * */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>

#include "gpioDevice.h"
#include <linux/netdevice.h>

#include <linux/timer.h>
#include <linux/str9100/str9100_gpio.h>

#define MAX_HW_GPIOS 21
#define SYSTEM_LED_GPIO 2

MODULE_DESCRIPTION("Metalink GPIO (LED/pushbutton) driver");
MODULE_AUTHOR("Copyright(c) 2008-2009 Metalink Ltd.");
MODULE_LICENSE("GPL");

/* An array defining what GPIO pins to initialize as output. */
static int outputLeds[MAX_GPIOS];
static int numOutputLeds;
module_param_array(outputLeds, int, &numOutputLeds, 0);

/* An array defining what GPIO pins to initialize as input. */
static int pushButtons[MAX_GPIOS];
static int numPushButtons;
module_param_array(pushButtons, int, &numPushButtons, 0);

static int pbc_flag[MAX_GPIOS];
static int major;
static spinlock_t gpio_lock = SPIN_LOCK_UNLOCKED;
static int gpio_unloading = 0;

static led_pattern_t ledPatterns[] = {
	/* 2. 1 second on, 1 second off */
	{
		-1,
		{_MS(1000), _MS(1000), -1}
	},
	/* 3. Blink_1: for 1 second: 0.1 on, 0.1 off, then 0.6 off */
	{
		-1,
		{_MS(100), _MS(100), _MS(100), _MS(100), _MS(100), _MS(100), _MS(100), _MS(100), _MS(100), _MS(600), -1}
	},
	/* 4. Blink_2: 0.1 on, 0.1 off */
	{
		-1,
		{_MS(100), _MS(100), -1}
	},
	/* 5. Traffic: 0.3 on, 0.3 off */
	{
		-1,
		{_MS(300), _MS(300), -1}
	},
	/* 6. WPS in progress: 0.2 on, 0.1 off */
	{
		-1,
		{_MS(200), _MS(100), -1}
	},
};

static const int NUM_PATTERNS = sizeof(ledPatterns) / sizeof(led_pattern_t);

/* Per-LED active pattern state */
static led_pattern_in_use_t ledPatternsUsed[MAX_GPIOS];

static DECLARE_WAIT_QUEUE_HEAD(wq);

static struct timer_list pulse_timers[MAX_GPIOS];
static struct timer_list pbc_timer[MAX_GPIOS];
static struct file_operations gpioFops = {
	owner:   THIS_MODULE,
	open:    gpio_open,
	release: gpio_close,
	write:   gpio_write,
	read:    gpio_read,
};

inline void gpio_set_off(unsigned long gpio)
{
	str9100_gpio_out_bit(GPIO_OFF, gpio);
}

inline void gpio_set_on(unsigned long gpio)
{
	str9100_gpio_out_bit(GPIO_ON, gpio);
}

/* Turn on/off gpio - state= 0 (gpio off), 1 (gpio on) */
void gpio_set(int gpio, int state)
{
	if (gpio < 0 || gpio >= MAX_HW_GPIOS)
		return;
	if (state == GPIO_ON)
		gpio_set_on(gpio);
	else
		gpio_set_off(gpio);
}

void pbc_validate_push(unsigned long pbc_wait_flag)
{
	u32 temp = GPIO_OFF;
	unsigned int timer_expires;
	unsigned int gpio = pbc_wait_flag >> 16;
	unsigned long pb_waiting_time;
	unsigned long flags;

	if (gpio >= MAX_HW_GPIOS)
		return;

	pbc_wait_flag &= 0xFFFF;
	timer_expires = MSECS_TO_JIFFIES(75) + jiffies;
	pbc_wait_flag++;
	str9100_gpio_in_bit(&temp, gpio);
	if (temp == GPIO_OFF) {
		spin_lock_irqsave(&gpio_lock, flags);
		if (!gpio_unloading) {
			str9100_gpio_set_edgeintr(&gpio_pbc_start, PIN_TRIG_SINGLE, PIN_TRIG_FAILING, gpio);
		}
		spin_unlock_irqrestore(&gpio_lock, flags);
		return;
	}

	if (gpio == RESET_DFT_GPIO) {
		pb_waiting_time = 12 * RESET_DFT_WAIT_SEC;
	} else {
		pb_waiting_time = 2;
	}

	if (pbc_wait_flag <= pb_waiting_time) {
		spin_lock_irqsave(&gpio_lock, flags);
		if (!gpio_unloading) {
			if (timer_pending(pbc_timer + gpio))
				del_timer(pbc_timer + gpio);
			init_timer(pbc_timer + gpio);
			pbc_timer[gpio].expires = timer_expires;
			pbc_timer[gpio].data = (gpio << 16) | pbc_wait_flag;
			pbc_timer[gpio].function = pbc_validate_push;
			add_timer(pbc_timer + gpio);
		}
		spin_unlock_irqrestore(&gpio_lock, flags);
	} else {
		if (gpio == RESET_DFT_GPIO) {
			printk("trigger RESET_DFT_GPIO\n");
		}
		gpio_pbc_end((int)gpio);
	}
}

void pbc_cancel(unsigned long gpio)
{
	unsigned long flags;

	if (gpio >= MAX_HW_GPIOS)
		return;
	spin_lock_irqsave(&gpio_lock, flags);
	pbc_flag[gpio] = 0;
	spin_unlock_irqrestore(&gpio_lock, flags);
}

void gpio_pbc_start(int gpio)
{
	unsigned long flags;

	if (gpio < 0 || gpio >= MAX_HW_GPIOS)
		return;
	str9100_gpio_clear_intr(gpio);

	spin_lock_irqsave(&gpio_lock, flags);
	if (gpio_unloading) {
		spin_unlock_irqrestore(&gpio_lock, flags);
		return;
	}
	spin_unlock_irqrestore(&gpio_lock, flags);

	pbc_validate_push((unsigned long)gpio << 16);
}

void gpio_pbc_end(int gpio)
{
	unsigned long flags;

	if (gpio < 0 || gpio >= MAX_HW_GPIOS)
		return;

	spin_lock_irqsave(&gpio_lock, flags);
	if (gpio_unloading) {
		spin_unlock_irqrestore(&gpio_lock, flags);
		return;
	}
	pbc_flag[gpio] = 2;
	str9100_gpio_clear_intr(gpio);
	str9100_gpio_set_edgeintr(&gpio_pbc_start, PIN_TRIG_SINGLE, PIN_TRIG_FAILING, gpio);

	if (timer_pending(pbc_timer + gpio)) {
		del_timer(pbc_timer + gpio);
	}
	init_timer(pbc_timer + gpio);
	pbc_timer[gpio].expires = SECS_TO_JIFFIES(20) + jiffies;
	pbc_timer[gpio].data = gpio;
	pbc_timer[gpio].function = pbc_cancel;
	add_timer(pbc_timer + gpio);
	spin_unlock_irqrestore(&gpio_lock, flags);

	wake_up_interruptible(&wq);
}

int gpio_init(void)
{
	int i, gpio;

	for (i = 0; i < MAX_GPIOS; i++) {
		init_timer(pulse_timers + i);
		init_timer(pbc_timer + i);
		ledPatternsUsed[i].pattern = NULL;
		ledPatternsUsed[i].curr_index = 0;
		ledPatternsUsed[i].curr_repetition = 0;
		pbc_flag[i] = 0;
	}

	for (i = 0; i < numOutputLeds && i < MAX_GPIOS; i++) {
		gpio = outputLeds[i];
		if (gpio < 0 || gpio >= MAX_HW_GPIOS)
			continue;
		str9100_gpio_write_direction_bit(PIN_OUTPUT, gpio);
		gpio_set(gpio, GPIO_OFF);
	}

	for (i = 0; i < numPushButtons && i < MAX_GPIOS; i++) {
		gpio = pushButtons[i];
		if (gpio < 0 || gpio >= MAX_HW_GPIOS)
			continue;
		str9100_gpio_write_direction_bit(PIN_INPUT, gpio);
		str9100_gpio_set_edgeintr(&gpio_pbc_start, PIN_TRIG_SINGLE, PIN_TRIG_FAILING, gpio);
	}

	return 0;
}

static ssize_t gpio_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
	int gpio = MINOR(file->f_dentry->d_inode->i_rdev);
	unsigned long flags;
	int val;

	if (gpio >= MAX_HW_GPIOS || gpio < 0)
		return (-ENODEV);
	if (count == 0)
		return 0;

	if (wait_event_interruptible(wq, pbc_flag[gpio] != 0 || gpio_unloading))
		return (-ERESTARTSYS);

	spin_lock_irqsave(&gpio_lock, flags);
	if (gpio_unloading) {
		spin_unlock_irqrestore(&gpio_lock, flags);
		return (-ENODEV);
	}
	val = pbc_flag[gpio];
	spin_unlock_irqrestore(&gpio_lock, flags);

	if (val > 0) {
		if (val > 1) {
			if (copy_to_user(buf, PBC_ON, 1)) {
				return (-EFAULT);
			}
			spin_lock_irqsave(&gpio_lock, flags);
			if (pbc_flag[gpio] > 1)
				pbc_flag[gpio]--;
			spin_unlock_irqrestore(&gpio_lock, flags);
			return 1;
		} else {
			spin_lock_irqsave(&gpio_lock, flags);
			pbc_flag[gpio] = 0;
			spin_unlock_irqrestore(&gpio_lock, flags);
			return 0;
		}
	} else {
		return (-EAGAIN);
	}
}

void led_pulse(unsigned long lednum)
{
	int *currIndex;
	int *currRepetition;
	int pulse_len;
	int ledStatus;
	led_pattern_t *currPattern;
	unsigned long flags;

	if (lednum >= MAX_HW_GPIOS)
		return;

	spin_lock_irqsave(&gpio_lock, flags);
	if (gpio_unloading) {
		spin_unlock_irqrestore(&gpio_lock, flags);
		return;
	}

	currPattern = ledPatternsUsed[lednum].pattern;
	if (currPattern == NULL) {
		spin_unlock_irqrestore(&gpio_lock, flags);
		return;
	}

	currIndex = &(ledPatternsUsed[lednum].curr_index);
	currRepetition = &(ledPatternsUsed[lednum].curr_repetition);

	/* If this is the last value in the pattern array (-1), start next repetition */
	if (currPattern->timer_vals[*currIndex] == -1) {
		*currIndex = 0;
		(*currRepetition)++;
	}

	/* Repeat forever if repetition_count is -1 */
	if (currPattern->repetition_count != -1 &&
	    ledPatternsUsed[lednum].curr_repetition == currPattern->repetition_count) {
		gpio_set((int)lednum, GPIO_OFF);
		ledPatternsUsed[lednum].pattern = NULL;
		spin_unlock_irqrestore(&gpio_lock, flags);
		return;
	}

	pulse_len = currPattern->timer_vals[*currIndex];

	init_timer(pulse_timers + lednum);
	pulse_timers[lednum].expires = pulse_len + jiffies;
	pulse_timers[lednum].data = lednum;
	pulse_timers[lednum].function = led_pulse;
	add_timer(pulse_timers + lednum);

	/* Toggle LED - even index ON, odd index OFF */
	ledStatus = (*currIndex & 0x1) ? GPIO_OFF : GPIO_ON;
	gpio_set((int)lednum, ledStatus);
	(*currIndex)++;
	spin_unlock_irqrestore(&gpio_lock, flags);
}

/*
 * Parse string LED/control commands safely without silent fallback to 0 (off):
 *   "green" / "on" -> 1 (LED ON)
 *   "off"          -> 0 (LED OFF)
 *   digits         -> parsed integer
 * Returns 0 on success, -EINVAL on empty/non-numeric garbage.
 */
static int string_to_int(const char *buf, int *out_val)
{
	const char *p;
	long val;
	int neg;
	int has_digit;

	p = buf;
	val = 0;
	neg = 0;
	has_digit = 0;

	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;

	if (strnicmp(p, "green", 5) == 0 && (p[5] == '\0' || p[5] == ' ' || p[5] == '\t' || p[5] == '\r' || p[5] == '\n')) {
		*out_val = 1;
		return 0;
	}
	if (strnicmp(p, "on", 2) == 0 && (p[2] == '\0' || p[2] == ' ' || p[2] == '\t' || p[2] == '\r' || p[2] == '\n')) {
		*out_val = 1;
		return 0;
	}
	if (strnicmp(p, "off", 3) == 0 && (p[3] == '\0' || p[3] == ' ' || p[3] == '\t' || p[3] == '\r' || p[3] == '\n')) {
		*out_val = 0;
		return 0;
	}

	if (*p == '-') {
		neg = 1;
		p++;
	} else if (*p == '+') {
		p++;
	}

	while (*p >= '0' && *p <= '9') {
		has_digit = 1;
		if (val > (LONG_MAX - (*p - '0')) / 10) {
			val = LONG_MAX;
			break;
		}
		val = val * 10 + (*p - '0');
		p++;
	}
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;

	if (!has_digit || *p != '\0')
		return (-EINVAL);

	*out_val = neg ? (int)(-val) : (int)val;
	return 0;
}

static ssize_t gpio_write(struct file *file, const char *buf, size_t count, loff_t *offset)
{
	char kbuf[16];
	int gpioval, pat_idx, gpio;
	size_t orig_count = count;
	unsigned long flags;

	gpio = MINOR(file->f_dentry->d_inode->i_rdev);

	if (gpio >= MAX_HW_GPIOS || gpio < 0)
		return (-ENODEV);

	if (count >= sizeof(kbuf))
		count = sizeof(kbuf) - 1;

	if (copy_from_user(kbuf, buf, count))
		return (-EFAULT);

	kbuf[count] = '\0';

	if (string_to_int(kbuf, &gpioval) < 0)
		return (-EINVAL);

	if (gpioval != 0 && gpioval != 1) {
		pat_idx = gpioval - 2;
		if (pat_idx < 0 || pat_idx >= NUM_PATTERNS)
			return (-EINVAL);
	} else {
		pat_idx = -1;
	}

	/* Disable any pending timer under lock before updating state */
	spin_lock_irqsave(&gpio_lock, flags);
	if (timer_pending(pulse_timers + gpio)) {
		del_timer(pulse_timers + gpio);
	}
	ledPatternsUsed[gpio].pattern = NULL;
	ledPatternsUsed[gpio].curr_index = 0;
	ledPatternsUsed[gpio].curr_repetition = 0;

	if (gpioval == 0) {
		gpio_set_off(gpio);
		spin_unlock_irqrestore(&gpio_lock, flags);
		if (gpio == SYSTEM_LED_GPIO) {
			stop_blinking_SYS_LED(1);
		}
	} else if (gpioval == 1) {
		gpio_set_on(gpio);
		spin_unlock_irqrestore(&gpio_lock, flags);
	} else {
		ledPatternsUsed[gpio].pattern = &(ledPatterns[pat_idx]);
		spin_unlock_irqrestore(&gpio_lock, flags);
		led_pulse((unsigned long)gpio);
	}

	return (ssize_t)orig_count;
}

static int gpio_open(struct inode *node, struct file *file)
{
	return 0;
}

void gpio_cleanup(void)
{
	int i, gpio;
	unsigned long flags;

	spin_lock_irqsave(&gpio_lock, flags);
	gpio_unloading = 1;
	/* Disable/clear all pushbutton interrupts before deleting timers */
	for (i = 0; i < numPushButtons && i < MAX_GPIOS; i++) {
		gpio = pushButtons[i];
		if (gpio >= 0 && gpio < MAX_HW_GPIOS) {
			str9100_gpio_clear_intr(gpio);
			pbc_flag[gpio] = 0;
		}
	}
	spin_unlock_irqrestore(&gpio_lock, flags);

	/* Sync-delete ALL timers across 0..MAX_GPIOS-1 */
	for (i = 0; i < MAX_GPIOS; i++) {
		del_timer_sync(pulse_timers + i);
		del_timer_sync(pbc_timer + i);
	}

	/* Turn all initialized LEDs off and clear any late interrupts again */
	for (i = 0; i < numOutputLeds && i < MAX_GPIOS; i++) {
		gpio = outputLeds[i];
		if (gpio >= 0 && gpio < MAX_HW_GPIOS) {
			gpio_set(gpio, GPIO_OFF);
		}
	}
	for (i = 0; i < numPushButtons && i < MAX_GPIOS; i++) {
		gpio = pushButtons[i];
		if (gpio >= 0 && gpio < MAX_HW_GPIOS) {
			str9100_gpio_clear_intr(gpio);
		}
	}
	wake_up_interruptible(&wq);
}

static int gpio_close(struct inode *node, struct file *file)
{
	return 0;
}

static int __init gpio_init_module(void)
{
	printk("<1>Loaded GPIO driver \n");

	SET_MODULE_OWNER(&gpioFops);

	major = register_chrdev(DEVICE_NUM, MODULE_NAME, &gpioFops);
	if (major < 0) {
		printk(KERN_ERR "Can't register %d\n ", DEVICE_NUM);
		return major;
	} else {
		printk("registered %d\n", major);
	}
	printk("Num LEDs to init %d\n", numOutputLeds);
	gpio_init();
	return 0;
}

static void __exit gpio_cleanup_module(void)
{
	if (unregister_chrdev(major > 0 ? major : DEVICE_NUM, MODULE_NAME) < 0) {
		printk(KERN_ERR "Can't unregister %s\n", MODULE_NAME);
	}

	gpio_cleanup();

	printk("<1>Successfully removed gpio driver\n");
}

module_init(gpio_init_module);
module_exit(gpio_cleanup_module);
