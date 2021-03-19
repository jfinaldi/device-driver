# Device-Driver
**Background**:

The Linux operating system consists of the Kernel some other minor components and then a large number of Device Drivers.  Device drivers are the key to how various hardware devices interface with the computer.

**Task**:

Develop a skeleton device driver that can be loaded and run in Linux.  Then add some minor functionality to the device driver such as the user/application passing in a value to the device driver and the device driver returns that nth word from a file.  Include a document on how to build, load and interact with the device driver along with screen shots of output.

**Requirements**:
It must be written in C.  It must be a valid an loadable device driver with at least some minimal user/application functionality. That includes an open, release, read, write, and at least one ioctl command.  It must also be able to be unloaded, and indicate that it has unloaded from the system.  Make use of the printk and copy_to_user functions.

You should also write a user application that utilizes your device driver.

**Example**:
Create a calculator device driver.  Open your device driver and write a series of numbers, use ioctl to perform an operation like summation, then use read to get the result. Close (release) the driver when you are done.



	
