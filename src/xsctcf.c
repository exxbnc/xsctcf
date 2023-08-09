/*
 * xsctcf - X11 set color temperature
 * To use the chinese flux run with -f 
 *
 * Public domain, do as you wish.
 */

#include "xsctcf.h"

#include <X11/extensions/Xrandr.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <math.h>
#include <string.h>

#include <pwd.h>

static void usage(char * pname) {
    printf("Xsct (%s)\n"
           "Usage: %s [options] [temperature] [brightness]\n"
           "\tIf the argument is 0, xsct resets the display to the default temperature (6500K)\n"
           "\tIf no arguments are passed, xsct estimates the current display temperature and brightness\n"
           "Options:\n"
           "\t-h, --help \t\t xsct will display this usage information\n"
           "\t-v, --verbose \t\t xsct will display debugging information\n"
           "\t-d, --delta\t\t xsct will shift temperature by the temperature value\n"
           "\t-s, --screen N\t\t xsct will only select screen specified by given zero-based index\n"
           "\t-f, --chinese-flux\t xsctcf will mimic flux using predefined values\n"
           "\t-c, --crtc N\t\t xsct will only select CRTC specified by given zero-based index\n", XSCTCF_VERSION, pname);
}

static double DoubleTrim(double x, double a, double b) {
    double buff[3] = {a, x, b};
    return buff[ (int)(x > a) + (int)(x > b) ];
}

static struct temp_status get_sct_for_screen(Display *dpy, int screen, int icrtc, int fdebug) {
    Window root = RootWindow(dpy, screen);
    XRRScreenResources *res = XRRGetScreenResourcesCurrent(dpy, root);

    int n, c;
    double t = 0.0;
    double gammar = 0.0, gammag = 0.0, gammab = 0.0, gammad = 0.0;
    struct temp_status temp;
    temp.temp = 0;
    temp.brightness = 1.0;

    n = res->ncrtc;
    if ((icrtc >= 0) && (icrtc < n))
        n = 1;
    else
        icrtc = 0;
    for (c = icrtc; c < (icrtc + n); c++) {
        RRCrtc crtcxid;
        int size;
        XRRCrtcGamma *crtc_gamma;
        crtcxid = res->crtcs[c];
        crtc_gamma = XRRGetCrtcGamma(dpy, crtcxid);
        size = crtc_gamma->size;
        gammar += crtc_gamma->red[size - 1];
        gammag += crtc_gamma->green[size - 1];
        gammab += crtc_gamma->blue[size - 1];

        XRRFreeGamma(crtc_gamma);
    }
    XFree(res);
    temp.brightness = (gammar > gammag) ? gammar : gammag;
    temp.brightness = (gammab > temp.brightness) ? gammab : temp.brightness;
    if (temp.brightness > 0.0 && n > 0) {
        gammar /= temp.brightness;
        gammag /= temp.brightness;
        gammab /= temp.brightness;
        temp.brightness /= n;
        temp.brightness /= BRIGHTHESS_DIV;
        temp.brightness = DoubleTrim(temp.brightness, 0.0, 1.0);
        if (fdebug > 0) fprintf(stderr, "DEBUG: Gamma: %f, %f, %f, brightness: %f\n", gammar, gammag, gammab, temp.brightness);
        gammad = gammab - gammar;
        if (gammad < 0.0) {
            if (gammab > 0.0) {
                t = exp((gammag + 1.0 + gammad - (GAMMA_K0GR + GAMMA_K0BR)) / (GAMMA_K1GR + GAMMA_K1BR)) + TEMPERATURE_ZERO;
            }
            else {
                t = (gammag > 0.0) ? (exp((gammag - GAMMA_K0GR) / GAMMA_K1GR) + TEMPERATURE_ZERO) : TEMPERATURE_ZERO;
            }
        }
        else {
            t = exp((gammag + 1.0 - gammad - (GAMMA_K0GB + GAMMA_K0RB)) / (GAMMA_K1GB + GAMMA_K1RB)) + (TEMPERATURE_NORM - TEMPERATURE_ZERO);
        }
    }
    else
        temp.brightness = DoubleTrim(temp.brightness, 0.0, 1.0);

