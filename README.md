operating-systems-send-and-receive
==================================
Authors
-------
Ciaran Downey - 891709818  
Stratton Aguilar -

Running the program
-------------------
Compile the programs. A simple `make` should work, assuming `gcc` is available
in your `$PATH`.

Open two terminal sessions. In the first, execute the receiver (`recv`) like so:

```bash
./recv
```

>Note: For this part you'll need a file to transfer. For convenience, we've
>included one in the project (`file.txt`) that has 3543 lines of "lorem ipsum".

In the second, execute the sender with a file of your choice like so:

```bash
./sender file.txt
```

Testing the transfer worked
---------------------------
```bash
# this should return 0
diff recvfile file.txt | wc -l
```

Details
-------
- We used C to complete this assignment
- We did not implement the extra credit
- The assignment was tested on an Arch Linux virtual machine. Source Vagrant box
  is available [here][vagrant].

[vagrant]: https://github.com/terrywang/vagrantboxes/blob/master/archlinux-x86_64.md
