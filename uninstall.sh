#! /bin/bash

# Define targets
DAEMON=iscsid
TOOL=iscsictl
KEXT=iSCSIInitiator.kext

# Stop the daemon, if it is running
sudo launchctl stop /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid
sudo launchctl unload /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo kextunload /Library/Extensions/$KEXT

# Remove the user tools and man pages
sudo rm /usr/share/man/man8/iscsictl.8
sudo rm /usr/share/man/man8/iscsid.8
sudo rm /usr/bin/$TOOL

# Remove daemon
sudo rm /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo rm /System/Library/LaunchDaemons/iscsid

# Remove kext
sudo rm -R /Library/Extensions/$KEXT
