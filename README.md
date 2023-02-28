**Update 28 February 2023** : Apple has not provided a mechanism for opening sockets from within system extensions (if you know of one do let me know).  The porting of this initiator away from kernel extensions can be facilitated by using 'IOUserSCSIParallelInterfaceController' in a system extension, but the opening / management of sockets are required for communication. Without a means to do so, re-architecture of the project is required (for which I do not have the bandwidth).  Such an architecture would likely result in a performance penality.  It's not clear what, if any, replacement Apple plans for the `kpi_socket` interface (in userland / System Extensions).

**Update 27 March 2021** : Further development is on hold until DriverKit 20.4 (Beta) is released, with support for `IOUserSCSIParallelInterfaceController`. This software will ultimately transition away from kernel extensions.


### Overview

iSCSI initiator is a software initiator for macOS. It allows machines running macOS to connect to iSCSI targets. It automatically detects and mounts logical units on which users can then create and mount volumes. For more information about the iSCSI standard, see IETF RFC3720.

### Installation Prerequisites

Builds of the kernel extension will not be signed and as a result macOS won't load them. Kext signing must therefore be disabled before attempting to install and load the kernel extension. Additionally, as of El Capitan, new security measures have been implemented that prevent the installation of files in certain protected system folders (unless the files are placed there by an appropriate installer). For this reason, it is important to follow the directions applicable to the relevant version of macOS **prior** to installation of the initiator.

##### macOS 10.10 and earlier (prior to El Capitan)
Run the following command at a terminal prompt:
 
    sudo nvram boot-args=kext-dev-mode=1

The kernel will load unsigned kernel extensions after a reboot.

##### macOS 10.11 and later

Run the following command at the Recover OS terminal window:

    csrutil disable

Follow the instructions in the [System Integrity Protection Guide](https://developer.apple.com/library/mac/documentation/Security/Conceptual/System_Integrity_Protection_Guide/KernelExtensions/KernelExtensions.html#//apple_ref/doc/uid/TP40016462-CH4-SW1) to access the Recover OS terminal window. Two reboots may be required during this process.

### Installation

Download the desired release image file (.dmg), mount and run `Installer.pkg` to install the initiator. Similarly, run `Uninstall.pkg` to remove the initiator from your system. Ensure that no iSCSI targets are connected when updating or removing the initiator software, or you may experience an error during installation.

If you have an existing installation, logout of all targets before launching the installer. The installer will attempt to unload the iSCSI initiator kernel extension, if one exists and install new files. The iSCSI configuration will not be altered (existing settings are preserved).

