The initiator may be built by running the `build.sh` script.  After the necessary components have been built, the script `install.sh` can be used for installation.  Once installed, the command-line utility `iscsictl` can be used to add targets and login to them. See the project Wiki for more information.

Important:  

1.  Run the `uninstall.sh` to remove older versions before installing each time.  This ensures that the property list is removed should its format change during development.  Run `clean.sh` to reset the initiator's configuration to defaults.  
2.  Disable kext signing before attempting to load the kext using `load.sh`.
 * Prior to El Capitan (that is, OS X versions 10.10 and below), this is achieved by running `sudo nvram boot-args=kext-dev-mode=1`
 * In El Capitan, this is achieved by running `csrutil enable` at the Recover OS terminal window (see the [System Integrity Protection Guide](https://developer.apple.com/library/mac/documentation/Security/Conceptual/System_Integrity_Protection_Guide/KernelExtensions/KernelExtensions.html#//apple_ref/doc/uid/TP40016462-CH4-SW1) for more details).

 In both cases, a reboot is required.

