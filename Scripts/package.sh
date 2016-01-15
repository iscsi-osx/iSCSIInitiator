# Package parameters
NAME="iSCSI Initiator for OS X"
VERSION=$(cd ../ ; agvtool what-version -terse)

# Set output paths
PACKAGE="../Package"
RELEASE="../Package/Release/"
BUNDLE_ID="com.github.iscsi-osx.iSCSIInitiator"

INSTALLER_SCRIPT="../Package/Scripts/Installer"
UNINSTALLER_SCRIPT="../Package/Scripts/Uninstaller"

INSTALLER_PATH="../Package/Components/Installer.pkg"
UNINSTALLER_PATH="../Package/Components/Uninstaller.pkg"

# DMG parameters
DMG_COMPONENTS="../Package/Components"
DMG="../Package/iSCSIInitiator.dmg"
DMG_SIZE=10000

# Relelase build of all three components
xcodebuild -workspace ../iSCSIInitiator.xcodeproj/project.xcworkspace \
           -scheme iSCSIInitiator -configuration release BUILD_DIR=$PACKAGE
xcodebuild -workspace ../iSCSIInitiator.xcodeproj/project.xcworkspace \
           -scheme iscsid -configuration release BUILD_DIR=$PACKAGE
xcodebuild -workspace ../iSCSIInitiator.xcodeproj/project.xcworkspace \
            -scheme iscsictl -configuration release BUILD_DIR=$PACKAGE

# Package the installer
pkgbuild --root $RELEASE \
	--identifier $BUNDLE_ID \
	--install-location /tmp/ \
    --scripts $INSTALLER_SCRIPT \
	--version $VERSION \
    $INSTALLER_PATH

# Package the uninstaller
pkgbuild --nopayload \
    --identifier $BUNDLE_ID \
    --scripts $UNINSTALLER_SCRIPT \
    --version $VERSION \
    $UNINSTALLER_PATH


# Build the DMG
rm -f $DMG
hdiutil create -srcfolder $DMG_COMPONENTS -volname "$NAME" -fs HFS+ \
    -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${DMG_SIZE}k $PACKAGE/$DMG

# Load the DMG
device=$(hdiutil attach -readwrite -noverify -noautoopen $DMG | \
    egrep '^/dev/' | sed 1q | awk '{print $1}')

sleep 2

# Modify DMG style
echo '
tell application "Finder"
tell disk "'${NAME}'"
open
set current view of container window to icon view
set toolbar visible of container window to false
set statusbar visible of container window to false
set the bounds of container window to {400, 100, 885, 430}
set theViewOptions to the icon view options of container window
set arrangement of theViewOptions to not arranged
set icon size of theViewOptions to 72
set position of item "'${applicationName}'" of container window to {100, 100}
set position of item "Applications" of container window to {375, 100}
update without registering applications
delay 5
close
end tell
end tell
' | osascript

# Set permissions & compress
chmod -Rf go-w /Volumes/"${NAME}"
sync
sync
hdiutil detach ${device}
hdiutil convert $DMG -format UDZO -imagekey zlib-level=9 -o $DMG.final
rm -f $DMG
mv $DMG.final $DMG