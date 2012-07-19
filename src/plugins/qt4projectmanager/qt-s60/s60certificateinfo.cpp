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

#include "s60certificateinfo.h"
#include "s60symbiancertificate.h"

#include <QDateTime>
#include <QFileInfo>
#include <QCoreApplication>
#include <QTextStream>
#include <QHash>
#include <QMutableHashIterator>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
    const char * const SIMPLE_DATE_FORMAT = "dd.MM.yyyy";
}

struct Capability {
    const char *name;
    const int value;
};

static const Capability capability[] =
{
    { "LocalServices", S60CertificateInfo::LocalServices },
    { "Location", S60CertificateInfo::Location },
    { "NetworkServices", S60CertificateInfo::NetworkServices },
    { "ReadUserData", S60CertificateInfo::ReadUserData },
    { "UserEnvironment", S60CertificateInfo::UserEnvironment },
    { "WriteUserData", S60CertificateInfo::WriteUserData },
    { "PowerMgmt", S60CertificateInfo::PowerMgmt },
    { "ProtServ", S60CertificateInfo::ProtServ },
    { "ReadDeviceData", S60CertificateInfo::ReadDeviceData },
    { "SurroundingsDD", S60CertificateInfo::SurroundingsDD },
    { "SwEvent", S60CertificateInfo::SwEvent },
    { "TrustedUI", S60CertificateInfo::TrustedUI },
    { "WriteDeviceData", S60CertificateInfo::WriteDeviceData },
    { "CommDD", S60CertificateInfo::CommDD },
    { "DiskAdmin", S60CertificateInfo::DiskAdmin },
    { "NetworkControl", S60CertificateInfo::NetworkControl },
    { "MultimediaDD", S60CertificateInfo::MultimediaDD },
    { "AllFiles", S60CertificateInfo::AllFiles },
    { "DRM", S60CertificateInfo::DRM },
    { "TCB", S60CertificateInfo::TCB }
};

struct CapabilitySet {
    const char *color;
    const int value;
};

static const CapabilitySet capabilitySet[] =
{
    { "green", S60CertificateInfo::UserCapabilities },
    { "darkorange", S60CertificateInfo::SystemCapabilities },
    { "orangered", S60CertificateInfo::RestrictedCapabilities },
    { "red", S60CertificateInfo::ManufacturerCapabilities }
};

QHash<int, QStringList> createCapabilityMap(uint capabilities)
{
    const int capabilityCount = sizeof(capability)/sizeof(capability[0]);
    const int capabilitySetCount = sizeof(capabilitySet)/sizeof(capabilitySet[0]);

    QHash<int, QStringList> capabilityMap; //to separate the groups of capabilities
    for(int i = 0; i < capabilityCount; ++i)
        if (capabilities&capability[i].value) {
            for (int j = 0; j < capabilitySetCount; ++j)
                if (capability[i].value&capabilitySet[j].value) {
                    capabilityMap[capabilitySet[j].value] << QLatin1String(capability[i].name);
                    break;
                }
        }

    QMutableHashIterator<int, QStringList> i(capabilityMap);
    while (i.hasNext()) {
        i.next();
        i.value().sort();
    }

    return capabilityMap;
}

QStringList createCapabilityList(uint capabilities)
{
    QHash<int, QStringList> capabilityMap(createCapabilityMap(capabilities));

    return capabilityMap[S60CertificateInfo::UserCapabilities]
            + capabilityMap[S60CertificateInfo::SystemCapabilities]
            + capabilityMap[S60CertificateInfo::RestrictedCapabilities]
            + capabilityMap[S60CertificateInfo::ManufacturerCapabilities];
}

QStringList createHtmlCapabilityList(uint capabilities)
{
    const int capabilitySetCount = sizeof(capabilitySet)/sizeof(capabilitySet[0]);
    QHash<int, QStringList> capabilityMap(createCapabilityMap(capabilities));
    QStringList result;

    for (int j = 0; j < capabilitySetCount; ++j) {
        QHashIterator<int, QStringList> i(capabilityMap);
        while (i.hasNext()) {
            i.next();
            if (i.key() == capabilitySet[j].value) {
                foreach (const QString &capability, i.value()) {
                    result << QString::fromAscii("<font color=\"%1\">%2</font>")
                              .arg(QLatin1String(capabilitySet[j].color)).arg(capability);
                }
                break;
            }
        }
    }
    return result;
}

S60CertificateInfo::S60CertificateInfo(const QString &filePath, QObject* parent)
    : QObject(parent),
      m_certificate(new S60SymbianCertificate(filePath)),
      m_filePath(filePath),
      m_capabilities(NoInformation)
{
    if (!m_certificate->isValid())
        return;

    m_imeiList = m_certificate->subjectInfo(QLatin1String("1.2.826.0.1.1796587.1.1.1.1"));

    const QStringList capabilityList(m_certificate->subjectInfo(QLatin1String("1.2.826.0.1.1796587.1.1.1.6")));
    if (capabilityList.isEmpty())
        m_capabilities = 0;
    else
        m_capabilities = capabilityList.at(0).toLong();
}

S60CertificateInfo::~S60CertificateInfo()
{
    delete m_certificate;
}

