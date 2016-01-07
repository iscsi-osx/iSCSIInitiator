if [ ! -f /usr/bin/xcrun ]; then
    echo "Xcode command-line tools not installed. Starting the installer."
    sudo xcode-select --install
    echo "Please rerun the build command."
    exit 1
fi
xcodebuild -scheme iSCSIDaemon build
xcodebuild -scheme iscsictl build
xcodebuild -scheme iSCSIInitiator build