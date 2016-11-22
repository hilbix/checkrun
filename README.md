Program watchdog to terminate a program with starving output

Checkrun runs a program and checks, if it did some output within a
given time interval.  If no output can be detected, the child will be
terminated, first with a HUP, then a TERM and afterwards a KILL.

In a distant future all this functionality here might be implemented
into my `timeout` tool as well, however I needed this feature before
timeout had this feature.  If you need to test a pipe for silence, use
`/bin/cat` as program, or try my `timeout` tool.


Compile:

```
git clone https://github.com/hilbix/checkrun.git
cd checkrun
git submodule update --init
make
sudo make install
```

Usage:

```
checkrun [options] program [args...]
To print help, see: checkrun -h
```

```
$ ./checkrun -h
Usage: checkrun [options] program [args..]
                version 0.0.4 compiled Nov 22 2016
        Runs program and monitors it's stdout for activity.
        If inactivity timeout is detected the program is terminated.
        Returns 36 on argument errors, 37 on kill problems
        else termination status of forked program (probably killed)
Options:
        -h      this help
        -a str  Activity string appended to -f instead of seen activity
                (has no effect if -f is not present)
        -d      Discard output of activity (=stdout of program)
        -f name Create file on activity
        -i str  Activity indicator (no LF is added) to stderr
        -n      Redirect stdin to program from /dev/null
        -q      Quiet mode (less comments)
                (see -v, from 2 to -2)
        -t secs Inactivity Timeout value (in seconds)
                (default 2)
        -v      Verbose mode (give more comments)
                (see -q, from -2 to 2)
```

Returns:

- 36 on argument errors
- 37 if it was unable to kill the program.
- return code of the forked program (possibly killed).

Example:

Something like following example script runs on one of my linux based
routers, because the (internal) network card sometimes stalls due to
lost interrupts on a too high traffic load:

```
while :
do
	checkrun -qqdni. ping 1.2.3.4
	ifconfig eth0 down
	ifconfig eth0 up
done
```

