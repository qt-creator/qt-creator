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

#include "qt4projectmanagerconstants.h"
#include "symbianqtversion.h"
#include "qt-s60/sbsv2parser.h"
#include "qt-s60/abldparser.h"

#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/pathchooser.h>
#include <utils/environment.h>
#include <proparser/profileevaluator.h>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QLabel>
#include <QFormLayout>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

SymbianQtVersion::SymbianQtVersion()
    : BaseQtVersion()
{
}

SymbianQtVersion::SymbianQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource)
{

}

SymbianQtVersion::~SymbianQtVersion()
{

}

SymbianQtVersion *SymbianQtVersion::clone() const
{
    return new SymbianQtVersion(*this);
}

bool SymbianQtVersion::equals(BaseQtVersion *other)
{
    if (!BaseQtVersion::equals(other))
        return false;
    SymbianQtVersion *o = static_cast<SymbianQtVersion *>(other);
    return m_sbsV2Directory == o->m_sbsV2Directory;
}

QString SymbianQtVersion::type() const
{
    return QLatin1String(QtSupport::Constants::SYMBIANQT);
}

bool SymbianQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;
    if (isBuildWithSymbianSbsV2() && (m_sbsV2Directory.isEmpty() || !QFileInfo(m_sbsV2Directory + QLatin1String("/sbs")).exists()))
        return false;
    return true;
}

QString SymbianQtVersion::invalidReason() const
{
    QString tmp = BaseQtVersion::invalidReason();
    if (isBuildWithSymbianSbsV2()
            && (m_sbsV2Directory.isEmpty() || !QFileInfo(m_sbsV2Directory + QLatin1String("/sbs")).exists()))
        return QCoreApplication::translate("QtVersion", "SBS was not found.");

    return tmp;
}

void SymbianQtVersion::restoreLegacySettings(QSettings *s)
{
    setSbsV2Directory(QDir::fromNativeSeparators(s->value(QLatin1String("SBSv2Directory")).toString()));
}

void SymbianQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);
    setSbsV2Directory(QDir::fromNativeSeparators(map.value(QLatin1String("SBSv2Directory")).toString()));
}

QVariantMap SymbianQtVersion::toMap() const
{
    QVariantMap result = BaseQtVersion::toMap();
    result.insert(QLatin1String("SBSv2Directory"), sbsV2Directory());
    return result;
}

QList<ProjectExplorer::Abi> SymbianQtVersion::detectQtAbis() const
{
    return QList<ProjectExplorer::Abi>()
            << ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::SymbianOS,
                                    ProjectExplorer::Abi::UnknownFlavor,
                                    ProjectExplorer::Abi::ElfFormat,
                                    32);
}

QString SymbianQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Symbian", "Qt Version is meant for Symbian");
}


bool SymbianQtVersion::supportsShadowBuilds() const
{
    return false;
}

bool SymbianQtVersion::supportsBinaryDebuggingHelper() const
{
    return false;
}

static const char *S60_EPOC_HEADERS[] = {
    "include", "mkspecs/common/symbian", "epoc32/include",
    "epoc32/include/osextensions/stdapis", "epoc32/include/osextensions/stdapis/sys",
    "epoc32/include/stdapis", "epoc32/include/stdapis/sys",
    "epoc32/include/osextensions/stdapis/stlport", "epoc32/include/stdapis/stlport",
    "epoc32/include/oem", "epoc32/include/middleware", "epoc32/include/domain/middleware",
    "epoc32/include/osextensions", "epoc32/include/domain/osextensions",
    "epoc32/include/domain/osextensions/loc", "epoc32/include/domain/middleware/loc",
    "epoc32/include/domain/osextensions/loc/sc", "epoc32/include/domain/middleware/loc/sc",
    "epoc32/include/mw", "epoc32/include/app",
    "epoc32/include/platform", "epoc32/include/platform/mw",
    "epoc32/include/platform/app", "epoc32/include/platform/loc",
    "epoc32/include/platform/mw/loc", "epoc32/include/platform/app/loc",
    "epoc32/include/platform/loc/sc", "epoc32/include/platform/app/loc/sc",
    "epoc32/include/platform/mw/loc/sc"
};

