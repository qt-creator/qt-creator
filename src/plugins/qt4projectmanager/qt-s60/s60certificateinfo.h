/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef S60CERTIFICATEINFO_H
#define S60CERTIFICATEINFO_H

#include <QObject>
#include <QStringList>
#include <QtGlobal>

namespace Qt4ProjectManager {
namespace Internal {

class S60SymbianCertificate;

class S60CertificateInfo : public QObject
{
    Q_OBJECT

public:
    enum CertificateState {
        CertificateValid,
        CertificateWarning,
        CertificateError
    };

    enum S60Capability {
        TCB                 = 1 << (31-0),
        CommDD              = 1 << (31-1),
        PowerMgmt           = 1 << (31-2),
        MultimediaDD        = 1 << (31-3),
        ReadDeviceData      = 1 << (31-4),
        WriteDeviceData     = 1 << (31-5),
        DRM                 = 1 << (31-6),
        TrustedUI           = 1 << (31-7),
        ProtServ            = 1 << (31-8),
        DiskAdmin           = 1 << (31-9),
        NetworkControl      = 1 << (31-10),
        AllFiles            = 1 << (31-11),
        SwEvent             = 1 << (31-12),
        NetworkServices     = 1 << (31-13),
        LocalServices       = 1 << (31-14),
        ReadUserData        = 1 << (31-15),
        WriteUserData       = 1 << (31-16),
        Location            = 1 << (31-17),
        SurroundingsDD      = 1 << (31-18),
        UserEnvironment     = 1 << (31-19),

        NoInformation       = 0
    };

    enum S60CapabilitySet {
        UserCapabilities       = LocalServices|Location|NetworkServices|ReadUserData
                                  |UserEnvironment|WriteUserData,
        SystemCapabilities     = PowerMgmt|ProtServ|ReadDeviceData|SurroundingsDD
                                  |SwEvent|TrustedUI|WriteDeviceData,
        RestrictedCapabilities = CommDD|DiskAdmin|NetworkControl|MultimediaDD,
        ManufacturerCapabilities = AllFiles|DRM|TCB
    };

    explicit S60CertificateInfo(const QString &filePath, QObject* parent = 0);
    ~S60CertificateInfo();

    CertificateState validateCertificate();
    QStringList devicesSupported() const;
    quint32 capabilitiesSupported() const;
    QString toHtml(bool keepShort = true);
    QString errorString() const;
    bool isDeveloperCertificate() const;

    bool compareCapabilities(const QStringList &givenCaps, QStringList &unsupportedCaps) const;

private:
    S60SymbianCertificate *m_certificate;
    QString m_filePath;
    QString m_errorString;
    QStringList m_imeiList;
    quint32 m_capabilities;
};

} // namespace Internal
} // namespace Qt4ProjectExplorer

#endif // S60CERTIFICATEINFO_H
