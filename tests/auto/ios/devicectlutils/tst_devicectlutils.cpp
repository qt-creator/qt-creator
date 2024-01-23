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

QTEST_GUILESS_MAIN(tst_Devicectlutils)

#include "tst_devicectlutils.moc"