    temp.temp = (int)(t + 0.5);

    return temp;
}

static void sct_for_screen(Display *dpy, int screen, int icrtc, struct temp_status temp, int fdebug) {
    double t = 0.0, b = 1.0, g = 0.0, gammar, gammag, gammab;
    int n, c;
    Window root = RootWindow(dpy, screen);
    XRRScreenResources *res = XRRGetScreenResourcesCurrent(dpy, root);

    t = (double)temp.temp;
    b = DoubleTrim(temp.brightness, 0.0, 1.0);
    if (temp.temp < TEMPERATURE_NORM) {
        gammar = 1.0;
        if (temp.temp > TEMPERATURE_ZERO) {
            g = log(t - TEMPERATURE_ZERO);
            gammag = DoubleTrim(GAMMA_K0GR + GAMMA_K1GR * g, 0.0, 1.0);
            gammab = DoubleTrim(GAMMA_K0BR + GAMMA_K1BR * g, 0.0, 1.0);
        }
        else {
            gammag = 0.0;
            gammab = 0.0;
        }
    }
    else {
        g = log(t - (TEMPERATURE_NORM - TEMPERATURE_ZERO));
        gammar = DoubleTrim(GAMMA_K0RB + GAMMA_K1RB * g, 0.0, 1.0);
        gammag = DoubleTrim(GAMMA_K0GB + GAMMA_K1GB * g, 0.0, 1.0);
        gammab = 1.0;
    }
    if (fdebug > 0) fprintf(stderr, "DEBUG: Gamma: %f, %f, %f, brightness: %f\n", gammar, gammag, gammab, b);
    n = res->ncrtc;
    if ((icrtc >= 0) && (icrtc < n))
        n = 1;
    else
        icrtc = 0;
    for (c = icrtc; c < (icrtc + n); c++) {
        int size, i;
        RRCrtc crtcxid;
        XRRCrtcGamma *crtc_gamma;
        crtcxid = res->crtcs[c];
        size = XRRGetCrtcGammaSize(dpy, crtcxid);

        crtc_gamma = XRRAllocGamma(size);

        for (i = 0; i < size; i++) {
            g = GAMMA_MULT * b * (double)i / (double)size;
            crtc_gamma->red[i] = (unsigned short int)(g * gammar + 0.5);
            crtc_gamma->green[i] = (unsigned short int)(g * gammag + 0.5);
            crtc_gamma->blue[i] = (unsigned short int)(g * gammab + 0.5);
        }

        XRRSetCrtcGamma(dpy, crtcxid, crtc_gamma);
        XRRFreeGamma(crtc_gamma);
    }

    XFree(res);
}

static char *trim(char *str) {
    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }
    if (*str == '\0') {
        return str;
    }
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) {
        end--;
    }
    *(end + 1) = '\0';
    return str;
}

