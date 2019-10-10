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
sudo mkdir -p $KEXT_DST
sudo cp -R $SOURCE_PATH/$KEXT $KEXT_DST/$KEXT
sudo chmod -R 755 $KEXT_DST/$KEXT
sudo chown -R root:wheel $KEXT_DST/$KEXT

# Copy framework
sudo mkdir -p $FRAMEWORK_DST
sudo cp -R $SOURCE_PATH/$FRAMEWORK $FRAMEWORK_DST/$FRAMEWORK
sudo chown -R root:wheel $FRAMEWORK_DST/$FRAMEWORK
sudo chmod -R 755 $FRAMEWORK_DST/$FRAMEWORK

# Copy daemon & set permissions
sudo rm -f /var/logs/iscsid.log
sudo mkdir -p $DAEMON_DST
sudo cp $SOURCE_PATH/$DAEMON $DAEMON_DST/$DAEMON
sudo cp $SOURCE_PATH/$DAEMON_PLIST $DAEMON_PLIST_DST
sudo chmod -R 744 $DAEMON_DST/$DAEMON
sudo chown -R root:wheel $DAEMON_DST/$DAEMON
sudo chmod 644 $DAEMON_PLIST_DST/$DAEMON_PLIST
sudo chown root:wheel $DAEMON_PLIST_DST/$DAEMON_PLIST

# Copy user tool
sudo mkdir -p $TOOL_DST
sudo cp $SOURCE_PATH/$TOOL $TOOL_DST/$TOOL
sudo chmod +x $TOOL_DST/$TOOL

# Copy man page
sudo mkdir -p $MAN_DST
sudo cp $SOURCE_PATH/$MAN_TOOL $MAN_DST
sudo cp $SOURCE_PATH/$MAN_DAEMON $MAN_DST

# Load kernel extension
sudo kextload $KEXT_DST/$KEXT

# Start daemon
sudo launchctl load $DAEMON_PLIST_DST/$DAEMON_PLIST
sudo launchctl start $DAEMON_PLIST

# Remove (old) configuration file
sudo rm -f $PREF_DST/$PREF_FILE

# Flush preferences cache
sudo killall cfprefsd
