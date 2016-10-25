
### Overview

iSCSI initiator is a software initiator for macOS. It allows machines running macOS to connect to iSCSI targets. It automatically detects and mounts logical units on which users can then create and mount volumes. For more information about the iSCSI standard, see IETF RFC3720.

### Installation Prerequisites

Manual builds of the kernel extension will not be signed and as a result macOS won't load them. Kext signing must therefore be disabled before attempting to install and load the kernel extension. Additionally, as of El Capitan, new security measures have been implemented that prevent the installation of files in certain protected system folders (unless the files are placed there by an appropriate installer). For this reason, it is important to follow the directions applicable to the relevant version of macOS **prior** to installation of the initiator.

##### OS X 10.10 and earlier (prior to El Capitan)
Run the following command at a terminal prompt:
 
    sudo nvram boot-args=kext-dev-mode=1

The kernel will load unsigned kernel extensions after a reboot.

##### OS X 10.11 and later

Run the following command at the Recover OS terminal window:

    csrutil enable

Follow the instructions in the [System Integrity Protection Guide](https://developer.apple.com/library/mac/documentation/Security/Conceptual/System_Integrity_Protection_Guide/KernelExtensions/KernelExtensions.html#//apple_ref/doc/uid/TP40016462-CH4-SW1) to access the Recover OS terminal window. Two reboots may be required during this process.

### Installation

Download the desired release image file (.dmg), mount and run `Installer.pkg` to install the initiator. Similarly, run `Uninstall.pkg` to remove the initiator from your system. Ensure that no iSCSI targets are connected when updating or removing the initiator software, or you may experience an error during installation.

If you have an existing installation, logout of all targets before launching the installer. The installer will attempt to unload the iSCSI initiator kernel extension, if one exists and install new files. The iSCSI configuration will not be altered (existing settings are preserved).

