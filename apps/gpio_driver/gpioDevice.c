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
/** ********************************************************************
*   Push button -     1. After the PBC button is pushed an interrupt is triggered
					  2. If the button is pushed for more than 1 sec. the pbc_flag
						 is set and the blocking read is released from it's
						 wait  state.
					  3. Meanwhile from the user space a blocking read ('cat') is
						 called and waiting for a EOF.
					  4. 'EOF' will be written only when pbc_flag is set after PBC
						 is triggered.
					  5. The flag will be set back to is default value ('No button is
						 pushed'= 0)after 20 seconds, if the user space didn't
						 take care of the push button ( 'gpio_read' was not 
						 called).
*/
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/uaccess.h> 

#include "gpioDevice.h"
#include <linux/netdevice.h> 

#include <linux/timer.h>
#include <linux/str9100/str9100_gpio.h>


MODULE_DESCRIPTION("Metalink GPIO (LED/pushbutton) driver");
MODULE_AUTHOR("Copyright(c) 2008-2009 Metalink Ltd.");
MODULE_LICENSE("GPL");


static unsigned char Buffer[BUFFER_SIZE];

/**
	Module parameters: 
		outputLeds - list of GPIOs to init for output, e.g. outputLeds=0,3,4
		pushButtons - list of GPIOs to init for input, e.g. pushButtons=1,13
*/

/* An array defining what GPIO pins to initialize as output. This is passed when insmoding the module */
static int outputLeds[MAX_GPIOS];
static int numOutputLeds;
module_param_array(outputLeds, int, &numOutputLeds, 0);

/* An array defining what GPIO pins to initialize as input. This is passed when insmoding the module */
static int pushButtons[MAX_GPIOS];
static int numPushButtons;
module_param_array(pushButtons, int, &numPushButtons, 0);


static int pbc_flag[MAX_GPIOS];
static int major;
int pulse_pattern;

/* ledPatterns - An array defining all possible LED patterns in the system. 
 * To pulse a gpio with a specific pattern, write the value (index + 2) to the appropriate LED device,
 * e.g. echo 2 > /dev/led1   
 *   will toggle the first pattern on led1 
 *
 *  Currently, all patterns are defined to repeat forever (repetition_count=-1), and must be stopped by the user.
 *   */
static led_pattern_t ledPatterns[] = {
	// 2. 1 second on, 1 second off
	{
		-1,
		{_MS(1000),_MS(1000),-1}
	},
	// 3. Blink_1: Repeat 120 seconds: for 1 second: 0.1 on, 0.1 off, then 0.5 off
	{
		-1,		// 80 for 120 seconds
		{_MS(100),_MS(100),_MS(100),_MS(100),_MS(100),_MS(100),_MS(100),_MS(100),_MS(100),_MS(600),-1}
	},
	// 4. Blink_2: Repeat 120 seconds: 0.1 on, 0.1 off
	{
		-1,		// 600 for 120 seconds
		{_MS(100),_MS(100),-1}
	},
	// 5. Traffic: 0.3 on, 0.3 off
	{
		-1,
		{_MS(300),_MS(300),-1}
	},
	// 6. WPS in progress: 0.2 on, 0.1 off
	{
		-1,
		{_MS(200),_MS(100),-1}
	},
};

static const int NUM_PATTERNS = sizeof(ledPatterns) / sizeof(led_pattern_t);


/* An array of led patterns , to connect the led with the pattern currently being used for it. */
static led_pattern_in_use_t ledPatternsUsed[MAX_GPIOS];


// TODO: Do you need one wq per pushbutton? Or is it ok to reuse the same queue, because we 
// always wait with an exit condition
static DECLARE_WAIT_QUEUE_HEAD(wq);

/* timers for the leds and pushbuttons - There are is a timer for each led - needed for looping the pulse function 
 * and a timer for each pushbutton, to make sure it was pushed long enough. */ 
// TODO: To reduce data size, the two timer lists can be united to one - each gpio can be only a led or a PB
// Currently, I left both lists for clarity
static struct timer_list pulse_timers[MAX_GPIOS];
static struct timer_list pbc_timer[MAX_GPIOS];
static struct file_operations gpioFops = {
	owner: 	 THIS_MODULE,
	open: 	 gpio_open,
	release: gpio_close,
	write: 	 gpio_write,
	read: 	 gpio_read,
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
    if (state == GPIO_ON)
		gpio_set_on(gpio);
	else 
		gpio_set_off(gpio);	
}

