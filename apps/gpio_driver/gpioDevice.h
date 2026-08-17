#ifndef GPIO_DEVICE_H
#define GPIO_DEVICE_H



/* IMPLEMENTING PULSING LEDS WITH PATTERNS
	array of on-off timers, in granularity of millisec. 
		e.g. {300,100,300,100,300,100,300,600,-1}:  on for 0.3, off 0.1, on 0.3.... off 0.6, -1==end of pattern
	Also have repitition count for each pattern. - how many times should the pattern be repeated, or -1 to repeat forever
*/

// TODO: Change to the number of GPIO pins on STAR
#define MAX_GPIOS 32

//Tim Wang, define the GPIO PIN and waiting time for reset to factory default
#define RESET_DFT_GPIO  3
#define RESET_DFT_WAIT_SEC  3
#define GPIO_ON  PIN_TRIG_HIGH
#define GPIO_OFF PIN_TRIG_LOW

// Macros that convert a number from msecs/secs to jiffies, for setting the kernel timers:
#define MSECS_TO_JIFFIES(X) ((X) * HZ / 1000)
#define SECS_TO_JIFFIES(X) ((X) * HZ )
// Shorthand form of the macro:
#define _MS(X) ((X) * HZ / 1000)

#define PBC_ON 		"1"
#define BUFFER_SIZE 512 

// Device 42 is available for testing
#define DEVICE_NUM 42
#define MODULE_NAME "mtlk_gpio"

/* Struct to define LED pulse patterns
 * array of on-off timers, in granularity of msec. 
 * The value written is atcually in jiffies, so each msec value must be wrapped in the _MS macro.
 * 	e.g. {_MS(300),_MS(100),_MS(300),_MS(600),-1}:  on for 0.3 sec, off 0.1, on 0.3, off 0.6, -1==end of pattern
 * Also: repitition count for each pattern. - how many times should the pattern be repeated, or -1 to repeat forever
 * */
#define MAX_LED_PATTERN 16
typedef struct {
	int repetition_count;
	int timer_vals[MAX_LED_PATTERN];
} led_pattern_t;

/* Struct to define a led pattern that is currently in use by a led
 * pattern - ptr to a led pattern that defines how the leds should blink
 * curr_index - what pattern to display next in the array
 * curr_repetition - what repetition number we are currently in
 * */
typedef struct {
	led_pattern_t* pattern;
	int curr_index;
	int curr_repetition;
} led_pattern_in_use_t;


void gpio_pulse(unsigned long, unsigned long);
void gpio_set_off(unsigned long);
void gpio_set_on(unsigned long);
void gpio_set(int, int);
void gpio_pbc_start(int);
void gpio_pbc_end(int); 
int  gpio_init(void);
void gpio_cleanup(void);

void gpio_pulse(unsigned long, unsigned long);
void led_pulse(unsigned long) ;



static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static ssize_t gpio_read(struct file *, char *, size_t, loff_t *);
static int gpio_open(struct inode *, struct file *);
static int gpio_close(struct inode *, struct file *);
static int  __init gpio_init_module(void);
static void __exit gpio_cleanup_module(void);

#endif
