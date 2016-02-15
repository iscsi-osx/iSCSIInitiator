
####OVERVIEW

iSCSI initiator is a software initiator for OS X. It allows machines running OS X to connect to iSCSI targets. It automatically detects and mounts logical units on which users can then create and mount volumes. For more information about the iSCSI standard, see IETF RFC3720.

####INSTALLATION

Disable kext signing before attempting to install the initiator.
 * Prior to El Capitan (that is, OS X versions 10.10 and below), this is achieved by running `sudo nvram boot-args=kext-dev-mode=1`
 * In El Capitan, this is achieved by running `csrutil disable` at the Recover OS terminal window (see the [System Integrity Protection Guide](https://developer.apple.com/library/mac/documentation/Security/Conceptual/System_Integrity_Protection_Guide/KernelExtensions/KernelExtensions.html#//apple_ref/doc/uid/TP40016462-CH4-SW1) for more details).

 In both cases, a reboot is required.
 
 
After kext signing is disabled, download the release image file (.dmg) and open it in OS X. Run the "Installer.pkg" file and follow the on-screen instructions to install the initiator.

