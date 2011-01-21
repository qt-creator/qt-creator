/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef S60PUBLISHEROVI_H
#define S60PUBLISHEROVI_H

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtGui/QColor>

#include "qt4buildconfiguration.h"

namespace ProjectExplorer {
class Project;
}

namespace Qt4ProjectManager {
class Qt4Project;
namespace Internal {
class Qt4SymbianTarget;
class ProFileReader;

namespace Constants {
const char * const REJECTED_VENDOR_NAMES_VENDOR = "Vendor";
const char * const REJECTED_VENDOR_NAMES_VENDOR_EN = "Vendor-EN";
const char * const REJECTED_VENDOR_NAMES_NOKIA = "Nokia";
const char * const REJECTED_VENDOR_NAMES_EMPTY = "";

const char * const CERTIFIED_SIGNED_CAPABILITY_NETWORK_CONTROL = "NetworkControl";
const char * const CERTIFIED_SIGNED_CAPABILITY_MULTIMEDIA_DD = "MultimediaDD";
const char * const CERTIFIED_SIGNED_CAPABILITY_COMM_DD = "CommDD";
const char * const CERTIFIED_SIGNED_CAPABILITY_DISK_ADMIN = "DiskAdmin";
const char * const MANUFACTURER_APPROVED_CAPABILITY_ALL_FILES = "AllFiles";
const char * const MANUFACTURER_APPROVED_CAPABILITY_DRM = "DRM";
const char * const MANUFACTURER_APPROVED_CAPABILITY_TCB = "TCB";
}

class S60PublisherOvi : public QObject
{
    Q_OBJECT

public:
    enum UID3Ranges {
        //    Protected UID range:   0x00000000 - 0x7FFFFFFF
        //    Unprotected UID range: 0x80000000 - 0xFFFFFFFF
        //
        //    Specifically, there are two important unprotected UID ranges:
        //    UIDs from Symbian Signed:         0xA0000000 - 0xAFFFFFFF
        //    UIDs for test/development use:    0xE0000000 - 0xEFFFFFFF
        //
        //    And one important protected range:
        //    UIDs from Ovi Sign: 0x20000000 to 0x2FFFFFFF
        //    Warning: Some of these UIDs are assigned by Symbiansigned.
        //    Apps with such UIDs cannot be signed by Ovi.
        //    It is currently impossible to say which UIDs have been assigned by whome.

        AssignedRestrictedStart = 0x20000000,
        AssignedRestrictedEnd = 0x2FFFFFFF,
        SymbianSignedUnprotectedStart = 0xA0000000,
        SymbianSignedUnprotectedEnd = 0xAFFFFFFF,
        TestStart = 0xE0000000,
        TestEnd = 0xEFFFFFFF
    };

    enum CapabilityLevel {
        CertifiedSigned,
        ManufacturerApproved
    };

public:
    explicit S60PublisherOvi(QObject *parent = 0);
    ~S60PublisherOvi();

    void setBuildConfiguration(Qt4BuildConfiguration *qt4bc);
    void cleanUp();
    void completeCreation();

    QString getGlobalVendorNameFromProFile();
    QString getLocalisedVendorNamesFromProFile();
    bool isVendorNameValid(const QString &vendorName);

    QString getQtVersionFromProFile();

    QString getUID3FromProFile();
    bool isUID3Valid(const QString &uid3);
    bool isTestUID3(const QString &uid3);
    bool isKnownSymbianSignedUID3(const QString &uid3);

    QString getCapabilitiesFromProFile();
    bool isCapabilityOneOf(const QString &capability, CapabilityLevel level);

    void updateProFile();
    void updateProFile(const QString &var, const QString &values);
    void buildSis();

    QString getCreatedSisFileContainingFolder();
    QString getCreatedSisFilePath();

    bool hasSucceeded();

    void setVendorName(const QString &vendorName);
    void setLocalVendorName(const QString &localVendorName);
    void setAppUid(const QString &appuid);

signals:
    void progressReport(const QString& status, QColor c);
    void succeeded();

public slots:
    void runQMake();
    void runBuild(int result);
    void runCreateSis(int result);
    void endBuild(int result);

private:
    void runStep(int result, const QString& buildStep, const QString& command, QProcess* currProc, QProcess* prevProc);

    QColor m_errorColor;
    QColor m_commandColor;
    QColor m_okColor;
    QColor m_normalColor;

    QProcess* m_qmakeProc;
    QProcess* m_buildProc;
    QProcess* m_createSisProc;

    Qt4BuildConfiguration * m_qt4bc;
    const Qt4SymbianTarget * m_activeTargetOfProject;
    Qt4Project * m_qt4project;
    ProFileReader *m_reader;
    QStringList m_rejectedVendorNames;
    QStringList m_capabilitiesForCertifiedSigned;
    QStringList m_capabilitesForManufacturerApproved;
    QString m_vendorName;
    QString m_localVendorName;
    QString m_appUid;

    bool m_finishedAndSuccessful;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60PUBLISHEROVI_H