/*
pbc_validate_push() - The interrupt handler for push button.
Make sure that the pushbutton was pressed long enough

The GPIO number of the PB is passed in the upper 16 bits of the data.
The wait flag is in the lower 16 bits.

1. When the pbc button is pressed (GPIO_ON) -> set the start time.
2. If the button is released - Check the the pbc was held for more than one 
	second and than set the pbc_flag to '1' and release the wait state of the
    gpio_read() .
   This is done by triggering a timer and calling this func again twice.

PBC state machine:
    ----> start ----> wait ----> wait ----> wait ----> end
  intr(fall)    call       timer      timer      call     
*/
void pbc_validate_push(unsigned long pbc_wait_flag) 
{
	u32 temp;
	unsigned int timer_expires;
	unsigned int gpio = pbc_wait_flag >> 16;
	unsigned long pb_waiting_time;  //Tim Wang, push button waiting time
	pbc_wait_flag &= 0xFFFF;
	timer_expires = MSECS_TO_JIFFIES(75)+jiffies; // 150 msec total.
	pbc_wait_flag++;
	str9100_gpio_in_bit(&temp, gpio);
	if (temp == GPIO_OFF)
	{
		str9100_gpio_set_edgeintr(&gpio_pbc_start, PIN_TRIG_SINGLE, PIN_TRIG_FAILING, gpio);
		return;
	}

    if (timer_pending(pbc_timer + gpio))
    {
		/* Stop any pending timer - it might be a previous wait-for-cancel timer */ 
    	del_timer(pbc_timer + gpio); 
    }
	//Tim Wang, for reset to facotry default, the waiting time will be longer than PBC.
	if (gpio == RESET_DFT_GPIO) {
		pb_waiting_time = 12 * RESET_DFT_WAIT_SEC;
	} else {
		pb_waiting_time = 2;
	}
	//printk("pbc_wait_flag=%d\n",pbc_wait_flag);
	if (pbc_wait_flag <= pb_waiting_time)
	{
		init_timer(pbc_timer + gpio);
		pbc_timer[gpio].expires = timer_expires;
		pbc_timer[gpio].data = (gpio << 16) | pbc_wait_flag;
		pbc_timer[gpio].function = pbc_validate_push;
   		add_timer(pbc_timer + gpio);
	} else 
	{
		// Do you want to leave the edge trigger interrupt, or start handling the PB as soon 
		// as it was pressed one second? 
		//   --> For SW-triggered restore defaults, handling immediately is necessary
		///OLD: str9100_gpio_set_edgeintr(&gpio_pbc_end, PIN_TRIG_SINGLE, PIN_TRIG_RISING, gpio);

		//Tim Wang, for reset to factory default
		if (gpio == RESET_DFT_GPIO) {
			printk("trigger RESET_DFT_GPIO\n");
		}

		gpio_pbc_end(gpio);
	}
}

/********************************************
* pbc_cancel - If the PBC is pushed and the user appl. do not get an indication
* 				for that in 20 second the pbc_flag is reset and the PBC is *					canceled.
********************************************/
void pbc_cancel(unsigned long gpio)
{
		pbc_flag[gpio]=0;
}

void gpio_pbc_start(int gpio)
{
		str9100_gpio_clear_intr(gpio);

		// First time to call 'pbc_validate_push'
		// Pass gpio in upper 16 bits, pass 0 in lower
		pbc_validate_push(gpio << 16); 
}

/* gpio_pbc_end:
 * Set flag to notify that PB was pushed. 
 * Wake up any readers waiting for data. 
 * Restart interrupt to be ready next press of PB.
 * Start interrupt to cancel the reading of this PB, if it isn't handled within 20 seconds */
void gpio_pbc_end(int gpio)
{
	pbc_flag[gpio]=2; 
	wake_up_interruptible(&wq);
	str9100_gpio_clear_intr(gpio);
	str9100_gpio_set_edgeintr(&gpio_pbc_start, PIN_TRIG_SINGLE, PIN_TRIG_FAILING, gpio);

    if (timer_pending(pbc_timer + gpio))
    {
    	del_timer(pbc_timer + gpio); 
    }
	/* Wait 20 seconds before the current pbc is canceled */
	init_timer(pbc_timer + gpio); 
   	pbc_timer[gpio].expires = SECS_TO_JIFFIES(20) + jiffies; 
	pbc_timer[gpio].data = gpio;
	pbc_timer[gpio].function = pbc_cancel;
	add_timer(pbc_timer + gpio);
}

