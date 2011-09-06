import platform;
import shutil;
import os;
import glob;
import atexit;
import codecs;

SDKPath = ''
SettingsPath = ''
tmpSettingsDir = ''
testSettings.logScreenshotOnFail = True

source("../../shared/utils.py")
source("../../shared/build_utils.py")
source("../../shared/mainwin.py")
source("../../shared/qtquick.py")

def removeTmpSettingsDir():
    appCtxt = currentApplicationContext()
    waitFor("appCtxt.isRunning==False")
    deleteDirIfExists(os.path.dirname(tmpSettingsDir))

if platform.system() in ('Windows', 'Microsoft'):
    SDKPath = "C:/QtSDK/src"
    cwd = os.getcwd()       # current dir is directory holding qtcreator.py
    cwd+="/../../settings/windows"
else:
    SDKPath = os.path.expanduser("~/QtSDK/src")
    cwd = os.getcwd()       # current dir is directory holding qtcreator.py
    cwd+="/../../settings/unix"

cwd = os.path.abspath(cwd)
tmpSettingsDir = tempDir()
tmpSettingsDir = os.path.abspath(tmpSettingsDir+"/settings")
shutil.copytree(cwd, tmpSettingsDir)
# the following only doesn't work if the test ends in an exception
atexit.register(removeTmpSettingsDir)
SettingsPath = " -settingspath %s" % tmpSettingsDir

