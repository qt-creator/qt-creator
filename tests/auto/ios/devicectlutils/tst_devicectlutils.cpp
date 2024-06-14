// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../../../../src/plugins/ios/devicectlutils.h"

#include <QtTest>

using namespace Ios::Internal;

class tst_Devicectlutils : public QObject
{
    Q_OBJECT

private slots:
    void parseError_data();
    void parseError();

    void parseDeviceInfo_data();
    void parseDeviceInfo();

    void parseAppInfo();

    void parseProcessIdentifier();

    void parseLaunchResult();
};

void tst_Devicectlutils::parseError_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("error");
    QTest::addRow("deploy-missing-plist") << QByteArray(R"raw(
{
  "error" : {
    "code" : 3002,
    "domain" : "com.apple.dt.CoreDeviceError",
    "userInfo" : {
      "NSLocalizedDescription" : {
        "string" : "Failed to install the app on the device."
      },
      "NSUnderlyingError" : {
        "error" : {
          "code" : 3000,
          "domain" : "com.apple.dt.CoreDeviceError",
          "userInfo" : {
            "NSLocalizedDescription" : {
              "string" : "The item at cmake_widgets.app is not a valid bundle."
            },
            "NSLocalizedFailureReason" : {
              "string" : "Failed to get the identifier for the app to be installed."
            },
            "NSLocalizedRecoverySuggestion" : {
              "string" : "Ensure that your bundle's Info.plist contains a value for the CFBundleIdentifier key."
            },
            "NSURL" : {
              "url" : "file:///Users/user/temp/build/build-cmake_widgets-Qt_6_6_1_for_iOS/Debug-iphoneos/cmake_widgets.app/"
            }
          }
        }
      },
      "NSURL" : {
        "url" : "file:///Users/user/temp/build/build-cmake_widgets-Qt_6_6_1_for_iOS/Debug-iphoneos/cmake_widgets.app/"
      }
    }
  },
  "info" : {
    "arguments" : [
      "devicectl",
      "device",
      "install",
      "app",
      "--device",
      "00000000-0000000000000000",
      "--quiet",
      "--json-output",
      "-",
      "/Users/user/temp/build/build-cmake_widgets-Qt_6_6_1_for_iOS/Debug-iphoneos/cmake_widgets.app"
    ],
    "commandType" : "devicectl.device.install.app",
    "environment" : {
      "TERM" : "xterm-256color"
    },
    "jsonVersion" : 2,
    "outcome" : "failed",
    "version" : "355.7.7"
  }
}
 )raw") << QString(R"raw(Operation failed: Failed to install the app on the device.
The item at cmake_widgets.app is not a valid bundle.
Failed to get the identifier for the app to be installed.
Ensure that your bundle's Info.plist contains a value for the CFBundleIdentifier key.)raw");
    QTest::addRow("deploy-device-gone") << QByteArray(R"raw(
{
  "error" : {
    "code" : 1,
    "domain" : "com.apple.CoreDevice.ControlChannelConnectionError",
    "userInfo" : {
      "NSLocalizedDescription" : {
        "string" : "Internal logic error: Connection was invalidated"
      },
      "NSUnderlyingError" : {
        "error" : {
          "code" : 0,
          "domain" : "com.apple.CoreDevice.ControlChannelConnectionError",
          "userInfo" : {
            "NSLocalizedDescription" : {
              "string" : "Transport error"
            },
            "NSUnderlyingError" : {
              "error" : {
                "code" : 60,
                "domain" : "Network.NWError",
                "userInfo" : {
                  "NSDescription" : {
                    "string" : "Operation timed out"
                  }
                }
              }
            }
          }
        }
      }
    }
  },
  "info" : {
    "arguments" : [
      "devicectl",
      "device",
      "install",
      "app",
      "--device",
      "00000000-0000000000000000",
      "--quiet",
      "--json-output",
      "-",
      "/Users/user/temp/build/build-cmake_widgets-Qt_6_6_1_for_iOS/Debug-iphoneos/cmake_widgets.app"
    ],
    "commandType" : "devicectl.device.install.app",
    "environment" : {
      "TERM" : "xterm-256color"
    },
    "jsonVersion" : 2,
    "outcome" : "failed",
    "version" : "355.7.7"
  }
}
)raw") << QString(R"raw(Operation failed: Internal logic error: Connection was invalidated
Transport error)raw");
}

void tst_Devicectlutils::parseError()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, error);
    const Utils::expected_str<QJsonValue> result = parseDevicectlResult(data);
    if (error.isEmpty()) {
        QVERIFY(result);
    } else {
        QVERIFY(!result);
        QCOMPARE(result.error(), error);
    }
}

