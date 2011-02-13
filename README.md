transmit
========
Copyright 2011, Alex Budovski.

Summary
-------
Transmit lets you files over a network, really fast! It is a simple alternative
to netcat on Windows. The connection is unencrypted, like `nc`.

`transmit` is designed for Windows only. Linux users have a modern version of
`netcat`. `transmit` is the transmitting part of the `netcat`
"listen + transmit" communication pair. It's mainly used to send from a
Windows host to a POSIX host since Windows to Windows communication can be done
via existing graphical tools such as TeraCopy.

Compiling
---------

Supports any recent version of Visual C++. With a VC++ command prompt,

    >cd transmit
    >nmake

This will automatically create a debug and release build in the `dbg\` and
`rel\` folders, respectively. Or you can just download the prebuilt `.tgz`
package from github.

Examples
--------

### Transfer a big file from hostA to hostB

On the receiving end (Linux box), assuming modern GNU `nc`.

    hostB$ nc -l 1234 > bigfile.dat

    # transmitter, Windows (cmd.exe)
    hostA> transmit c:\bigfile.dat hostB 1234

Alternatively, you can use an MinGW bash shell (e.g. Git Bash)

    # transmitter, Windows (Bash)
    hostA$ transmit /c/bigfile.dat hostB 1234

The advantage of the latter is that it allows you to use all the *nix utilities
like `time` to see how long the transfer took, `tar` to send (and optionally
compress) entire directories, preserving names and structure, etc.

### Transfer a directory, and compress with gzip.

On the receiver (Linux):

    hostB$ nc -l 1234 | tar -xvzf -

The sender (Windows, with Bash). Suppose we want to send a folder `big_dir`,
compressing it on the fly with gzip:

    hostA$ tar -czvf - big_dir | transmit - hostA 1234

Since gzipping is somewhat expensive on the CPU, you may get better throughput
by omitting the 'z' flag in the tar command. I.e. `tar -cvf` and `tar -xvf`.
