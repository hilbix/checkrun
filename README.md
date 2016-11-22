Compile:

```
make
sudo make install
```

Usage:

```
checkrun [options] program [args...]
To print help, see: checkrun -h
```

Returns:

- 36 on argument errors
- 37 if it was unable to kill the program.
- return code of the forked program (possibly killed).

About: see [DESCRIPTION](DESCRIPTION)

Source: https://github.com/hilbix/checkrun

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

