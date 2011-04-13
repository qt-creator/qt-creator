/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "s60publisherovi.h"

#include "qt4symbiantarget.h"
#include "s60certificateinfo.h"
#include "s60manager.h"

#include "qt4buildconfiguration.h"
#include "qmakestep.h"
#include "makestep.h"
#include "qt4project.h"
#include "qtversionmanager.h"

#include "profilereader.h"
#include "prowriter.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildstep.h>

#include <utils/qtcassert.h>

#include <QtCore/QProcess>

namespace Qt4ProjectManager {
namespace Internal {

S60PublisherOvi::S60PublisherOvi(QObject *parent) :
    QObject(parent),
    m_reader(0)
{
    // build m_rejectedVendorNames
    m_rejectedVendorNames.append(Constants::REJECTED_VENDOR_NAMES_NOKIA);
    m_rejectedVendorNames.append(Constants::REJECTED_VENDOR_NAMES_VENDOR);
    m_rejectedVendorNames.append(Constants::REJECTED_VENDOR_NAMES_VENDOR_EN);
    m_rejectedVendorNames.append(Constants::REJECTED_VENDOR_NAMES_EMPTY);

    // build m_capabilitiesForCertifiedSigned
    m_capabilitiesForCertifiedSigned.append(Constants::CERTIFIED_SIGNED_CAPABILITY_COMM_DD);
    m_capabilitiesForCertifiedSigned.append(Constants::CERTIFIED_SIGNED_CAPABILITY_DISK_ADMIN);
    m_capabilitiesForCertifiedSigned.append(Constants::CERTIFIED_SIGNED_CAPABILITY_MULTIMEDIA_DD);
    m_capabilitiesForCertifiedSigned.append(Constants::CERTIFIED_SIGNED_CAPABILITY_NETWORK_CONTROL);

    // build m_capabilitesForManufacturerApproved
    m_capabilitesForManufacturerApproved.append(Constants::MANUFACTURER_APPROVED_CAPABILITY_ALL_FILES);
    m_capabilitesForManufacturerApproved.append(Constants::MANUFACTURER_APPROVED_CAPABILITY_DRM);
    m_capabilitesForManufacturerApproved.append(Constants::MANUFACTURER_APPROVED_CAPABILITY_TCB);

    // set up colours for progress reports
    m_errorColor = Qt::red;
    m_commandColor = Qt::blue;
    m_okColor = Qt::darkGreen;
    m_normalColor = Qt::black;

    m_finishedAndSuccessful = false;

    m_qmakeProc = new QProcess(this);
    m_buildProc = new QProcess(this);
    m_createSisProc = new QProcess(this);
    connect(m_qmakeProc,SIGNAL(finished(int)), SLOT(runBuild(int)));
    connect(m_buildProc,SIGNAL(finished(int)), SLOT(runCreateSis(int)));
    connect(m_createSisProc,SIGNAL(finished(int)), SLOT(endBuild(int)));
}

S60PublisherOvi::~S60PublisherOvi()
{
    cleanUp();
}

void S60PublisherOvi::setBuildConfiguration(Qt4BuildConfiguration *qt4bc)
{
    // set build configuration
    m_qt4bc = qt4bc;
}

void S60PublisherOvi::setVendorName(const QString &vendorName)
{
    m_vendorName = vendorName;
}

void S60PublisherOvi::setLocalVendorNames(const QString &localVendorNames)
{
    m_localVendorNames = localVendorNames;
}

void S60PublisherOvi::setAppUid(const QString &appuid)
{
    m_appUid = appuid;
}

void S60PublisherOvi::cleanUp()
{
    if (m_qt4project && m_reader) {
        m_qt4project->destroyProFileReader(m_reader);
        m_reader = 0;
    }
}

void S60PublisherOvi::completeCreation()
{
    // set active target
    m_activeTargetOfProject = qobject_cast<Qt4SymbianTarget *>(m_qt4bc->target());
    QTC_ASSERT(m_activeTargetOfProject, return);

    //set up project
    m_qt4project = m_activeTargetOfProject->qt4Project();

    // set up pro file reader
    m_reader = m_qt4project->createProFileReader(m_qt4project->rootProjectNode(), m_qt4bc);
    //m_reader->setCumulative(false); // todo need to reenable that, after fixing parsing for symbian scopes

    ProFile *profile = m_reader->parsedProFile(m_qt4project->rootProjectNode()->path());
    m_reader->accept(profile, ProFileEvaluator::LoadProOnly);
    profile->deref();

    // set up process for creating the resulting sis files
    m_qmakeProc->setEnvironment(m_qt4bc->environment().toStringList());
    m_qmakeProc->setWorkingDirectory(m_qt4bc->buildDirectory());

    m_buildProc->setEnvironment(m_qt4bc->environment().toStringList());
    m_buildProc->setWorkingDirectory(m_qt4bc->buildDirectory());

    m_createSisProc->setEnvironment(m_qt4bc->environment().toStringList());
    m_createSisProc->setWorkingDirectory(m_qt4bc->buildDirectory());
}

QString S60PublisherOvi::globalVendorName() const
{
    QStringList vendorinfos = m_reader->values("vendorinfo");

    foreach (QString vendorinfo, vendorinfos) {
        if (vendorinfo.startsWith(':')) {
            return vendorinfo.remove(':').remove("\"").trimmed();
        }
    }
    return QString();
}

QString S60PublisherOvi::localisedVendorNames() const
{
    QStringList vendorinfos = m_reader->values("vendorinfo");
    QString result;

    QStringList localisedVendorNames;
    foreach (QString vendorinfo, vendorinfos) {
        if (vendorinfo.startsWith('%')) {
            localisedVendorNames = vendorinfo.remove("%{").remove('}').split(',');
            foreach (QString localisedVendorName, localisedVendorNames) {
                if (!result.isEmpty())
                    result.append(", ");
                result.append(localisedVendorName.remove("\"").trimmed());
            }
            return result;
        }
    }
    return QString();
}

bool S60PublisherOvi::isVendorNameValid(const QString &vendorName) const
{
    // vendorName cannot containg "Nokia"
    if (vendorName.trimmed().contains(Constants::REJECTED_VENDOR_NAMES_NOKIA, Qt::CaseInsensitive))
        return false;

    // vendorName cannot be any of the rejected vendor names
    foreach (const QString &rejectedVendorName, m_rejectedVendorNames)
        if (vendorName.trimmed().compare(rejectedVendorName, Qt::CaseInsensitive) == 0)
            return false;

    return true;
}

QString S60PublisherOvi::qtVersion() const
{
    return m_qt4bc->qtVersion()->displayName();
}

QString S60PublisherOvi::uid3() const
{
    return m_reader->value("TARGET.UID3");
}

bool S60PublisherOvi::isUID3Valid(const QString &uid3) const
{
    bool ok;
    ulong hex = uid3.trimmed().toULong(&ok, 0);

    return ok && (hex >= AssignedRestrictedStart && hex <= AssignedRestrictedEnd);
}

bool S60PublisherOvi::isTestUID3(const QString &uid3) const
{
    bool ok;
    ulong hex = uid3.trimmed().toULong(&ok, 0);
    return ok && (hex >= TestStart && hex <=TestEnd);
}

bool S60PublisherOvi::isKnownSymbianSignedUID3(const QString &uid3) const
{
    bool ok;
    ulong hex = uid3.trimmed().toULong(&ok, 0);
    return ok && (hex >= SymbianSignedUnprotectedStart && hex <= SymbianSignedUnprotectedEnd);
}

QString S60PublisherOvi::capabilities() const
{
    return m_reader->values("TARGET.CAPABILITY").join(", ");
}

bool S60PublisherOvi::isCapabilityOneOf(const QString &capability, CapabilityLevel level) const
{
    QStringList capabilitiesInLevel;
    if (level == CertifiedSigned)
        capabilitiesInLevel = m_capabilitiesForCertifiedSigned;
    else if (level == ManufacturerApproved)
        capabilitiesInLevel = m_capabilitesForManufacturerApproved;

    return capabilitiesInLevel.contains(capability.trimmed());
}

void S60PublisherOvi::updateProFile(const QString &var, const QString &values)
{
    QStringList lines;
    ProFile *profile = m_reader->parsedProFile(m_qt4project->rootProjectNode()->path());

    QFile qfile(m_qt4project->rootProjectNode()->path());
    if (qfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lines = QString::fromLocal8Bit(qfile.readAll()).split(QLatin1Char('\n'));
        qfile.close();
        while (!lines.isEmpty() && lines.last().isEmpty())
            lines.removeLast();
    } else {
        m_qt4project->proFileParseError(tr("Error while reading .pro file %1: %2").arg(m_qt4project->rootProjectNode()->path(), qfile.errorString()));
        return;
    }

    ProWriter::putVarValues(profile, &lines, QStringList() << values, var,
                            ProWriter::ReplaceValues | ProWriter::OneLine | ProWriter::AssignOperator,
                            "symbian");

    if (qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qfile.write(lines.join("\n").toLocal8Bit());
        qfile.close();
    }
}

void S60PublisherOvi::updateProFile()
{
    updateProFile("vendorinfo", "\"%{\\\"" + m_localVendorNames + "\\\"}\" \":\\\"" + m_vendorName + "\\\"\"" );
    updateProFile("TARGET.UID3", m_appUid);
}

void S60PublisherOvi::buildSis()
{
    updateProFile();
    runQMake();
}

void S60PublisherOvi::runQMake()
{
    m_finishedAndSuccessful = false;

    ProjectExplorer::AbstractProcessStep *qmakeStep = m_qt4bc->qmakeStep();
    qmakeStep->init();
    const ProjectExplorer::ProcessParameters * const qmakepp = qmakeStep->processParameters();
    runStep(QProcess::NormalExit,
            "Running QMake",
            qmakepp->effectiveCommand() + ' ' + qmakepp->arguments(),
            m_qmakeProc,
            0);
}

void S60PublisherOvi::runBuild(int result)
{
    ProjectExplorer::AbstractProcessStep * makeStep = m_qt4bc->makeStep();
    makeStep->init();
    const ProjectExplorer::ProcessParameters * const makepp = makeStep->processParameters();
    runStep(result,
            "Running Build Steps",
            makepp->effectiveCommand() + ' ' + makepp->arguments(),
            m_buildProc,
            m_qmakeProc);
}

void S60PublisherOvi::runCreateSis(int result)
{
    ProjectExplorer::AbstractProcessStep * makeStep = m_qt4bc->makeStep();
    makeStep->init();
    const ProjectExplorer::ProcessParameters * const makepp = makeStep->processParameters();
    QString makeTarget =  QLatin1String(" unsigned_installer_sis");

    if (m_qt4bc->qtVersion()->qtVersion() == QtVersionNumber(4,6,3) )
        makeTarget =  QLatin1String(" installer_sis");
    runStep(result,
            "Making Sis File",
            makepp->effectiveCommand() + makeTarget,
            m_createSisProc,
            m_buildProc);
}

void S60PublisherOvi::endBuild(int result)
{
    // show what happened in last step
    emit progressReport(QString(m_createSisProc->readAllStandardOutput() + '\n'), m_okColor);
    emit progressReport(QString(m_createSisProc->readAllStandardError() + '\n'), m_errorColor);

    QString fileNamePostFix =  QLatin1String("_installer_unsigned.sis");
    if (m_qt4bc->qtVersion()->qtVersion() == QtVersionNumber(4,6,3) )
        fileNamePostFix =  QLatin1String("_installer.sis");

    QString resultFile = m_qt4bc->buildDirectory() + "/" + m_qt4project->displayName() + fileNamePostFix;



    QFileInfo fi(resultFile);
    if (result == QProcess::NormalExit && fi.exists()) {
        emit progressReport(tr("Created %1\n").arg(QDir::toNativeSeparators(resultFile)), m_normalColor);
        m_finishedAndSuccessful = true;
        emit succeeded();
    } else {
        emit progressReport(tr(" Sis file not created due to previous errors\n"), m_errorColor);
    }
    emit progressReport(tr("Done!\n"), m_commandColor);
}

QString S60PublisherOvi::createdSisFileContainingFolder()
{
    QString fileNamePostFix =  QLatin1String("_installer_unsigned.sis");
    if (m_qt4bc->qtVersion()->qtVersion() == QtVersionNumber(4,6,3) )
        fileNamePostFix =  QLatin1String("_installer.sis");

    QString resultFile = m_qt4bc->buildDirectory() + "/" + m_qt4project->displayName() + fileNamePostFix;
    QFileInfo fi(resultFile);

    return fi.exists() ? QDir::toNativeSeparators(m_qt4bc->buildDirectory()) : QString();
}

QString S60PublisherOvi::createdSisFilePath()
{
    QString fileNamePostFix =  QLatin1String("_installer_unsigned.sis");
    if (m_qt4bc->qtVersion()->qtVersion() == QtVersionNumber(4,6,3) )
        fileNamePostFix =  QLatin1String("_installer.sis");

    QString resultFile = m_qt4bc->buildDirectory() + "/" + m_qt4project->displayName() + fileNamePostFix;
    QFileInfo fi(resultFile);

    return fi.exists() ? QDir::toNativeSeparators(m_qt4bc->buildDirectory()+ '/' + m_qt4project->displayName() + fileNamePostFix) : QString();
}

bool S60PublisherOvi::hasSucceeded()
{
    return m_finishedAndSuccessful;
}

void S60PublisherOvi::runStep(int result, const QString& buildStep, const QString& command, QProcess* currProc, QProcess* prevProc)
{
    // todo react to readyRead() instead of reading all at the end
    // show what happened in last step
    if (prevProc) {
        emit progressReport(QString(prevProc->readAllStandardOutput() + '\n'), m_okColor);
        emit progressReport(QString(prevProc->readAllStandardError() + '\n'), m_errorColor);
    }

    // if the last state finished ok then run the build.
    if (result == QProcess::NormalExit) {
         emit progressReport(buildStep + '\n', m_commandColor);
         emit progressReport(command + '\n', m_commandColor);

         currProc->start(command);
    } else {
        emit progressReport(tr("Sis file not created due to previous errors\n") , m_errorColor);
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
