/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "s60certificateinfo.h"
#include "s60manager.h"

#include "qt4buildconfiguration.h"
#include "qmakestep.h"
#include "makestep.h"
#include "qt4project.h"
#include "qt4nodes.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/profilereader.h>

#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <proparser/prowriter.h>

#include <QProcess>

namespace Qt4ProjectManager {
namespace Internal {

S60PublisherOvi::S60PublisherOvi(QObject *parent) :
    QObject(parent),
    m_reader(0),
    m_finishedAndSuccessful(false)
{
    // build m_rejectedVendorNames
    m_rejectedVendorNames.append(QLatin1String(Constants::REJECTED_VENDOR_NAMES_NOKIA));
    m_rejectedVendorNames.append(QLatin1String(Constants::REJECTED_VENDOR_NAMES_VENDOR));
    m_rejectedVendorNames.append(QLatin1String(Constants::REJECTED_VENDOR_NAMES_VENDOR_EN));
    m_rejectedVendorNames.append(QLatin1String(Constants::REJECTED_VENDOR_NAMES_EMPTY));

    // build m_capabilitiesForCertifiedSigned
    m_capabilitiesForCertifiedSigned.append(QLatin1String(Constants::CERTIFIED_SIGNED_CAPABILITY_COMM_DD));
    m_capabilitiesForCertifiedSigned.append(QLatin1String(Constants::CERTIFIED_SIGNED_CAPABILITY_DISK_ADMIN));
    m_capabilitiesForCertifiedSigned.append(QLatin1String(Constants::CERTIFIED_SIGNED_CAPABILITY_MULTIMEDIA_DD));
    m_capabilitiesForCertifiedSigned.append(QLatin1String(Constants::CERTIFIED_SIGNED_CAPABILITY_NETWORK_CONTROL));

    // build m_capabilitesForManufacturerApproved
    m_capabilitesForManufacturerApproved.append(QLatin1String(Constants::MANUFACTURER_APPROVED_CAPABILITY_ALL_FILES));
    m_capabilitesForManufacturerApproved.append(QLatin1String(Constants::MANUFACTURER_APPROVED_CAPABILITY_DRM));
    m_capabilitesForManufacturerApproved.append(QLatin1String(Constants::MANUFACTURER_APPROVED_CAPABILITY_TCB));

    // set up colours for progress reports
    m_errorColor = Qt::red;
    m_commandColor = Qt::blue;
    m_okColor = Qt::darkGreen;
    m_normalColor = Qt::black;
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

void S60PublisherOvi::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void S60PublisherOvi::setVendorName(const QString &vendorName)
{
    m_vendorName = vendorName;
}

void S60PublisherOvi::setLocalVendorNames(const QString &localVendorNames)
{
    QStringList vendorNames = localVendorNames.split(QLatin1Char(','));
    QStringList resultingList;
    foreach (QString vendorName, vendorNames) {
        resultingList.append(QLatin1String("\\\"") + vendorName.trimmed() + QLatin1String("\\\""));
    }
    m_localVendorNames = resultingList.join(QLatin1String(", "));
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
    m_publishSteps.clear();
}

void S60PublisherOvi::completeCreation()
{
#if 0 // FIXME: This needs serious reworking!
    // set active target
    m_activeTargetOfProject = qobject_cast<Qt4SymbianTarget *>(m_qt4bc->target());
    QTC_ASSERT(m_activeTargetOfProject, return);

    //set up project
    m_qt4project = m_activeTargetOfProject->qt4Project();

    // set up pro file reader
    m_reader = m_qt4project->createProFileReader(m_qt4project->rootQt4ProjectNode(), m_qt4bc);
    //m_reader->setCumulative(false); // todo need to reenable that, after fixing parsing for symbian scopes

    ProFile *profile = m_reader->parsedProFile(m_qt4project->rootProjectNode()->path());
    m_reader->accept(profile, ProFileEvaluator::LoadProOnly);
    profile->deref();

    // set up process for creating the resulting SIS files
    ProjectExplorer::AbstractProcessStep * makeStep = m_qt4bc->makeStep();
    makeStep->init();
    const ProjectExplorer::ProcessParameters * const makepp = makeStep->processParameters();

    ProjectExplorer::AbstractProcessStep *qmakeStep = m_qt4bc->qmakeStep();
    qmakeStep->init();
    const ProjectExplorer::ProcessParameters * const qmakepp = qmakeStep->processParameters();

    m_publishSteps.clear();
    const QChar space = QLatin1Char(' ');
    m_publishSteps.append(new S60CommandPublishStep(*m_qt4bc,
                                                    makepp->effectiveCommand() + QLatin1String(" clean -w"),
                                                    tr("Clean"),
                                                    false));

    m_publishSteps.append(new S60CommandPublishStep(*m_qt4bc,
                                                    qmakepp->effectiveCommand() + space + qmakepp->arguments(),
                                                    tr("qmake")));

    m_publishSteps.append(new S60CommandPublishStep(*m_qt4bc,
                                                    makepp->effectiveCommand() + space + makepp->arguments(),
                                                    tr("Build")));
    if (isDynamicLibrary(*m_qt4project)) {
        const QString freezeArg = QLatin1String("freeze-") + makepp->arguments();
        m_publishSteps.append(new S60CommandPublishStep(*m_qt4bc,
                                                        makepp->effectiveCommand() + space + freezeArg,
                                                        tr("Freeze")));

        m_publishSteps.append(new S60CommandPublishStep(*m_qt4bc,
                                                        makepp->effectiveCommand() + QLatin1String(" clean -w"),
                                                        tr("Secondary clean"),
                                                        false));

        m_publishSteps.append(new S60CommandPublishStep(*m_qt4bc,
                                                        qmakepp->effectiveCommand() + space + qmakepp->arguments(),
                                                        tr("Secondary qmake")));

        m_publishSteps.append(new S60CommandPublishStep(*m_qt4bc,
                                                        makepp->effectiveCommand() + space + makepp->arguments(),
                                                        tr("Secondary build")));
    }

    QString signArg = QLatin1String("unsigned_installer_sis");
    if (m_qt4bc->qtVersion()->qtVersion() == QtSupport::QtVersionNumber(4,6,3) )
        signArg = QLatin1String("installer_sis");
    m_publishSteps.append(new S60CommandPublishStep(*m_qt4bc,
                                                    makepp->effectiveCommand() + space + signArg,
                                                    tr("Making SIS file")));

    // set up access to vendor names
    QStringList deploymentLevelVars = m_reader->values(QLatin1String("DEPLOYMENT"));
    QStringList vendorInfoVars;
    QStringList valueLevelVars;

    foreach (const QString &deploymentLevelVar, deploymentLevelVars) {
        vendorInfoVars = m_reader->values(deploymentLevelVar+QLatin1String(".pkg_prerules"));
        foreach (const QString &vendorInfoVar, vendorInfoVars) {
            valueLevelVars = m_reader->values(vendorInfoVar);
            foreach (const QString &valueLevelVar, valueLevelVars) {
                if (valueLevelVar.startsWith(QLatin1String("%{\""))) {
                    m_vendorInfoVariable = vendorInfoVar;
                    break;
                }
            }
        }
    }
#endif
}

bool S60PublisherOvi::isDynamicLibrary(const Qt4Project &project) const
{
    Qt4ProFileNode *proFile = project.rootQt4ProjectNode();
    if (proFile->projectType() == LibraryTemplate) {
        const QStringList &config(proFile->variableValue(ConfigVar));
        if (!config.contains(QLatin1String("static")) && !config.contains(QLatin1String("staticlib")))
            return true;
    }
    return false;
}

QString S60PublisherOvi::nameFromTarget() const
{
    QString target = m_reader->value(QLatin1String("TARGET"));
    if (target.isEmpty())
        target = QFileInfo(m_qt4project->rootProjectNode()->path()).baseName();
    return target;
}

QString S60PublisherOvi::displayName() const
{
    const QStringList displayNameList = m_reader->values(QLatin1String("DEPLOYMENT.display_name"));

    if (displayNameList.isEmpty())
        return nameFromTarget();

    return displayNameList.join(QLatin1String(" "));
}

QString S60PublisherOvi::globalVendorName() const
{
    QStringList vendorinfos = m_reader->values(m_vendorInfoVariable);

    foreach (QString vendorinfo, vendorinfos) {
        if (vendorinfo.startsWith(QLatin1Char(':'))) {
            return vendorinfo.remove(QLatin1Char(':')).remove(QLatin1Char('"')).trimmed();
        }
    }
    return QString();
}

QString S60PublisherOvi::localisedVendorNames() const
{
    QStringList vendorinfos = m_reader->values(m_vendorInfoVariable);
    QString result;

    QStringList localisedVendorNames;
    foreach (QString vendorinfo, vendorinfos) {
        if (vendorinfo.startsWith(QLatin1Char('%'))) {
            localisedVendorNames = vendorinfo.remove(QLatin1String("%{")).remove(QLatin1Char('}')).split(QLatin1Char(','));
            foreach (QString localisedVendorName, localisedVendorNames) {
                if (!result.isEmpty())
                    result.append(QLatin1String(", "));
                result.append(localisedVendorName.remove(QLatin1Char('"')).trimmed());
            }
            return result;
        }
    }
    return QString();
}

bool S60PublisherOvi::isVendorNameValid(const QString &vendorName) const
{
    // vendorName cannot containg "Nokia"
    if (vendorName.trimmed().contains(QLatin1String(Constants::REJECTED_VENDOR_NAMES_NOKIA), Qt::CaseInsensitive))
        return false;

    // vendorName cannot be any of the rejected vendor names
    foreach (const QString &rejectedVendorName, m_rejectedVendorNames)
        if (vendorName.trimmed().compare(rejectedVendorName, Qt::CaseInsensitive) == 0)
            return false;

    return true;
}

QString S60PublisherOvi::qtVersion() const
{
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(m_qt4bc->target()->profile());
    return version ? version->displayName() : QString();
}

QString S60PublisherOvi::uid3() const
{
    return m_reader->value(QLatin1String("TARGET.UID3"));
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
    return ok && (hex >= TestStart && hex <= TestEnd);
}

bool S60PublisherOvi::isKnownSymbianSignedUID3(const QString &uid3) const
{
    bool ok;
    ulong hex = uid3.trimmed().toULong(&ok, 0);
    return ok && (hex >= SymbianSignedUnprotectedStart && hex <= SymbianSignedUnprotectedEnd);
}

QString S60PublisherOvi::capabilities() const
{
    return m_reader->values(QLatin1String("TARGET.CAPABILITY")).join(QLatin1String(", "));
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

    Utils::FileReader reader;
    if (!reader.fetch(m_qt4project->rootProjectNode()->path(), QIODevice::Text)) {
        emit progressReport(reader.errorString(), m_errorColor);
        return;
    }
    lines = QString::fromLocal8Bit(reader.data()).split(QLatin1Char('\n'));

    ProWriter::putVarValues(profile, &lines, QStringList() << values, var,
                            ProWriter::ReplaceValues | ProWriter::OneLine | ProWriter::AppendOperator,
                            QLatin1String("symbian"));

    Utils::FileSaver saver(m_qt4project->rootProjectNode()->path(), QIODevice::Text);
    saver.write(lines.join(QLatin1String("\n")).toLocal8Bit());
    if (!saver.finalize())
        emit progressReport(saver.errorString(), m_errorColor);
}

void S60PublisherOvi::updateProFile()
{
    if (m_vendorInfoVariable.isEmpty()) {
        m_vendorInfoVariable = QLatin1String("vendorinfo");
        updateProFile(QLatin1String("my_deployment.pkg_prerules"), m_vendorInfoVariable);
        updateProFile(QLatin1String("DEPLOYMENT"), QLatin1String("my_deployment"));
    }

    if (!m_displayName.isEmpty() && m_displayName != nameFromTarget())
        updateProFile(QLatin1String("DEPLOYMENT.display_name"), m_displayName);

    updateProFile(m_vendorInfoVariable, QLatin1String("\"%{")
                  + m_localVendorNames
                  + QLatin1String("}\" \":\\\"")
                  + m_vendorName
                  + QLatin1String("\\\"\"") );
    updateProFile(QLatin1String("TARGET.UID3"), m_appUid);
}

void S60PublisherOvi::buildSis()
{
    updateProFile();
    if (!runStep()) {
        emit progressReport(tr("Done.\n"), m_commandColor);
        emit finished();
    }
}

bool S60PublisherOvi::runStep()
{
    QTC_ASSERT(m_publishSteps.count(), return false);

    S60PublishStep *step = m_publishSteps.at(0);
    emit progressReport(step->displayDescription() + QLatin1Char('\n'), m_commandColor);
    connect(step, SIGNAL(finished(bool)), this, SLOT(publishStepFinished(bool)));
    connect(step, SIGNAL(output(QString,bool)), this, SLOT(printMessage(QString,bool)));
    step->start();
    return true;
}

bool S60PublisherOvi::nextStep()
{
    QTC_ASSERT(m_publishSteps.count(), return false);
    m_publishSteps.removeAt(0);
    return m_publishSteps.count();
}

void S60PublisherOvi::printMessage(QString message, bool error)
{
    emit progressReport(message + QLatin1Char('\n'), error ? m_errorColor : m_okColor);
}

void S60PublisherOvi::publishStepFinished(bool success)
{
    if (!success && m_publishSteps.at(0)->mandatory()) {
        emit progressReport(tr("SIS file not created due to previous errors.\n") , m_errorColor);
        emit finished();
        return;
    }

    if (nextStep())
        runStep();
    else {
        QString sisFile;
        if (sisExists(sisFile)) {
            emit progressReport(tr("Created %1.\n").arg(QDir::toNativeSeparators(sisFile)), m_normalColor);
            m_finishedAndSuccessful = true;
            emit succeeded();
        }
        emit progressReport(tr("Done.\n"), m_commandColor);
        emit finished();
    }
}

bool S60PublisherOvi::sisExists(QString &sisFile)
{
    QString fileNamePostFix = QLatin1String("_installer_unsigned.sis");
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(m_qt4bc->target()->profile());
    if (version && version->qtVersion() == QtSupport::QtVersionNumber(4,6,3) )
        fileNamePostFix = QLatin1String("_installer.sis");

    sisFile = m_qt4bc->buildDirectory() + QLatin1Char('/') + m_qt4project->displayName() + fileNamePostFix;

    QFileInfo fi(sisFile);
    return fi.exists();
}

QString S60PublisherOvi::createdSisFileContainingFolder()
{
    QString fileNamePostFix = QLatin1String("_installer_unsigned.sis");
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(m_qt4bc->target()->profile());
    if (version && version->qtVersion() == QtSupport::QtVersionNumber(4,6,3) )
        fileNamePostFix = QLatin1String("_installer.sis");

    QString resultFile = m_qt4bc->buildDirectory() + QLatin1Char('/') + m_qt4project->displayName() + fileNamePostFix;
    QFileInfo fi(resultFile);

    return fi.exists() ? QDir::toNativeSeparators(m_qt4bc->buildDirectory()) : QString();
}

QString S60PublisherOvi::createdSisFilePath()
{
    QString fileNamePostFix = QLatin1String("_installer_unsigned.sis");
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(m_qt4bc->target()->profile());
    if (version && version->qtVersion() == QtSupport::QtVersionNumber(4,6,3) )
        fileNamePostFix = QLatin1String("_installer.sis");

    const QString resultFile = m_qt4bc->buildDirectory() + QLatin1Char('/')
                               + m_qt4project->displayName() + fileNamePostFix;
    QFileInfo fi(resultFile);
    return fi.exists() ? QDir::toNativeSeparators(resultFile) : QString();
}

bool S60PublisherOvi::hasSucceeded()
{
    return m_finishedAndSuccessful;
}

// ======== S60PublishStep

S60PublishStep::S60PublishStep(bool mandatory, QObject *parent)
    : QObject(parent),
      m_succeeded(false),
      m_mandatory(mandatory)
{
}

bool S60PublishStep::succeeded() const
{
    return m_succeeded;
}

bool S60PublishStep::mandatory() const
{
    return m_mandatory;
}

void S60PublishStep::setSucceeded(bool succeeded)
{
    m_succeeded = succeeded;
}

// ======== S60CommandPublishStep

S60CommandPublishStep::S60CommandPublishStep(const Qt4ProjectManager::Qt4BuildConfiguration &bc,
                                             const QString &command,
                                             const QString &name,
                                             bool mandatory,
                                             QObject *parent)
    : S60PublishStep(mandatory, parent),
      m_proc(new QProcess(this)),
      m_command(command),
      m_name(name)
{
    m_proc->setEnvironment(bc.environment().toStringList());
    m_proc->setWorkingDirectory(bc.buildDirectory());

    connect(m_proc, SIGNAL(finished(int)), SLOT(processFinished(int)));
}

void S60CommandPublishStep::processFinished(int exitCode)
{
    QByteArray outputText = m_proc->readAllStandardOutput();
    if (!outputText.isEmpty())
        emit output(QString::fromLocal8Bit(outputText), false);

    outputText = m_proc->readAllStandardError();
    if (!outputText.isEmpty())
        emit output(QString::fromLocal8Bit(outputText), true);

    setSucceeded(exitCode == QProcess::NormalExit);
    emit finished(succeeded());
}

void S60CommandPublishStep::start()
{
    emit output(m_command, false);
    m_proc->start(m_command);
}

QString S60CommandPublishStep::displayDescription() const
{
    //: %1 is a name of the Publish Step i.e. Clean Step
    return tr("Running %1").arg(m_name);
}

} // namespace Internal
} // namespace Qt4ProjectManager
