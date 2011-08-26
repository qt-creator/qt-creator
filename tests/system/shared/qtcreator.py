import platform;
import shutil;
import os;
import glob;

SDKPath = ''
SettingsPath = ''
testSettings.logScreenshotOnFail = True

source("../../shared/utils.py")
source("../../shared/mainwin.py")

if platform.system() in ('Windows', 'Microsoft'):
    SDKPath = "C:/QtSDK/src"
    cwd = os.getcwd()       # current dir is directory holding qtcreator.py
    cwd+="/../../settings/windows"
    cwd = os.path.abspath(cwd)
    SettingsPath = " -settingspath %s" % cwd
else:
    SDKPath = os.path.expanduser("~/QtSDK/src")
    cwd = os.getcwd()       # current dir is directory holding qtcreator.py
    cwd+="/../../settings/unix"
    cwd = os.path.abspath(cwd)
    SettingsPath = " -settingspath %s" % cwd
