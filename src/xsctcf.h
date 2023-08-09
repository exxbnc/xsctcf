/*
 * xsctcf - X11 set color temperature
 *
 * Public domain, do as you wish.
 */

#include <X11/Xlib.h>

#ifndef __XSCTCF_H
#define __XSCTCF_H

#define XSCTCF_VERSION "1.0"

#define TEMPERATURE_NORM    6500
#define TEMPERATURE_ZERO    700
#define GAMMA_MULT          65535.0
// Approximation of the `redshift` table from
// https://github.com/jonls/redshift/blob/04760afe31bff5b26cf18fe51606e7bdeac15504/src/colorramp.c#L30-L273
// without limits:
// GAMMA = K0 + K1 * ln(T - T0)
// Red range (T0 = TEMPERATURE_ZERO)
// Green color
#define GAMMA_K0GR          -1.47751309139817
#define GAMMA_K1GR          0.28590164772055
// Blue color
#define GAMMA_K0BR          -4.38321650114872
#define GAMMA_K1BR          0.6212158769447
// Blue range  (T0 = TEMPERATURE_NORM - TEMPERATURE_ZERO)
// Red color
#define GAMMA_K0RB          1.75390204039018
#define GAMMA_K1RB          -0.1150805671482
// Green color
#define GAMMA_K0GB          1.49221604915144
#define GAMMA_K1GB          -0.07513509588921
#define BRIGHTHESS_DIV      65470.988
#define DELTA_MIN           -1000000

// Below defines the chineseflux constants
#define DEFAULT_USER_MIN             3500         // MIN TEMP
#define DEFAULT_USER_MAX             6000         // MAX TEMP
#define DEFAULT_USER_BRIGHT           1.0         // BRRIGHTNESS
#define DEFAULT_MORNING_TIME_HOUR      10         // TIME IN HOURS FOR MORNING
#define DEFAULT_MORNING_TIME_MIN        0         // TIME MINUTES FOR MONING
#define DEFAULT_NIGHT_TIME_HOUR        22         // TIME IN HOURS FOR NIGHT 
#define DEFAULT_NIGHT_TIME_MIN          0         // TIME MINUTES FOR MONING
#define DEFAULT_TIME_SLEEP             10         // SLEEP TIME BETWEEN CHECKS IN SECONDS
#define DEFAULT_STEP_SLEEP          50000         // SLEEP TIME BETWEEN TEMP STEPS IN MICROSECONDS 
#define DEFAULT_STEP_DIST               1         // STEP BETWEEN COLORS

struct temp_status {
    int temp;
    double brightness;
};

static void usage(char * pname);
static double DoubleTrim(double x, double a, double b);
static struct temp_status get_sct_for_screen(Display *dpy, int screen, int icrtc, int fdebug);
static void sct_for_screen(Display *dpy, int screen, int icrtc, struct temp_status temp, int fdebug);
static char *trim(char *str);

#endif /* __XSCTCF_H */
