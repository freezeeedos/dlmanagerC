Dlmanager
========
Download manager written in C/LibCURL. It opens a file in a text editor, and downloads the links you paste in it.

I won't try to make everything work right away, I use the program everyday, and do my best to fix problems as they come.
If you like this program, feel free to report bugs at freezeeedos(at)gmail.com .

Sample Output:

      => Getting 'CentOS-6.4-x86_64-netinstall.iso':
    100%  230.0/230.0 mB       805 kB/s    eta: 0h0m0s     
      => Getting 'CentOS-6.4-x86_64-minimal.iso':
    100%  342.3/342.3 mB       813 kB/s    eta: 0h0m0s
    

Install:
--------

Linux/cygwin:

You will need the GNU C COMPILER.

* Install the "libcurl-dev" package (or equivalent for your distribution)
* './build.sh'
* Enjoy !

[libcurl website] (http://curl.haxx.se/libcurl/)
