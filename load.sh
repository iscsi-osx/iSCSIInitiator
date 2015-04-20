sudo cp -R ./DerivedData/iSCSI_Initiator/Build/Products/Debug/iSCSIInitiator.kext /tmp/
sudo chmod -R 755 /tmp/iSCSIInitiator.kext
sudo chown -R root:wheel /tmp/iSCSIInitiator.kext
sudo kextload /tmp/iSCSIInitiator.kext

sudo rm -f /var/logs/iscsid.log
sudo cp ./DerivedData/iSCSI_Initiator/Build/Products/Debug/iscsid /tmp/iscsid
sudo cp ./User\ Tools/com.github.iscsi-osx.iscsid.plist /System/Library/LaunchDaemons

sudo chmod -R 744 /tmp/iscsid
sudo chown -R root:wheel /tmp/iscsid
sudo chmod 644 /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo chown root:wheel /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist

sudo launchctl load /System/Library/LaunchDaemons/com.github.iscsi-osx.iscsid.plist
sudo launchctl start com.github.iscsi-osx.iscsid