void tst_Devicectlutils::parseDeviceInfo_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("usbId");
    QTest::addColumn<QString>("error");
    QTest::addColumn<QMap<QString, QString>>("info");

    const QByteArray data(R"raw(
{
  "info" : {
    "arguments" : [
      "devicectl",
      "list",
      "devices",
      "--quiet",
      "--json-output",
      "-"
    ],
    "commandType" : "devicectl.list.devices",
    "environment" : {
      "TERM" : "xterm-256color"
    },
    "jsonVersion" : 2,
    "outcome" : "success",
    "version" : "355.7.7"
  },
  "result" : {
    "devices" : [
      {
        "capabilities" : [
          {
            "featureIdentifier" : "com.apple.coredevice.feature.connectdevice",
            "name" : "Connect to Device"
          },
          {
            "featureIdentifier" : "com.apple.coredevice.feature.acquireusageassertion",
            "name" : "Acquire Usage Assertion"
          },
          {
            "featureIdentifier" : "com.apple.coredevice.feature.unpairdevice",
            "name" : "Unpair Device"
          }
        ],
        "connectionProperties" : {
          "authenticationType" : "manualPairing",
          "isMobileDeviceOnly" : false,
          "lastConnectionDate" : "2024-01-29T08:49:25.179Z",
          "pairingState" : "paired",
          "potentialHostnames" : [
            "00000000-0000000000000000.coredevice.local",
            "00000000-0000-0000-0000-000000000000.coredevice.local"
          ],
          "transportType" : "wired",
          "tunnelState" : "disconnected",
          "tunnelTransportProtocol" : "tcp"
        },
        "deviceProperties" : {
          "bootedFromSnapshot" : true,
          "bootedSnapshotName" : "com.apple.os.update-0000",
          "ddiServicesAvailable" : false,
          "developerModeStatus" : "enabled",
          "hasInternalOSBuild" : false,
          "name" : "Some iOS device",
          "osBuildUpdate" : "21D50",
          "osVersionNumber" : "17.3",
          "rootFileSystemIsWritable" : false
        },
        "hardwareProperties" : {
          "cpuType" : {
            "name" : "arm64e",
            "subType" : 2,
            "type" : 16777228
          },
          "deviceType" : "iPad",
          "ecid" : 0,
          "hardwareModel" : "J211AP",
          "internalStorageCapacity" : 64000000000,
          "isProductionFused" : true,
          "marketingName" : "iPad mini (5th generation)",
          "platform" : "iOS",
          "productType" : "iPad11,2",
          "reality" : "physical",
          "serialNumber" : "000000000000",
          "supportedCPUTypes" : [
            {
              "name" : "arm64e",
              "subType" : 2,
              "type" : 16777228
            },
            {
              "name" : "arm64",
              "subType" : 0,
              "type" : 16777228
            },
            {
              "name" : "arm64",
              "subType" : 1,
              "type" : 16777228
            },
            {
              "name" : "arm64_32",
              "subType" : 1,
              "type" : 33554444
            }
          ],
          "supportedDeviceFamilies" : [
            1,
            2
          ],
          "thinningProductType" : "iPad11,2",
          "udid" : "00000000-0000000000000000"
        },
        "identifier" : "00000000-0000-0000-0000-000000000000",
        "visibilityClass" : "default"
      }
    ]
  }
})raw");

    QTest::addRow("handled device")
        << data << QString("000000000000000000000000") << QString()
        << QMap<QString, QString>({{"cpuArchitecture", "arm64e"},
                                   {"developerStatus", "Development"},
                                   {"deviceConnected", "YES"},
                                   {"deviceName", "Some iOS device"},
                                   {"osVersion", "17.3 (21D50)"},
                                   {"productType", "iPad11,2"},
                                   {"uniqueDeviceId", "00000000-0000000000000000"}});
    QTest::addRow("unhandled device")
        << data << QString("000000000000000000000001")
        << QString("Device is not handled by devicectl") << QMap<QString, QString>({});
}

void tst_Devicectlutils::parseDeviceInfo()
{
    using InfoMap = QMap<QString, QString>;
    QFETCH(QByteArray, data);
    QFETCH(QString, usbId);
    QFETCH(QString, error);
    QFETCH(InfoMap, info);

    const Utils::expected_str<InfoMap> result = Ios::Internal::parseDeviceInfo(data, usbId);
    if (error.isEmpty()) {
        QVERIFY(result);
        QCOMPARE(*result, info);
    } else {
        QVERIFY(!result);
        QCOMPARE(result.error(), error);
    }
}

