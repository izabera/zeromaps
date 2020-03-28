A "living" Linux process with no memory
=======================================



tl;dr
----

- thread1 goes into uninterruptible sleep
- thread2 unmaps everything and segfaults
- segv can't kill the process because of thread1's D state
- /proc/pid/maps is now empty
- ???
- PROFIT!!!



[![asciicast](https://asciinema.org/a/313677.svg)](https://asciinema.org/a/313677)



Implementation details
----------------------

This code gets a list of all memory maps from `/proc/self/maps`, then creates a
new executable map where it jits some code that calls `munmap()` on each of the
maps it just got, and finally on the map it's on.  This is just a quick example
with no portability in mind, so the source code contains the actual bytes that
would be emitted by a x64 compiler.  After unmapping the final map, where the
jit code lies, there's no new instruction to execute and a segfault is raised.

This segfault can't kill the entire process if one thread is stuck in
uninterruptible sleep.  To reliably send a thread in such state, we create a
simple FUSE filesystem in python, in which doing anything on a particular file
will block until a key is pressed.

This code also does its own "linking" to make sure that the list of maps
doesn't get unmapped too early.


Requirements
------------

- a c compiler
- python3 + fuse
- x64
- a modern Linux with no vsyscall page (this page is too high up and munmap
  would return EINVAL)


Installation
------------

`pip` has to be the python 3 pip, not the python2 pip, which is
often installed as `pip2` these days.
`pip` installed fusepy 3.0.1 for me.

    $ pip install fusepy
    $ cc -o zeromaps zeromaps.c
    $ mkdir x
    $ ./fs.py x
    ... after you start zeromaps
    /blockme
    blocked, press enter

In another window:

    $ ./zeromaps
    spawned thread
    got all the maps
    BTW MY PID IS 37677
    start unmapping like crazy

You can check that all the memory mappings are deleted:

    $ cat /proc/37677/maps

Why
---

I don't know.  I thought it was funny.
