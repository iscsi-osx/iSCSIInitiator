#! /bin/bash

# Define targets
DAEMON=iscsid
TOOL=iscsictl
KEXT=iSCSIInitiator.kext

# Look for build products in places Xcode might place them.
for BUILD_PATH in \
            ../DerivedData/Build/Products/Debug \
            ../DerivedData/iSCSIInitiator/Build/Products/Debug \
            ~/Library/Developer/Xcode/DerivedData/iSCSIInitiator*/Build/Products/Debug \
            ; do
    if [ -d "${BUILD_PATH}" ]; then
        SOURCE_PATH="${BUILD_PATH}"
        break;
    fi
done

if [ X"" == X"${SOURCE_PATH}" ]; then
    echo "Unable to locate iSCSIInitiator binaries; did you run build.sh without errors?"
    exit 1
fi

# Copy kernel extension & load it
sudo cp -R $SOURCE_PATH/$KEXT /Library/Extensions/$KEXT
sudo chmod -R 755 /Library/Extensions/$KEXT
sudo chown -R root:wheel /Library/Extensions/$KEXT

# Copy daemon & set permissions
sudo rm -f /var/logs/iscsid.log
sudo cp $SOURCE_PATH/$DAEMON /Library/PrivilegedHelperTools/$DAEMON
sudo cp ../User\ Tools/com.github.iscsi-osx.iscsid.plist /System/Library/LaunchDaemons
sudo chmod -R 744 /Library/PrivilegedHelperTools/$DAEMON
sudo chown -R root:wheel /Library/PrivilegedHelperTools/$DAEMON
sudo chmod 644 /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo chown root:wheel /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist

# Copy user tool
sudo cp $SOURCE_PATH/$TOOL /usr/local/bin/$TOOL
sudo chmod +x /usr/local/bin/$TOOL

# Copy man page
sudo cp ../User\ Tools/iscsictl.8 /usr/share/man/man8
sudo cp ../User\ Tools/iscsid.8 /usr/share/man/man8

# Load kernel extension
sudo kextload /Library/Extensions/$KEXT

# Start daemon
sudo launchctl load /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo launchctl start com.github.iscsi-osx.iscsid
