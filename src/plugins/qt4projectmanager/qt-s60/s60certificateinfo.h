/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef S60CERTIFICATEINFO_H
#define S60CERTIFICATEINFO_H

#include <QtCore/QObject>
#include <QtCore/QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(S60SymbianCertificate)

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

    explicit S60CertificateInfo(const QString &filePath, QObject* parent = 0);
    ~S60CertificateInfo();

    CertificateState validateCertificate();
    quint32 capabilitiesSupported();
    QString toHtml();
    QString errorString() const;

private:
    S60SymbianCertificate *m_certificate;
    QString m_filePath;
    QString m_errorString;
};

#endif // S60CERTIFICATEINFO_H
