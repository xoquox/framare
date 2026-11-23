APP_ID = harbour-framare
APP_NAME = Framare
APP_ORGANIZATION = harbour
APP_INSTANCE = framare
APP_DBUS_NAME = $${APP_ORGANIZATION}.$${APP_INSTANCE}
APP_TRANSPORT = /usr/libexec/$$APP_ID/unframeo

DEFINES += FRAMARE_NAME=\\\"$$APP_NAME\\\" \
    FRAMARE_ORGANIZATION=\\\"$$APP_ORGANIZATION\\\" \
    FRAMARE_INSTANCE=\\\"$$APP_INSTANCE\\\" \
    FRAMARE_DBUS_NAME=\\\"$$APP_DBUS_NAME\\\" \
    FRAMARE_CONTROL_INTERFACE=\\\"$${APP_DBUS_NAME}.Control\\\" \
    FRAMARE_TRANSPORT=\\\"$$APP_TRANSPORT\\\"

DESKTOP_LINES = "[Desktop Entry]" "Type=Application" \
    "X-Nemo-Application-Type=silica-qt5" "Name=$$APP_NAME" \
    "Exec=$$APP_ID" "Icon=$$APP_ID" "X-Share-Methods=images;" \
    "[X-Sailjail]" "OrganizationName=$$APP_ORGANIZATION" \
    "ApplicationName=$$APP_INSTANCE" "Permissions=Internet;Pictures;MediaIndexing;RemovableMedia;" \
    "[X-Share Method images]" "Description=$$APP_NAME – Send to a photo frame" \
    "Description[de]=$$APP_NAME – An Bilderrahmen senden" \
    "Capabilities=image/jpeg;image/png;" "SupportsMultipleFiles=yes"
write_file($$OUT_PWD/$${APP_ID}.desktop, DESKTOP_LINES)