void SymbianQtVersion::addToEnvironment(const ProjectExplorer::Profile *p, Utils::Environment &env) const
{
    BaseQtVersion::addToEnvironment(p, env);
    // Generic Symbian environment:
    QString epocRootPath = ProjectExplorer::SysRootProfileInformation::sysRoot(p).toString();
    QDir epocDir(epocRootPath);

    // Clean up epoc root path for the environment:
    if (!epocRootPath.endsWith(QLatin1Char('/')))
        epocRootPath.append(QLatin1Char('/'));
    if (!isBuildWithSymbianSbsV2()) {
#ifdef Q_OS_WIN
        if (epocRootPath.count() > 2
                && epocRootPath.at(0).toLower() >= QLatin1Char('a')
                && epocRootPath.at(0).toLower() <= QLatin1Char('z')
                && epocRootPath.at(1) == QLatin1Char(':')) {
            epocRootPath = epocRootPath.mid(2);
        }
#endif
    }
    env.set(QLatin1String("EPOCROOT"), QDir::toNativeSeparators(epocRootPath));

    env.prependOrSetPath(epocDir.filePath(QLatin1String("epoc32/tools"))); // e.g. make.exe
    // Windows only:
    if (ProjectExplorer::Abi::hostAbi().os() == ProjectExplorer::Abi::WindowsOS) {
        QString winDir = QLatin1String(qgetenv("WINDIR"));
        if (!winDir.isEmpty())
            env.prependOrSetPath(QDir(winDir).filePath(QLatin1String("system32")));

        if (epocDir.exists(QLatin1String("epoc32/gcc/bin")))
            env.prependOrSetPath(epocDir.filePath(QLatin1String("epoc32/gcc/bin"))); // e.g. cpp.exe, *NOT* gcc.exe
        // Find perl in the special Symbian flavour:
        if (epocDir.exists(QLatin1String("../../tools/perl/bin"))) {
            epocDir.cd(QLatin1String("../../tools/perl/bin"));
            env.prependOrSetPath(epocDir.absolutePath());
        } else {
            env.prependOrSetPath(epocDir.filePath(QLatin1String("perl/bin")));
        }
    }

    // SBSv2:
    if (isBuildWithSymbianSbsV2()) {
        QString sbsHome(env.value(QLatin1String("SBS_HOME")));
        QString sbsConfig = sbsV2Directory();
        if (!sbsConfig.isEmpty()) {
            env.prependOrSetPath(sbsConfig);
            // SBS_HOME is the path minus the trailing /bin:
            env.set(QLatin1String("SBS_HOME"),
                    QDir::toNativeSeparators(sbsConfig.left(sbsConfig.count() - 4))); // We need this for Qt 4.6.3 compatibility
        } else if (!sbsHome.isEmpty()) {
            env.prependOrSetPath(sbsHome + QLatin1String("/bin"));
        }
    }
}

QList<ProjectExplorer::HeaderPath> SymbianQtVersion::systemHeaderPathes(const ProjectExplorer::Profile *p) const
{
    QList<ProjectExplorer::HeaderPath> result;
    QString root = ProjectExplorer::SysRootProfileInformation::sysRoot(p).toString() + QLatin1Char('/');
    const int count = sizeof(S60_EPOC_HEADERS) / sizeof(const char *);
    for (int i = 0; i < count; ++i) {
        const QDir dir(root + QLatin1String(S60_EPOC_HEADERS[i]));
        if (dir.exists())
            result.append(ProjectExplorer::HeaderPath(dir.absolutePath(),
                                                      ProjectExplorer::HeaderPath::GlobalHeaderPath));
    }
    result.append(BaseQtVersion::systemHeaderPathes(p));
    return result;
}

ProjectExplorer::IOutputParser *SymbianQtVersion::createOutputParser() const
{
    if (isBuildWithSymbianSbsV2()) {
        return new SbsV2Parser;
    } else {
        ProjectExplorer::IOutputParser *parser = new AbldParser;
        parser->appendOutputParser(new ProjectExplorer::GnuMakeParser);
        return parser;
    }
}

QString SymbianQtVersion::sbsV2Directory() const
{
    return m_sbsV2Directory;
}

void SymbianQtVersion::setSbsV2Directory(const QString &directory)
{
    QDir dir(directory);
    const QString sbs = QLatin1String("sbs");
    if (dir.exists(sbs)) {
        m_sbsV2Directory = dir.absolutePath();
        return;
    }
    dir.cd(QLatin1String("bin"));
    if (dir.exists(sbs)) {
        m_sbsV2Directory = dir.absolutePath();
        return;
    }
    m_sbsV2Directory = directory;
}

bool SymbianQtVersion::isBuildWithSymbianSbsV2() const
{
    ensureMkSpecParsed();
    return m_isBuildUsingSbsV2;
}

void SymbianQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    QString makefileGenerator = evaluator->value(QLatin1String("MAKEFILE_GENERATOR"));
    m_isBuildUsingSbsV2 = (makefileGenerator == QLatin1String("SYMBIAN_SBSV2"));
    BaseQtVersion::parseMkSpec(evaluator);
}

Core::FeatureSet SymbianQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    if (qtVersion() >= QtSupport::QtVersionNumber(4, 7, 4)) //no reliable test for components, yet.
        features |= Core::FeatureSet(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_SYMBIAN);
    features |= Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE);

    return features;
}

QString SymbianQtVersion::platformName() const
{
    return QLatin1String(QtSupport::Constants::SYMBIAN_PLATFORM);
}

QString SymbianQtVersion::platformDisplayName() const
{
    return QLatin1String(QtSupport::Constants::SYMBIAN_PLATFORM_TR);
}

QtSupport::QtConfigWidget *SymbianQtVersion::createConfigurationWidget() const
{
    return new SymbianQtConfigWidget(const_cast<SymbianQtVersion *>(this));
}

SymbianQtConfigWidget::SymbianQtConfigWidget(SymbianQtVersion *version)
    : m_version(version)
{
    QFormLayout *fl = new QFormLayout();
    fl->setMargin(0);
    setLayout(fl);

    if (version->isBuildWithSymbianSbsV2()) {
        Utils::PathChooser *sbsV2Path = new Utils::PathChooser;
        sbsV2Path->setExpectedKind(Utils::PathChooser::ExistingDirectory);
        fl->addRow(tr("SBS v2 directory:"), sbsV2Path);
        sbsV2Path->setPath(QDir::toNativeSeparators(version->sbsV2Directory()));
        sbsV2Path->setEnabled(version->isBuildWithSymbianSbsV2());
        connect(sbsV2Path, SIGNAL(changed(QString)),
                this, SLOT(updateCurrentSbsV2Directory(QString)));
    }
}

void SymbianQtConfigWidget::updateCurrentSbsV2Directory(const QString &path)
{
    m_version->setSbsV2Directory(QDir::fromNativeSeparators(path));
    emit changed();
}
