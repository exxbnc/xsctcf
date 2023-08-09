# About

xsctcf (X11 set color temperature) is a UNIX tool which allows you to set the color
temperature of your screen. It is simpler than [Redshift](https://github.com/jonls/redshift) and [f.lux](https://justgetflux.com/).

Original code was published by Ted Unangst in the public domain:
https://www.tedunangst.com/flak/post/sct-set-color-temperature

Original source code can be found here https://github.com/faf0/sct.

Reason for making this is a lack of a f.lux counter part for linux. I do not like redshift. A very primative implementation but works for my needs.

# XSCTCF Changes over faf0/sct.
- fixed the header locations
- added -f (--chinese-flux) to use the chinese flux option
- some syntatic changes
- added macros to header
- uses gnu99 over c99. Reason is for usleep().

# Chinese Flux Usage

Define your settings in the header file and make. The f.lux ripoff will check what hour it is local time and change the temperature of the screen gradually based on parameters. 

Can use --verbose aswell to test your config to make sure it works as intented. 

# Using a config
To use the config run "make config". This will generate a config file at HOME/.config/xsctcf/xsctcf.con
The program will automatically use the config over the default values

# Using it 
Add to startup applications as /home/usr/dest/xsctcf -f
or run using /home/usr/dest/xsctcf -f &

# Stopping the process
kill it using sigterm or sigkill

# Limitations: 
- Only able to set two temeratures and two times
- Needs to be killed manually
- The program assumes that once running you will not change the screen temperature manually. With this it means the program has no clue what the screen temperature is after the initial        check. It will jump back to what the program thinks it should be. This is so it doesn't do unneccesary checks on the screen temperature.
- The program will know the start temp, the lower bound, upper bound, and step distance it will find values closest to these that are devisiable by the step distance. Finds decently close values for up to a step distance of 23. The highest I tested. I would not use a large step distance as it defeats the purpose of using the f.lux capabilities.
- Possible bugs that I am unaware of but works for what I need it to.

# Installation

## Make-based

On UNIX-based systems, a convenient method of building this software is using Make.
Since the `Makefile` is simple and portable, both the BSD and [GNU make](https://www.gnu.org/software/make/) variants will have no problems parsing and executing it correctly.

The simplest invocation is
~~~sh
make
~~~
that will use the default values for all flags as provided in the `Makefile`.

Overriding any of the following variables is supported by exporting a variable with the same name and your desired content to the environment:
* `CC`
* `CFLAGS`
* `LDFLAGS`
* `PREFIX`
* `BIN` (the directory into which the resulting binary will be installed)
* `MAN` (the directory into which the man page will be installed)
* `INSTALL` (`install(1)` program used to create directories and copy files)

Both example calls are equivalent and will build the software with the specified compiler and custom compiler flags:
~~~sh
make CC='clang' CFLAGS='-O2 -pipe -g3 -ggdb3' LDFLAGS='-L/very/special/directory -flto'
~~~

~~~sh
export CC='clang'
export CFLAGS='-O2 -pipe -g3 -ggdb3'
export LDFLAGS='-L/very/special/directory -flto'
make
~~~

The software can be installed by running the following command:
~~~sh
make install
~~~

If you prefer a different installation location, override the `PREFIX` variable:
~~~sh
make install PREFIX="${HOME}/xsctcf-prefix"
~~~

~~~sh
export PREFIX="${HOME}/xsctcf-prefix"
make install
~~~

## Manual compilation

Compile the code using the following command:
~~~sh
gcc -Wall -Wextra -Werror -pedantic -std=gnu99 -O2 -I /usr/X11R6/include xsctcf.c -o xsct -L /usr/X11R6/lib -lX11 -lXrandr -lm -s
~~~

# Usage

Provided that xsctcf binary is located in your `$PATH`, execute it using the following command:
~~~sh
xsctcf 3700 0.9
~~~

The first parameter (`3700` above) represents the color temperature.

The second parameter (`0.9` above) represents the brightness. The values are in the range `[0.0, 1.0]`.

If `xsctcf` is called with parameter 0, the color temperature is set to `6500`.

If `xsctcf` is called with the color temperature parameter only, the brightness is set to `1.0`.

If `xsctcf` is called without parameters, the current display temperature and brightness are estimated.

The following options, which can be specified before the optional temperature parameter, are supported:
- `-h`, `--help`: display the help page
- `-v`, `--verbose`: display debugging information
- `-d <delta>`, `--delta <delta>`: shift temperature by the temperature value
- `-s <screen>`, `--screen <screen>` `N`: use the screen specified by given zero-based index
- `-c <crtc>`, `--crtc <crtc>` `N`: use the CRTC specified by given zero-based index
- `-f <flux>`, `--chinese-flux <flux>`: performs like flux

Test xsctcf using the following command:
~~~sh
xsctcf 3700 0.9 && xsct
~~~

# Resources

The following website by Mitchell Charity provides a table for the conversion between black-body temperatures and color pixel values:
http://www.vendian.org/mncharity/dir3/blackbody/

---
