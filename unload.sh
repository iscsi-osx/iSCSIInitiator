sudo kextunload /tmp/iSCSIInitiator.kext
sudo launchctl stop /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid
sudo launchctl unload /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist

