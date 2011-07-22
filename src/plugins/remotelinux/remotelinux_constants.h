#ifndef REMOTELINUX_CONSTANTS_H
#define REMOTELINUX_CONSTANTS_H

namespace RemoteLinux {
namespace Constants {

const char RemoteLinuxSettingsCategory[] = "X.Maemo";
const char RemoteLinuxSettingsTrCategory[]
    = QT_TRANSLATE_NOOP("RemoteLinux", "Linux Devices");
const char RemoteLinuxSettingsCategoryIcon[] = ":/projectexplorer/images/MaemoDevice.png";

const char GenericTestDeviceActionId[] = "RemoteLinux.GenericTestDeviceAction";
const char GenericDeployKeyToDeviceActionId[] = "RemoteLinux.GenericDeployKeyToDeviceAction";
const char GenericRemoteProcessesActionId[] = "RemoteLinux.GenericRemoteProcessesAction";

} // Constants
} // RemoteLinux

#endif // REMOTELINUX_CONSTANTS_H