int main(int argc, char **argv) {
    int i, screen, screens;
    int screen_specified, screen_first, screen_last, crtc_specified;
    struct temp_status temp;
    int fdebug = 0, fdelta = 0, fhelp = 0, cflux = 0;

    Display *dpy = XOpenDisplay(NULL);

    if (!dpy) {
        perror("XOpenDisplay(NULL) failed");
        fprintf(stderr, "ERROR! Ensure DISPLAY is set correctly!\n");
        return EXIT_FAILURE;
    }
    screens = XScreenCount(dpy);
    screen_first = 0;
    screen_last = screens - 1;
    screen_specified = -1;
    crtc_specified = -1;
    temp.temp = DELTA_MIN;
    temp.brightness = -1.0;
    for (i = 1; i < argc; i++) {
        if ((strcmp(argv[i],"-h") == 0) || (strcmp(argv[i],"--help") == 0)) fhelp = 1;
        else if ((strcmp(argv[i],"-v") == 0) || (strcmp(argv[i],"--verbose") == 0)) fdebug = 1;
        else if ((strcmp(argv[i],"-d") == 0) || (strcmp(argv[i],"--delta") == 0)) fdelta = 1;
        else if ((strcmp(argv[i],"-f") == 0) || (strcmp(argv[i], "--chinese-flux") == 0)) cflux = 1;
        else if ((strcmp(argv[i],"-s") == 0) || (strcmp(argv[i],"--screen") == 0)) {
            i++;
            if (i < argc) {
                screen_specified = atoi(argv[i]);
            } else {
                fprintf(stderr, "ERROR! Required value for screen not specified!\n");
                fhelp = 1;
            }
        }
        else if ((strcmp(argv[i],"-c") == 0) || (strcmp(argv[i],"--crtc") == 0)) {
            i++;
            if (i < argc) {
                crtc_specified = atoi(argv[i]);
            } else {
                fprintf(stderr, "ERROR! Required value for crtc not specified!\n");
                fhelp = 1;
            }
        }
        else if (temp.temp == DELTA_MIN) temp.temp = atoi(argv[i]);
        else if (temp.brightness < 0.0) temp.brightness = atof(argv[i]);
        else {
            fprintf(stderr, "ERROR! Unknown parameter: %s\n!", argv[i]);
            fhelp = 1;
        }
    }

    if(cflux) {

        int USER_MIN =  DEFAULT_USER_MIN;          
        int USER_MAX =  DEFAULT_USER_MAX;     
        float USER_BRIGHT = DEFAULT_USER_BRIGHT;
        int MORNING_TIME = DEFAULT_MORNING_TIME_HOUR;
        int NIGHT_TIME = DEFAULT_NIGHT_TIME_HOUR;
        int TIME_SLEEP = DEFAULT_TIME_SLEEP;
        int STEP_SLEEP = DEFAULT_STEP_SLEEP;
        int STEP_DIST = DEFAULT_STEP_DIST;

        char *uHomeDir = getenv("HOME");
        char uDest[256];
        strncpy(uDest, uHomeDir, sizeof(uDest) - 1);
        strncat(uDest, "/.config/xsctcf/xsctcf.conf", sizeof(uDest) - strlen(uDest) - 1);

        FILE *uConfig;

        if(access(uDest, F_OK)!= -1){
            uConfig = fopen(uDest, "r");
            char line[100];
            while (fgets(line, sizeof(line), uConfig)) {
                char *tLine = trim(line);
                if (strlen(tLine) == 0 || tLine[0] == '#') continue;  // Skip empty lines and comments

                char *param = trim(strtok(tLine, "="));
                char *value = trim(strtok(NULL, "="));

                if (!(param && value)) continue;

                if (strcmp(trim(param), "USER_MIN") == 0)  USER_MIN = atoi(value);
                else if (strcmp(param, "USER_MAX") == 0) USER_MAX = atoi(value);
                else if (strcmp(param, "USER_BRIGHT") == 0) USER_BRIGHT = atof(value);
                else if (strcmp(param, "MORNING_TIME") == 0) MORNING_TIME = atoi(value);
                else if (strcmp(param, "NIGHT_TIME") == 0) NIGHT_TIME = atoi(value);
                else if (strcmp(param, "TIME_SLEEP") == 0) TIME_SLEEP = atoi(value);
                else if (strcmp(param, "STEP_SLEEP") == 0) STEP_SLEEP = atoi(value);
                else if (strcmp(param, "STEP_DIST") == 0) STEP_DIST = atoi(value); 
            }

            fclose(uConfig);
            if(fdebug) fprintf(stderr,"Using Config\n\n");
        }
        else if(fdebug) fprintf(stderr, "Not Using Config\nWill Use Default Values!\n\n");
        if(fdebug) fprintf(stderr, "USER_MIN: %d\nUSER_MAX: %d\nUSER_BRIGHT: %f\nMORNING_TIME: %d\nNIGHT_TIME: %d\nTIME_SLEEP: %d\nSTEP_SLEEP: %d\nSTEP_DIST: %d\n\n", USER_MIN, USER_MAX, USER_BRIGHT, MORNING_TIME, NIGHT_TIME, TIME_SLEEP, STEP_SLEEP, STEP_DIST);

        // Config error handling
        if(STEP_DIST <= 0 || MORNING_TIME > NIGHT_TIME || USER_BRIGHT <= 0 || USER_MAX <= 100 || USER_MIN <= 100 || MORNING_TIME <= 0 || NIGHT_TIME <= 0 || TIME_SLEEP <= 0 || STEP_SLEEP <= 0) exit(EXIT_FAILURE);

        time_t currentTime = time(NULL);
        struct tm *localTime = localtime(&currentTime);
        struct temp_status cFTemp = get_sct_for_screen(dpy, screen_first, crtc_specified, fdebug);

        int currentHour = localTime->tm_hour;

        cFTemp.brightness = USER_BRIGHT;
        
        cFTemp.temp = ((cFTemp.temp + STEP_DIST / 2) / STEP_DIST) * STEP_DIST;
        int feasibleMin = ((USER_MIN + STEP_DIST / 2) / STEP_DIST) * STEP_DIST;
        int feasibleMax = ((USER_MAX + STEP_DIST / 2) / STEP_DIST) * STEP_DIST;

        int newTemp = currentHour >= NIGHT_TIME || currentHour < MORNING_TIME ? feasibleMin : feasibleMax;
        int step = newTemp > cFTemp.temp ? STEP_DIST : (newTemp == cFTemp.temp ? 0 : -STEP_DIST);

        while(1) {
            currentTime = time(NULL);
            localTime = localtime(&currentTime);
            currentHour = localTime->tm_hour;

            if(fdebug) fprintf(stderr,"DEBUG:\nStep: %d\nCurrent Temp: %d\nMax Temp: %d\nMin Temp: %d\n\n",step,cFTemp.temp,feasibleMax,feasibleMin);

            while(!step || newTemp != cFTemp.temp) {

                if(step) cFTemp.temp += step;
                else step = STEP_DIST;

                for(screen = screen_first; screen <= screen_last; screen++)
                    sct_for_screen(dpy, screen, crtc_specified, cFTemp, fdebug);

                usleep(STEP_SLEEP);
            }

            newTemp = currentHour >= NIGHT_TIME || currentHour < MORNING_TIME ? feasibleMin : feasibleMax;
            step = newTemp > cFTemp.temp ? STEP_DIST : (newTemp == cFTemp.temp ? 0 : -STEP_DIST);

            sleep(TIME_SLEEP);
        }
    }

    if (fhelp > 0) {
        usage(argv[0]);
    }
    else if (screen_specified >= screens) {
        fprintf(stderr, "ERROR! Invalid screen index: %d!\n", screen_specified);
    }
    else {
        if (temp.brightness < 0.0) temp.brightness = 1.0;
        if (screen_specified >= 0) {
            screen_first = screen_specified;
            screen_last = screen_specified;
        }
        if ((temp.temp < 0) && (fdelta == 0)) {
            // No arguments, so print estimated temperature for each screen
            for (screen = screen_first; screen <= screen_last; screen++) {
                temp = get_sct_for_screen(dpy, screen, crtc_specified, fdebug);
                printf("Screen %d: temperature ~ %d %f\n", screen, temp.temp, temp.brightness);
            }
        }
        else {
            if (fdelta == 0) {
                // Set temperature to given value or default for a value of 0
                if (temp.temp == 0) {
                    temp.temp = TEMPERATURE_NORM;
                }
                else if (temp.temp < TEMPERATURE_ZERO) {
                    fprintf(stderr, "WARNING! Temperatures below %d cannot be displayed.\n", TEMPERATURE_ZERO);
                    temp.temp = TEMPERATURE_ZERO;
                }
                for (screen = screen_first; screen <= screen_last; screen++) {
                   sct_for_screen(dpy, screen, crtc_specified, temp, fdebug);
                }
            }
            else {
                // Delta mode: Shift temperature of each screen by given value
                for (screen = screen_first; screen <= screen_last; screen++) {
                    struct temp_status tempd = get_sct_for_screen(dpy, screen, crtc_specified, fdebug);
                    tempd.temp += temp.temp;
                    if (tempd.temp < TEMPERATURE_ZERO) {
                        fprintf(stderr, "WARNING! Temperatures below %d cannot be displayed.\n", TEMPERATURE_ZERO);
                        tempd.temp = TEMPERATURE_ZERO;
                    }
                    sct_for_screen(dpy, screen, crtc_specified, tempd, fdebug);
                }
            }
        }
    }

    XCloseDisplay(dpy);

    return EXIT_SUCCESS;
}