/*
gpio_init() - 
	1. Set the gpio direction bits
	2. Set the interrupt handler for Push Button
	3. If The Restore Defualt button is pushed (gpio value is 'GPIO_ON') for more
	    than 2 seconds after the gpio init, the interrupt handler for 
		Restore Default is set. 
*/
int gpio_init(void)
{
	int i, gpio;
	
	for (i = 0; i < numOutputLeds && i < MAX_GPIOS; i++) {
		gpio = outputLeds[i];
		str9100_gpio_write_direction_bit( PIN_OUTPUT, gpio);
		gpio_set(gpio, GPIO_OFF);

		pulse_timers[gpio].expires = 0;
		ledPatternsUsed[gpio].pattern = NULL;
		ledPatternsUsed[gpio].curr_index = 0;
		ledPatternsUsed[gpio].curr_repetition = 0;

		//printk("Initialized LED %d\n", gpio);
	}

	for (i = 0; i < numPushButtons && i < MAX_GPIOS; i++) {
		gpio = pushButtons[i];

		str9100_gpio_write_direction_bit( PIN_INPUT, gpio);
		str9100_gpio_set_edgeintr(&gpio_pbc_start, PIN_TRIG_SINGLE, PIN_TRIG_FAILING, gpio);
	}
	
	return 0;
}



/********************************************
 * GPIO Read:  
 *	PBC: The read is blocked (wait_event_interruptible) until the PBC button
		  is pushed. If the read is 'released' it returns with 
		  'EOF' ('return 0') only.
 * Restore Defualt:  Check rdflt_flag and returns 'on' or 'off' accordinally.
 *
 * Both buttons write one byte and return, and then return 0 for EOF.
 * 
 ********************************************/
static ssize_t gpio_read(struct file *file,  char *buf, size_t count, loff_t *offset)
{
	int gpio = MINOR(file->f_dentry->d_inode->i_rdev);
	if (gpio >= MAX_GPIOS || gpio < 0)
		return (-ENODEV);
	/* If pbc button is pushed PBC_ON is copied to /dev/pbc0. BLOCK until it is pressed. */
	// TODO: Expand wq to an array??
	wait_event_interruptible(wq,pbc_flag[gpio]!=0);
	if (pbc_flag[gpio] > 0)
	{
		if (pbc_flag[gpio] >1)
		{
			if (copy_to_user(buf,PBC_ON, 1))
			{  
				return (-ENOMEM);
			}
			pbc_flag[gpio]--;
			return 1;
		} else {
			pbc_flag[gpio]--;
			return 0;
		}	
	} else 
	{
		return 0;			
	}
}

/********************************************
 * led_pulse() - Toggle the Led ON /OFF according to a pattern
 * The pattern is defined by an array of timer values - each value triggers the LED on or off for 0.1*timer_val seconds
 * In addition, each pattern must be repeated a predefined amount of times
 *
 *
 * Use the pattern to set the duration, and use the index for the toggle on/off
 * if -1, then start pattern from beginning, incr. repetition count, start again if needed
 *
 * ledPatterns is an array of all the possible patterns
 * ledPatternsUsed is an array of what pattern is used for a specific GPIO, and what stage of displaying the pattern it is in
********************************************/
void led_pulse(unsigned long lednum) 
{
	int *currIndex;
	int *currRepetition;
	int pulse_len;
	int ledStatus;
	led_pattern_t* currPattern;

	// If this is the first call to led_pulse, initialize the used pattern structure
	if (ledPatternsUsed[lednum].pattern == NULL) {
		///printk("Initializing pattern\n");
		ledPatternsUsed[lednum].pattern = &(ledPatterns[pulse_pattern]);
		ledPatternsUsed[lednum].curr_index = 0;
		ledPatternsUsed[lednum].curr_repetition = 0;
	}
	
	// Define some local variables to avoid having to work with long levels of redirection
	currPattern = ledPatternsUsed[lednum].pattern;
	currIndex = &(ledPatternsUsed[lednum].curr_index);
	currRepetition = &(ledPatternsUsed[lednum].curr_repetition);

	
	
	/* If this is the last value in the pattern array (timer_val is -1), start the next repetition */
	if (currPattern->timer_vals[*currIndex] == -1) {
		///printk("End of pattern\n");
		*currIndex = 0;
		(*currRepetition)++;

	}
	/* If the last repetition was completed, turn off led and don't renew timer. 
	 * Repeat forever if repetition count is -1  */
	if (currPattern->repetition_count != -1 && ledPatternsUsed[lednum].curr_repetition == currPattern->repetition_count) 
	{
		///printk("End of repetitions\n");
		gpio_set(lednum, GPIO_OFF);
		// On last pulse: clear pattern, (zeroing indices is done when setting a new pattern)
		ledPatternsUsed[lednum].pattern = NULL;
		return;
	}
	
	// Calculate the duration of the current led pulse
	///printk("LED Duration: %d\n",  currPattern->timer_vals[*currIndex]);
	// (To save realtime processing time, the MACRO MSECS_TO_JIFFIES was
	// moved to the definition of the patterns array.)
	pulse_len = currPattern->timer_vals[*currIndex];

    // Add a timer to call the function again, after pulse_len time
    init_timer(pulse_timers + lednum);
    pulse_timers[lednum].expires = pulse_len + jiffies;
    pulse_timers[lednum].data = lednum;
    pulse_timers[lednum].function = led_pulse;
    add_timer(pulse_timers + lednum);
	
    /* Toggle LED - use index: on even index LED is on, on odd index it is off */
    ledStatus = (*currIndex & 0x1) ? GPIO_OFF : GPIO_ON;
	gpio_set(lednum, ledStatus);
	(*currIndex)++;
}


