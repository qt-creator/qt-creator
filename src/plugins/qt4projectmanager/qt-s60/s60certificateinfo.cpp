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

#include "s60symbiancertificate.h"

S60CertificateInfo::CertificateState S60CertificateInfo::validateCertificate(const QString &certFilePath, QString *errorString)
{
    CertificateState result = CertificateValid;
    S60SymbianCertificate *certificate = new S60SymbianCertificate(certFilePath);
    if (certificate->isValid()) {
        QDateTime currentTime(QDateTime::currentDateTimeUtc());
        QDateTime endTime(certificate->endTime());
        QDateTime startTime(certificate->startTime());
        if (currentTime > endTime) {
            if (errorString)
                *errorString = QCoreApplication::translate(
                        "S60Utils::validateCertificate",
                        "The \"%1\" certificate has already expired and cannot be used.\nExpiration date: %2.")
                    .arg(QFileInfo(certFilePath).fileName())
                    .arg(endTime.toLocalTime().toString());
            result = CertificateError;
        } else if (currentTime < startTime) {
            if (errorString)
                *errorString = QCoreApplication::translate(
                        "S60Utils::validateCertificate",
                        "The \"%1\" certificate is not yet valid.\nValid from: %2.")
                    .arg(QFileInfo(certFilePath).fileName())
                    .arg(startTime.toLocalTime().toString());
            result = CertificateWarning; //This certificate may be valid in the near future
        }
    } else {
        if (errorString)
            *errorString = QCoreApplication::translate(
                    "S60Utils::validateCertificate",
                    "The \"%1\" certificate is not a valid X.509 certificate.")
                .arg(QFileInfo(certFilePath).baseName());
        result = CertificateError;
    }
    delete certificate;
    return result;
}
