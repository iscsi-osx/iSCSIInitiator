#! /bin/bash

# Define targets
DAEMON=iscsid
TOOL=iscsictl
KEXT=iSCSIInitiator.kext
FRAMEWORK=iSCSI.framework
DAEMON_PLIST=com.github.iscsi-osx.iscsid.plist
MAN_TOOL=iscsictl.8
MAN_DAEMON=iscsid.8

# Define install path
DAEMON_DST=/usr/local/libexec
DAEMON_PLIST_DST=/Library/LaunchDaemons
FRAMEWORK_DST=/Library/Frameworks
TOOL_DST=/usr/local/bin
MAN_DST=/usr/local/share/man/man8
PREF_DST=/Library/Preferences
PREF_FILE=com.github.iscsi-osx.iSCSIInitiator.plist

# Get minor version of the OS
OSX_MINOR_VER=$(sw_vers -productVersion | awk -F '.' '{print $2}')

# Minor version of OS X Mavericks
OSX_MAVERICKS_MINOR_VER="9"

if [ "$OSX_MINOR_VER" -gt "$OSX_MAVERICKS_MINOR_VER" ]; then
    KEXT_DST=/Library/Extensions
else
    KEXT_DST=/System/Library/Extensions
fi

# Stop, unload and remove launch daemon
sudo launchctl stop $DAEMON_PLIST_DST/$DAEMON_PLIST
sudo launchctl unload $DAEMON_PLIST_DST/$DAEMON_PLIST
sudo rm -f $DAEMON_PLIST_DST/$DAEMON_PLIST
sudo rm -f /usr/sbin/$DAEMON # Old location
sudo rm -f /System/Library/LaunchDaemons/$DAEMON_PLIST # Old location
sudo rm -f $DAEMON_DST/$DAEMON

# Unload & remove kernel extension
sudo kextunload $KEXT_DST/$KEXT
sudo rm -f -R $KEXT_DST/$KEXT

# Remove user tools
sudo rm -f /usr/bin/$TOOL # Old location
sudo rm -f $TOOL_DST/$TOOL

# Remove framework
sudo rm -R $FRAMEWORK_DST/$FRAMEWORK

# Remove man pages
sudo rm -f $MAN_DST/$MAN_DAEMON
sudo rm -f $MAN_DST/$MAN_TOOL

# Remove configuration file
sudo rm -f $PREF_DST/$PREF_FILE

# Flush preferences cache
sudo killall cfprefsd
