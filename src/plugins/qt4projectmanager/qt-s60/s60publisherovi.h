/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#ifndef S60PUBLISHEROVI_H
#define S60PUBLISHEROVI_H

#include <QObject>
#include <QColor>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace QtSupport {
class ProFileReader;
}

namespace ProjectExplorer {
class Project;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4Project;
namespace Internal {
class Qt4SymbianTarget;
class S60PublishStep;

namespace Constants {
const char REJECTED_VENDOR_NAMES_VENDOR[] = "Vendor";
const char REJECTED_VENDOR_NAMES_VENDOR_EN[] = "Vendor-EN";
const char REJECTED_VENDOR_NAMES_NOKIA[] = "Nokia";
const char REJECTED_VENDOR_NAMES_EMPTY[] = "";

const char CERTIFIED_SIGNED_CAPABILITY_NETWORK_CONTROL[] = "NetworkControl";
const char CERTIFIED_SIGNED_CAPABILITY_MULTIMEDIA_DD[] = "MultimediaDD";
const char CERTIFIED_SIGNED_CAPABILITY_COMM_DD[] = "CommDD";
const char CERTIFIED_SIGNED_CAPABILITY_DISK_ADMIN[] = "DiskAdmin";
const char MANUFACTURER_APPROVED_CAPABILITY_ALL_FILES[] = "AllFiles";
const char MANUFACTURER_APPROVED_CAPABILITY_DRM[] = "DRM";
const char MANUFACTURER_APPROVED_CAPABILITY_TCB[] = "TCB";
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

    QString displayName() const;
    QString globalVendorName() const;
    QString localisedVendorNames() const;
    bool isVendorNameValid(const QString &vendorName) const;

    QString nameFromTarget() const;
    QString qtVersion() const;

    QString uid3() const;
    bool isUID3Valid(const QString &uid3) const;
    bool isTestUID3(const QString &uid3) const;
    bool isKnownSymbianSignedUID3(const QString &uid3) const;

    QString capabilities() const;
    bool isCapabilityOneOf(const QString &capability, CapabilityLevel level) const;

    void updateProFile();
    void updateProFile(const QString &var, const QString &values);
    void buildSis();

    QString createdSisFileContainingFolder();
    QString createdSisFilePath();

    bool hasSucceeded();

    void setDisplayName(const QString &displayName);
    void setVendorName(const QString &vendorName);
    void setLocalVendorNames(const QString &localVendorNames);
    void setAppUid(const QString &appuid);

signals:
    void progressReport(const QString& status, QColor c);
    void succeeded();
    void finished();

public slots:
    void publishStepFinished(bool succeeded);
    void printMessage(QString message, bool error);

private:
    bool nextStep();
    bool runStep();

    bool sisExists(QString &sisFile);
    bool isDynamicLibrary(const Qt4Project &project) const;

private:
    QColor m_errorColor;
    QColor m_commandColor;
    QColor m_okColor;
    QColor m_normalColor;

    Qt4BuildConfiguration * m_qt4bc;
    const Qt4SymbianTarget * m_activeTargetOfProject;
    Qt4Project * m_qt4project;
    QtSupport::ProFileReader *m_reader;
    QStringList m_rejectedVendorNames;
    QStringList m_capabilitiesForCertifiedSigned;
    QStringList m_capabilitesForManufacturerApproved;
    QString m_vendorInfoVariable;
    QString m_vendorName;
    QString m_localVendorNames;
    QString m_appUid;
    QString m_displayName;

    QList<S60PublishStep *> m_publishSteps;

    bool m_finishedAndSuccessful;
};

class S60PublishStep : public QObject
{
    Q_OBJECT

public:
    explicit S60PublishStep(bool mandatory, QObject *parent = 0);
    virtual void start() = 0;

    virtual QString displayDescription() const = 0;
    bool succeeded() const;
    bool mandatory() const;

signals:
    void finished(bool succeeded);
    void output(QString output, bool error);

protected:
    void setSucceeded(bool succeeded);

private:
    bool m_succeeded;
    bool m_mandatory;
};

class S60CommandPublishStep : public S60PublishStep
{
    Q_OBJECT

public:
    explicit S60CommandPublishStep(const Qt4BuildConfiguration& bc,
                                   const QString &command,
                                   const QString &name,
                                   bool mandatory = true,
                                   QObject *parent = 0);

    virtual void start();
    virtual QString displayDescription() const;

private slots:
    void processFinished(int exitCode);

private:
    QProcess* m_proc;
    const QString m_command;
    const QString m_name;

};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60PUBLISHEROVI_H
