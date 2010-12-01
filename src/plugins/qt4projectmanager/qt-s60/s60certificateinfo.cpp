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

#include "s60certificateinfo.h"

#include <QDateTime>
#include <QFileInfo>
#include <QCoreApplication>

#include <botan/x509cert.h>

S60CertificateInfo::CertificateState S60CertificateInfo::validateCertificate(const QString &certFilePath, QString *errorString)
{
    bool certFileCorrupted = false;
    CertificateState result = CertificateValid;
    Botan::X509_Certificate *certificate = 0;
    try {
        certificate = new Botan::X509_Certificate(certFilePath.toStdString());
        if (certificate) {
            const char * const CERTIFICATE_DATE_FORMAT = "yyyy/M/d h:mm:ss UTC";

            QDateTime startTime = QDateTime::fromString(QString::fromStdString(certificate->start_time()),
                                                        QLatin1String(CERTIFICATE_DATE_FORMAT));
            QDateTime startTimeUTC(startTime.date(), startTime.time(), Qt::UTC);

            QDateTime endTime = QDateTime::fromString(QString::fromStdString(certificate->end_time()),
                                                      QLatin1String(CERTIFICATE_DATE_FORMAT));
            QDateTime endTimeUTC(endTime.date(), endTime.time(), Qt::UTC);

            QDateTime currentTime(QDateTime::currentDateTimeUtc());
            if (currentTime > endTimeUTC) {
                if (errorString)
                    *errorString = QCoreApplication::translate(
                            "S60Utils::validateCertificate",
                            "The \"%1\" certificate has already expired and cannot be used.\nExpiration date: %2.")
                        .arg(QFileInfo(certFilePath).fileName())
                        .arg(endTimeUTC.toLocalTime().toString());
                result = CertificateError;
            } else if (currentTime < startTimeUTC) {
                if (errorString)
                    *errorString = QCoreApplication::translate(
                            "S60Utils::validateCertificate",
                            "The \"%1\" certificate is not yet valid.\nValid from: %2.")
                        .arg(QFileInfo(certFilePath).fileName())
                        .arg(startTimeUTC.toLocalTime().toString());
                result = CertificateWarning; //This certificate may be valid in the near future
            }
        } else
            certFileCorrupted = true;
    } catch (Botan::Exception &e) {
        Q_UNUSED(e)
        certFileCorrupted = true;
    }
    delete certificate;
    if (certFileCorrupted) {
        if (errorString)
            *errorString = QCoreApplication::translate(
                    "S60Utils::validateCertificate",
                    "The \"%1\" certificate is not a valid X.509 certificate.")
                .arg(QFileInfo(certFilePath).baseName());
        result = CertificateError;
    }
    return result;
}