void tst_Devicectlutils::parseAppInfo()
{
    const QByteArray data(R"raw(
{
  "info" : {
    "arguments" : [
      "devicectl",
      "device",
      "info",
      "apps",
      "--device",
      "00000000-0000000000000000",
      "--quiet",
      "--json-output",
      "-"
    ],
    "commandType" : "devicectl.device.info.apps",
    "environment" : {
      "TERM" : "xterm-256color"
    },
    "jsonVersion" : 2,
    "outcome" : "success",
    "version" : "355.7.7"
  },
  "result" : {
    "apps" : [
      {
        "appClip" : false,
        "builtByDeveloper" : true,
        "bundleIdentifier" : "org.iuehrg.cmake-widgets",
        "bundleVersion" : "0.1",
        "defaultApp" : false,
        "hidden" : false,
        "internalApp" : false,
        "name" : "cmake_widgets",
        "removable" : true,
        "url" : "file:///private/var/containers/Bundle/Application/FAEC04B7-41E6-4A3C-952E-D89792DA053C/cmake_widgets.app/",
        "version" : "0.1"
      }
    ],
    "defaultAppsIncluded" : false,
    "deviceIdentifier" : "00000000-0000-0000-0000-000000000000",
    "hiddenAppsIncluded" : false,
    "internalAppsIncluded" : false,
    "removableAppsIncluded" : true
  }
})raw");

    const Utils::expected_str<QUrl> result
        = Ios::Internal::parseAppInfo(data, "org.iuehrg.cmake-widgets");
    QVERIFY(result);
    QCOMPARE(*result,
             QUrl("file:///private/var/containers/Bundle/Application/"
                  "FAEC04B7-41E6-4A3C-952E-D89792DA053C/cmake_widgets.app/"));
}

void tst_Devicectlutils::parseProcessIdentifier()
{
    const QByteArray data(R"raw(
{
  "info" : {
    "arguments" : [
      "devicectl",
      "device",
      "info",
      "processes",
      "--device",
      "00000000-0000000000000000",
      "--quiet",
      "--json-output",
      "-",
      "--filter",
      "executable.path BEGINSWITH '/private/var/containers/Bundle/Application/00000000-0000-0000-0000-000000000000/test.app'"
    ],
    "commandType" : "devicectl.device.info.processes",
    "environment" : {
      "TERM" : "xterm-256color"
    },
    "jsonVersion" : 2,
    "outcome" : "success",
    "version" : "355.7.7"
  },
  "result" : {
    "deviceIdentifier" : "00000000-0000-0000-0000-000000000000",
    "runningProcesses" : [
      {
        "executable" : "file:///private/var/containers/Bundle/Application/00000000-0000-0000-0000-000000000000/test.app/test",
        "processIdentifier" : 1000
      }
    ]
  }
})raw");

    const Utils::expected_str<qint64> result = Ios::Internal::parseProcessIdentifier(data);
    QVERIFY(result);
    QCOMPARE(*result, 1000);
}

void tst_Devicectlutils::parseLaunchResult()
{
    const QByteArray data(R"raw(
{
  "info" : {
    "arguments" : [
      "devicectl",
      "device",
      "process",
      "launch",
      "--device",
      "00000000-0000000000000000",
      "--quiet",
      "--json-output",
      "-",
      "org.iuehrg.cmake-widgets"
    ],
    "commandType" : "devicectl.device.process.launch",
    "environment" : {
      "TERM" : "xterm-256color"
    },
    "jsonVersion" : 2,
    "outcome" : "success",
    "version" : "355.7.7"
  },
  "result" : {
    "deviceIdentifier" : "00000000-0000-0000-0000-000000000000",
    "launchOptions" : {
      "activatedWhenStarted" : true,
      "arguments" : [

      ],
      "environmentVariables" : {
        "TERM" : "xterm-256color"
      },
      "platformSpecificOptions" : {

      },
      "startStopped" : false,
      "terminateExistingInstances" : false,
      "user" : {
        "active" : true
      }
    },
    "process" : {
      "auditToken" : [
        4294967295,
        501,
        501,
        501,
        501,
        1802,
        0,
        5118
      ],
      "executable" : "file:///private/var/containers/Bundle/Application/00000000-0000-0000-0000-000000000000/test.app/test",
      "processIdentifier" : 1802
    }
  }
})raw");
    const Utils::expected_str<qint64> result = Ios::Internal::parseLaunchResult(data);
    QVERIFY(result);
    QCOMPARE(*result, 1802);
}

QTEST_GUILESS_MAIN(tst_Devicectlutils)

#include "tst_devicectlutils.moc"
