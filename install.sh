#! /bin/bash

# Define targets
DAEMON=iscsid
TOOL=iscsictl
KEXT=iSCSIInitiator.kext

# Define paths
SOURCE_PATH=./DerivedData/Build/Products/Debug

# Copy kernel extension & load it
sudo cp -R $SOURCE_PATH/$KEXT /Library/Extensions/$KEXT
sudo chmod -R 755 /Library/Extensions/$KEXT
sudo chown -R root:wheel /Library/Extensions/$KEXT

# Copy daemon & set permissions
sudo rm -f /var/logs/iscsid.log
sudo cp $SOURCE_PATH/$DAEMON /System/Library/LaunchDaemons
sudo cp ./User\ Tools/com.github.iscsi-osx.iscsid.plist /System/Library/LaunchDaemons
sudo chmod -R 744 /System/Library/LaunchDaemons/$DAEMON
sudo chown -R root:wheel /System/Library/LaunchDaemons/$DAEMON
sudo chmod 644 /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo chown root:wheel /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist

# Copy user tool
sudo cp $SOURCE_PATH/$TOOL /usr/bin/$TOOL
sudo chmod +x /usr/bin/$TOOL

# Copy man page
sudo cp ./User\ Tools/iscsictl.8 /usr/share/man/man8
sudo cp ./User\ Tools/iscsid.8 /usr/share/man/man8

# Load kernel extension
sudo kextload /Library/Extensions/$KEXT

# Start daemon
sudo launchctl load /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo launchctl start com.github.iscsi-osx.iscsid
