#! /bin/bash

# Define targets
DAEMON=iscsid
TOOL=iscsictl
KEXT=iSCSIInitiator.kext
FRAMEWORK=iSCSI.framework

# Get minor version of the OS
OSX_MINOR_VER=$(sw_vers -productVersion | awk -F '.' '{print $2}')

# Minor version of OS X Mavericks
OSX_MAVERICKS_MINOR_VER="9"

if [ "$OSX_MINOR_VER" -ge "$OSX_MAVERICKS_MINOR_VER" ]; then
    KEXT_DST=/Library/Extensions
else
    KEXT_DST=/System/Library/Extensions
fi

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
sudo cp -R $SOURCE_PATH/$KEXT $KEXT_DST/$KEXT
sudo chmod -R 755 $KEXT_DST/$KEXT
sudo chown -R root:wheel $KEXT_DST/$KEXT

# Copy framework
sudo cp -R $SOURCE_PATH/$FRAMEWORK /Library/Frameworks/$FRAMEWORK

# Copy daemon & set permissions
sudo rm -f /var/logs/iscsid.log
sudo mkdir -p /Library/PrivilegedHelperTools/
sudo cp $SOURCE_PATH/$DAEMON /Library/PrivilegedHelperTools/$DAEMON
sudo cp $SOURCE_PATH/com.github.iscsi-osx.iscsid.plist /Library/LaunchDaemons
sudo chmod -R 744 /Library/PrivilegedHelperTools/$DAEMON
sudo chown -R root:wheel /Library/PrivilegedHelperTools/$DAEMON
sudo chmod 644 /Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo chown root:wheel /Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist

# Copy user tool
sudo cp $SOURCE_PATH/$TOOL /usr/local/bin/$TOOL
sudo chmod +x /usr/local/bin/$TOOL

# Copy man page
sudo cp $SOURCE_PATH/iscsictl.8 /usr/share/man/man8
sudo cp $SOURCE_PATH/iscsid.8 /usr/share/man/man8

# Load kernel extension
sudo kextload /Library/Extensions/$KEXT

# Start daemon
sudo launchctl load /Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo launchctl start com.github.iscsi-osx.iscsid
