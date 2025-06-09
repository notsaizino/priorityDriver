Driver Priority Changer

A Windows driver that lets you tweak the priority of any running process by its Process ID (PID) and Thread Number. Need to give your game a performance boost or prioritize a critical app? This driver does the trick.


Features:

    1. Change thread priority for any process

    2. Lightweight and efficient

    3. Works on modern versions of Windows (tested on Windows 10/11)

How It Works

The driver hooks into the target process and modifies the priority of a specified thread. 

The project contains 3 files: priority_driver.sln contains the actual code of the driver itself. It's filled with comments explaining my logic and every function in it (so if anyone wants to use it as reference you can).
The second file is HEADER.h, which is a library that links my (user-defined) kernel functions (booser-write, etc..) to the userclient app, which is in the clientmode folder. 

**NOTE: USE AT YOUR OWN RISK. I HAVE NOT FULLY TESTED THIS DRIVER, BUT THERE SEEMS TO BE NO ISSUES/BUGS WHEN COMPILING AND WHEN TRACING WITH WINDBG.**
