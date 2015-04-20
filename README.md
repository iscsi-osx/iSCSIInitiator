# Overview

The iSCSI initiator consists of a kernel extension (`iSCSIInitiator.kext`), a user-space daemon (`iscsid`) and a command-line utility (`iscsictl`).  

# Installation

The scripts `load.sh` and `unload.sh` are provided for testing purposes.  These scripts load and unload the kernel extension and launch daemon.  Once the kernel extension and daemon have been loaded, the command-line utility `iscsictl` can be used to add targets and login to them.  These scripts need to be re-executed each time the computer is restarted.  This project is a work in progress and a permanent installer will be provided in the future.

The load script copies the kernel extension `iSCSIInitiator.kext` to `/tmp/iSCSIInitiator.kext`, sets permissions appropriately and runs `kextload` to load the extension.  It further copies the daemon to `/tmp/iscsid/` and the daemon's property list to `/System/Library/LaunchDaemons/`, sets permissions of those files appropriately and tells `launchd` to start the daemon.

# Usage

Once the kernel extension and daemon have been loaded, the command-line utility `iscsictl` can be used to manage iSCSI functions.  The utility operates on a property list that keeps track of defined targets and communicates with the daemon to manage iSCSI sessions.

To add a target to the database (property list):

`# sudo iscsictl -add -target <target IQN> -portal <host:port> -interface <interface name>`

To login to the target:

`# iscsictl -login -target <iSCSI-qualified-name> -portal <host:port>`

To logout of the target:

`# iscsictl -logout -target <iSCSI-qualified-name>`

To list defined target and active sessions:

`# iscsictl -targets`

To list active LUNs:

`# iscsictl -luns`




# Implementation Overview

The kernel extension is based on IOKit and uses the SCSI Parallel Family IOKit driver to implement the SCSI stack.  The extension itself generates iSCSI protocol data units (PDUs) from SCSI commands passed down from the OS driver stack and parses incoming PDUs to complete tasks.  An IOKit user client class allows a user-space daemon to communicate with the kernel extension.  Specifically, the user client allows the daemon to create and remove sessions and connections, and to query the kernel for information about active sessions and connections.  A separate mach port allows the kernel extension to send notifications to the daemon as needed.

Session management functions are handled in user space.  This includes discovery, login, logout, error recovery, etc.  These are implemented within a daemon that runs continuously (it runs as an OS X Launch Daemon).  The command-line utility communicates with the daemon and manages a property list that keeps track of defined and discovery targets (separately) including their portals and both session-wide and connection-wide configuration parameters.  The daemon and command-line utility are implemented using Core Foundation libraries with some low-level API calls as needed.   