/********************************************
 * GPIO Write: turn a GPIO on/off or flash it
 * The GPIO number is passed as the device number,
 * and the value to write is passed in the buf.
 * 
 ********************************************/
static ssize_t gpio_write(struct file *file, const char *buf, size_t count, loff_t *offset)
{
	
	int gpioval, gpio;
	// Get the led number from the device MINOR number
	gpio = MINOR(file->f_dentry->d_inode->i_rdev);

	if (gpio >= MAX_GPIOS || gpio < 0)
		return (-ENODEV);
	if(copy_from_user(Buffer, buf, count))
		return (-ENOMEM);
	// Parse passed argument
	gpioval = simple_strtoul(Buffer, NULL, 10);
	// First disable any pending timer, to stop traffic simulation
	if (timer_pending(pulse_timers+gpio))
	{
		del_timer(pulse_timers+gpio);
		ledPatternsUsed[gpio].pattern = NULL;
	}

	// Write the value to GPIO, using the device minor number
	if (gpioval == 0) {
		gpio_set_off(gpio);

		//Tim Wang, stop the system LED blinking
#define SYSTEM_LED_GPIO  2
		if (gpio == SYSTEM_LED_GPIO) {
			stop_blinking_SYS_LED(1);
		}
		
	} else if (gpioval == 1) {
		gpio_set_on(gpio);
	} else  {
		/* pulse_pattern is the index to the LED pattern (after subtracting 2) */
		pulse_pattern = gpioval - 2;
		if (pulse_pattern >= NUM_PATTERNS) {
			return (-EINVAL);
		}
		led_pulse(gpio); 
	} 
	return count;
}

/********************************************
 * Open the GPIO file: mark it as in use
 ********************************************/
static int gpio_open(struct inode *node, struct file *file)
{
    //int gpionum = MINOR(inode->i_rdev);
	// MOD_INC_USE_COUNT;	
	return 0;
}
void gpio_cleanup(void)
{
	int i, gpio;
	for (i = 0; i < numOutputLeds && i < MAX_GPIOS; i++) {
		/* disable any pending timer */
		//if (timer_pending(pulse_timers+outputLeds[i]))
		//{
			del_timer(pulse_timers+outputLeds[i]);
		//}

		/* turn all leds off */
		gpio_set(outputLeds[i], GPIO_OFF);
	}
	// Loop on valid PBs:
	/* Release the blocked reads, timers and interrupts */
	for (i = 0; i < numPushButtons && i < MAX_GPIOS; i++) {
		gpio = pushButtons[i];
		pbc_flag[gpio]=0; 
		if (timer_pending(pbc_timer + gpio)) {
			del_timer(pbc_timer + gpio);
		}
		str9100_gpio_clear_intr(gpio);
	}
	wake_up_interruptible(&wq);
}

/********************************************
 * Close the GPIO file: decrement the use count
 ********************************************/
static int gpio_close(struct inode *node, struct file *file)
{
	// MOD_DEC_USE_COUNT;
  	return 0;
}
/********************************************
 * Module init function
 ********************************************/
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
	// Init GPIOs (and turn on power GPIO)
	gpio_init();
	return 0;

  
}

/********************************************
 * Module cleanup function
 ********************************************/
static void __exit gpio_cleanup_module(void)
{
	/* disable any pending timer */

	/* Do GPIOs cleanup*/
	gpio_cleanup();

	/* Unregister the device*/	
	if (unregister_chrdev(DEVICE_NUM, MODULE_NAME) < 0) {
		printk(KERN_ERR "Can't unregister %s", MODULE_NAME);
		return ;
	}
	printk("<1>Successfully removed gpio driver\n");
}

module_init(gpio_init_module);
module_exit(gpio_cleanup_module);


