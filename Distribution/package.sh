# Package parameters
NAME="iSCSI Initiator for OS X"
BUNDLE_ID="com.github.iscsi-osx.iSCSIInitiator"
VERSION=$(cd ../ ; agvtool what-version -terse)

# Output of final DMG
RELEASE="../Release"

# DMG parameters
DMG_BASE_NAME="iSCSIInitiator"
DMG_SIZE=10000

# XCode temporary build path for release binaries
XCODE_RELEASE_BUILD_DIR="tmp"

# Temporary path with package and DMG components
TMP_ROOT="../tmp"
TMP_PACKAGE_DIR="../tmp/Packages"

# Kernel extension path (built)
KEXT_PATH=../$XCODE_RELEASE_BUILD_DIR/Release/iSCSIInitiator.kext
DAEMON_PATH=KEXT_PATH=../$XCODE_RELEASE_BUILD_DIR/Release/iscsid
TOOL_PATH=KEXT_PATH=../$XCODE_RELEASE_BUILD_DIR/Release/iscsictl

# Location of installer and uninstaller scripts
INSTALLER_SCRIPT="Scripts/Installer"
UNINSTALLER_SCRIPT="Scripts/Uninstaller"

# Dialog title for installer and uninstaller scripts
INSTALLER_TITLE="iSCSI Initiator Installer"
UNINSTALLER_TITLE="iSCSI Initiator Uninstaller"

# Output path of intaller and uninstaller packages
INSTALLER_PATH="../tmp/Packages/Installer.pkg"
UNINSTALLER_PATH="../tmp/Packages/Uninstaller.pkg"

# Path of installer and uninstaller distribution XML files
INSTALLER_DIST_XML="Resources/Installer.xml"
UNINSTALLER_DIST_XML="Resources/Uninstaller.xml"

# Requirements
REQUIREMENTS_PATH="Resources/Requirements.plist"

# Relelase build of all three components
xcodebuild -workspace ../iSCSIInitiator.xcodeproj/project.xcworkspace \
           -scheme iSCSIInitiator -configuration release BUILD_DIR=$XCODE_RELEASE_BUILD_DIR
xcodebuild -workspace ../iSCSIInitiator.xcodeproj/project.xcworkspace \
           -scheme iscsid -configuration release BUILD_DIR=$XCODE_RELEASE_BUILD_DIR
xcodebuild -workspace ../iSCSIInitiator.xcodeproj/project.xcworkspace \
            -scheme iscsictl -configuration release BUILD_DIR=$XCODE_RELEASE_BUILD_DIR

# Create folder for pkg output (.pkg files for DMG)
mkdir -p $TMP_PACKAGE_DIR
mkdir -p $RELEASE

# Package the installer
pkgbuild --root ../$XCODE_RELEASE_BUILD_DIR/Release \
    --identifier $BUNDLE_ID \
    --install-location /tmp/ \
    --scripts $INSTALLER_SCRIPT \
    --version $VERSION \
    $INSTALLER_PATH.tmp

# Package the uninstaller
pkgbuild --nopayload \
    --identifier $BUNDLE_ID \
    --scripts $UNINSTALLER_SCRIPT \
    --version $VERSION \
    $UNINSTALLER_PATH.tmp

# Put packages inside a product archive
productbuild --distribution $INSTALLER_DIST_XML \
    --package-path $TMP_PACKAGE_DIR \
    --product $REQUIREMENTS_PATH \
    $INSTALLER_PATH

productbuild --distribution $UNINSTALLER_DIST_XML \
    --package-path $TMP_PACKAGE_DIR \
    --product $REQUIREMENTS_PATH \
    $UNINSTALLER_PATH

# Cleanup temporary packages, leaving final pacakges for DMG
rm $INSTALLER_PATH.tmp
rm $UNINSTALLER_PATH.tmp

# Build the DMG
hdiutil create -srcfolder $TMP_PACKAGE_DIR -volname "$NAME" -fs HFS+ \
    -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${DMG_SIZE}k $TMP_ROOT/$DMG_BASE_NAME.dmg

# Load the DMG
device=$(hdiutil attach -readwrite -noverify -noautoopen $TMP_ROOT/$DMG_BASE_NAME.dmg | \
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

# Create final output
rm -f $RELEASE/$DMG_BASE_NAME-$VERSION.dmg
hdiutil convert $TMP_ROOT/$DMG_BASE_NAME.dmg -format UDZO -imagekey zlib-level=9 -o $RELEASE/$DMG_BASE_NAME-$VERSION.dmg

# Cleanup
rm -r $TMP_ROOT