S60CertificateInfo::CertificateState S60CertificateInfo::validateCertificate()
{
    CertificateState result = CertificateValid;
    if (m_certificate->isValid()) {
        QDateTime currentTime(QDateTime::currentDateTimeUtc());
        QDateTime endTime(m_certificate->endTime());
        QDateTime startTime(m_certificate->startTime());
        if (currentTime > endTime) {
            m_errorString = tr("The certificate \"%1\" has already expired and cannot be used."
                               "\nExpiration date: %2.")
                    .arg(QFileInfo(m_filePath).fileName())
                    .arg(endTime.toLocalTime().toString(QLatin1String(SIMPLE_DATE_FORMAT)));
            result = CertificateError;
        } else if (currentTime < startTime) {
            m_errorString = tr("The certificate \"%1\" is not yet valid.\nValid from: %2.")
                    .arg(QFileInfo(m_filePath).fileName())
                    .arg(startTime.toLocalTime().toString(QLatin1String(SIMPLE_DATE_FORMAT)));
            result = CertificateWarning; //This certificate may be valid in the near future
        }
    } else {
        m_errorString = tr("The certificate \"%1\" is not a valid X.509 certificate.")
                .arg(QFileInfo(m_filePath).baseName());
        result = CertificateError;
    }
    return result;
}

bool S60CertificateInfo::compareCapabilities(const QStringList &givenCaps, QStringList &unsupportedCaps) const
{
    if (!m_certificate->isValid())
        return false;
    unsupportedCaps.clear();
    if (capabilitiesSupported() == NoInformation)
        return true;

    QStringList capabilities(createCapabilityList(capabilitiesSupported()));
    foreach (const QString &capability, givenCaps) {
        if (!capabilities.contains(capability, Qt::CaseInsensitive))
            unsupportedCaps << capability;
    }
    return true;
}

QString S60CertificateInfo::errorString() const
{
    return m_errorString.isEmpty()?m_certificate->errorString():m_errorString;
}

QStringList S60CertificateInfo::devicesSupported() const
{
    return m_imeiList;
}

quint32 S60CertificateInfo::capabilitiesSupported() const
{
    return m_capabilities;
}

bool S60CertificateInfo::isDeveloperCertificate() const
{
    return !devicesSupported().isEmpty() || capabilitiesSupported();
}

QString S60CertificateInfo::toHtml(bool keepShort)
{
    if (!m_certificate->isValid())
        return errorString();

    QString htmlString;
    QTextStream str(&htmlString);
    str << "<html><body><table>"
        << "<tr><td><b>" << tr("Type: ") << "</b></td>";

    if (isDeveloperCertificate())
        str << "<td>" << tr("Developer certificate") << "</td>";
    if (m_certificate->isSelfSigned())
        str << "<td>" << tr("Self signed certificate") << "</td>";
    str << "</tr>";

    QString issuer;
    QStringList issuerOrganizationList(m_certificate->issuerInfo(QLatin1String("X520.Organization")));
    if (!issuerOrganizationList.isEmpty())
        issuer = issuerOrganizationList.join(QLatin1String(" "));

    QString subject;
    QStringList subjectOrganizationList(m_certificate->subjectInfo(QLatin1String("X520.Organization")));
    if (!subjectOrganizationList.isEmpty())
        subject = subjectOrganizationList.join(QLatin1String(" "));

    QDateTime startDate(m_certificate->startTime().toLocalTime());
    QDateTime endDate(m_certificate->endTime().toLocalTime());

    str << "<tr><td><b>" << tr("Issued by: ")
        << "</b></td><td>" << issuer << "</td></tr>"
        << "<tr><td><b>" << tr("Issued to: ")
        << "</b></td><td>" << subject << "</td></tr>"
        << "<tr><td><b>" << tr("Valid from: ")
        << "</b></td><td>" << startDate.toString(QLatin1String(SIMPLE_DATE_FORMAT)) << "</td></tr>"
        << "<tr><td><b>" << tr("Valid to: ")
        << "</b></td><td>" << endDate.toString(QLatin1String(SIMPLE_DATE_FORMAT)) << "</td></tr>";

    if (capabilitiesSupported()) {
        QStringList capabilities;
        if (keepShort)
            capabilities = createCapabilityList(capabilitiesSupported());
        else
            capabilities = createHtmlCapabilityList(capabilitiesSupported());
        str << "<tr><td><b>" << tr("Capabilities: ")
            << "</b></td><td><i>" << capabilities.join(QLatin1String(" ")) << "</i></td></tr>";
    }

    const QStringList &imeiList(devicesSupported());
    if (!imeiList.isEmpty()) {
        QString imeiListString;
        const QString space(QLatin1Char(' '));
        int MAX_DISPLAYED_IMEI_COUNT = 30;
        if (imeiList.count() > MAX_DISPLAYED_IMEI_COUNT && keepShort) {//1000 items would be too much :)
            for (int i = 0; i < MAX_DISPLAYED_IMEI_COUNT; ++i)
                imeiListString += imeiList.at(i) + space;
            imeiListString.replace(imeiListString.length()-1, 1, QLatin1String("..."));
        } else
            imeiListString = imeiList.join(space);
        str << "<tr><td><b>" << tr("Supporting %n device(s): ", "", imeiList.count())
            << "</b></td><td>" << imeiListString << "</td></tr>";
    }
    return htmlString;
}
