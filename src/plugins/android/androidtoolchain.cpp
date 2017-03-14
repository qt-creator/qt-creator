/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "androidtoolchain.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidqtversion.h"

#include <extensionsystem/pluginmanager.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/target.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QDirIterator>
#include <QFormLayout>
#include <QLabel>
#include <QRegExp>
#include <QVBoxLayout>

namespace {
    const QLatin1String NDKGccVersionRegExp("-\\d[\\.\\d]+");
}

namespace Android {
namespace Internal {

using namespace ProjectExplorer;
using namespace Utils;

static const char ANDROID_QT_VERSION_KEY[] = "Qt4ProjectManager.Android.QtVersion";
static const char ANDROID_NDK_TC_VERION[] = "Qt4ProjectManager.Android.NDK_TC_VERION";

QHash<Abi, QList<int> > AndroidToolChainFactory::m_newestVersionForAbi;
FileName AndroidToolChainFactory::m_ndkLocation;

AndroidToolChain::AndroidToolChain(const Abi &abi, const QString &ndkToolChainVersion, Core::Id l, Detection d)
    : GccToolChain(Constants::ANDROID_TOOLCHAIN_ID, d),
      m_ndkToolChainVersion(ndkToolChainVersion), m_secondaryToolChain(false)
{
    setLanguage(l);
    setTargetAbi(abi);
    setDisplayName(QString::fromLatin1("Android GCC (%1, %2-%3)")
                   .arg(ToolChainManager::displayNameOfLanguageId(l),
                        AndroidConfig::displayName(targetAbi()),
                        ndkToolChainVersion));
}

// for fromMap
AndroidToolChain::AndroidToolChain()
    : GccToolChain(Constants::ANDROID_TOOLCHAIN_ID, ToolChain::ManualDetection),
      m_secondaryToolChain(false)
{
}

AndroidToolChain::AndroidToolChain(const AndroidToolChain &tc) :
    GccToolChain(tc), m_ndkToolChainVersion(tc.m_ndkToolChainVersion),
    m_secondaryToolChain(tc.m_secondaryToolChain)
{ }

AndroidToolChain::~AndroidToolChain()
{ }

QString AndroidToolChain::typeDisplayName() const
{
    return AndroidToolChainFactory::tr("Android GCC");
}

bool AndroidToolChain::isValid() const
{
    return GccToolChain::isValid() && targetAbi().isValid() && !m_ndkToolChainVersion.isEmpty()
            && compilerCommand().isChildOf(AndroidConfigurations::currentConfig().ndkLocation());
}

void AndroidToolChain::addToEnvironment(Environment &env) const
{
// TODO this vars should be configurable in projects -> build tab
// TODO invalidate all .pro files !!!

    env.set(QLatin1String("ANDROID_NDK_HOST"), AndroidConfigurations::currentConfig().toolchainHost());
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX"), AndroidConfig::toolchainPrefix(targetAbi()));
    env.set(QLatin1String("ANDROID_NDK_TOOLS_PREFIX"), AndroidConfig::toolsPrefix(targetAbi()));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION"), m_ndkToolChainVersion);
    const Utils::FileName javaHome = AndroidConfigurations::currentConfig().openJDKLocation();
    if (!javaHome.isEmpty() && javaHome.toFileInfo().exists()) {
        env.set(QLatin1String("JAVA_HOME"), javaHome.toString());
        Utils::FileName javaBin = javaHome;
        javaBin.appendPath(QLatin1String("bin"));
        const QString jb = javaBin.toUserOutput();
        if (!Utils::contains(env.path(), [&jb](const QString &p) { return p == jb; }))
            env.prependOrSetPath(jb);
    }
    env.set(QLatin1String("ANDROID_HOME"), AndroidConfigurations::currentConfig().sdkLocation().toString());
    env.set(QLatin1String("ANDROID_SDK_ROOT"), AndroidConfigurations::currentConfig().sdkLocation().toString());
}

bool AndroidToolChain::operator ==(const ToolChain &tc) const
{
    if (!GccToolChain::operator ==(tc))
        return false;

    return m_ndkToolChainVersion == static_cast<const AndroidToolChain &>(tc).m_ndkToolChainVersion;
}

ToolChainConfigWidget *AndroidToolChain::configurationWidget()
{
    return new AndroidToolChainConfigWidget(this);
}

FileName AndroidToolChain::suggestedDebugger() const
{
    return AndroidConfigurations::currentConfig().gdbPath(targetAbi(), m_ndkToolChainVersion);
}

FileName AndroidToolChain::suggestedGdbServer() const
{
    FileName path = AndroidConfigurations::currentConfig().ndkLocation();
    path.appendPath(QString::fromLatin1("prebuilt/android-%1/gdbserver/gdbserver")
                    .arg(Abi::toString(targetAbi().architecture())));
    if (path.exists())
        return path;
    path = AndroidConfigurations::currentConfig().ndkLocation();
    path.appendPath(QString::fromLatin1("toolchains/%1-%2/prebuilt/gdbserver")
                               .arg(AndroidConfig::toolchainPrefix(targetAbi()))
                               .arg(m_ndkToolChainVersion));
    if (path.exists())
        return path;

    return FileName();
}

QVariantMap AndroidToolChain::toMap() const
{
    QVariantMap result = GccToolChain::toMap();
    result.insert(QLatin1String(ANDROID_NDK_TC_VERION), m_ndkToolChainVersion);
    return result;
}

bool AndroidToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    if (data.contains(QLatin1String(ANDROID_QT_VERSION_KEY))) {
        QString command = compilerCommand().toString();
        QString ndkPath = AndroidConfigurations::currentConfig().ndkLocation().toString();
        if (!command.startsWith(ndkPath))
            return false;
        command = command.mid(ndkPath.length());
        if (!command.startsWith(QLatin1String("/toolchains/")))
            return false;
        command = command.mid(12);
        int index = command.indexOf(QLatin1Char('/'));
        if (index == -1)
            return false;
        command = command.left(index);
        QRegExp versionRegExp(NDKGccVersionRegExp);
        index = versionRegExp.indexIn(command);
        if (index == -1)
            return false;
        m_ndkToolChainVersion = command.mid(index + 1);
        QString platform = command.left(index);
        setTargetAbi(AndroidConfig::abiForToolChainPrefix(platform));
    } else {
        m_ndkToolChainVersion = data.value(QLatin1String(ANDROID_NDK_TC_VERION)).toString();
    }

    Abi abi = targetAbi();
    m_secondaryToolChain = AndroidToolChainFactory::versionCompareLess(AndroidToolChainFactory::versionNumberFromString(m_ndkToolChainVersion),
                                                                       AndroidToolChainFactory::newestToolChainVersionForArch(abi));
    return isValid();
}

FileNameList AndroidToolChain::suggestedMkspecList() const
{
    return FileNameList()<< FileName::fromLatin1("android-g++");
}

QString AndroidToolChain::makeCommand(const Environment &env) const
{
    QStringList extraDirectories = AndroidConfigurations::currentConfig().makeExtraSearchDirectories();
    if (HostOsInfo::isWindowsHost()) {
        FileName tmp = env.searchInPath(QLatin1String("ma-make.exe"), extraDirectories);
        if (!tmp.isEmpty())
            return QString();
        tmp = env.searchInPath(QLatin1String("mingw32-make"), extraDirectories);
        return tmp.isEmpty() ? QLatin1String("mingw32-make") : tmp.toString();
    }

    QString make = QLatin1String("make");
    FileName tmp = env.searchInPath(make, extraDirectories);
    return tmp.isEmpty() ? make : tmp.toString();
}

QString AndroidToolChain::ndkToolChainVersion() const
{
    return m_ndkToolChainVersion;
}

bool AndroidToolChain::isSecondaryToolChain() const
{
    return m_secondaryToolChain;
}

void AndroidToolChain::setSecondaryToolChain(bool b)
{
    if (m_secondaryToolChain == b)
        return;
    m_secondaryToolChain = b;
    toolChainUpdated();
}

GccToolChain::DetectedAbisResult AndroidToolChain::detectSupportedAbis() const
{
    return QList<Abi>() << targetAbi();
}

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

AndroidToolChainConfigWidget::AndroidToolChainConfigWidget(AndroidToolChain *tc) :
   ToolChainConfigWidget(tc)
{
    QLabel *label = new QLabel(AndroidConfigurations::currentConfig().ndkLocation().toUserOutput());
    m_mainLayout->addRow(tr("NDK Root:"), label);
}

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

AndroidToolChainFactory::AndroidToolChainFactory()
{
    setDisplayName(tr("Android GCC"));
}

QSet<Core::Id> Android::Internal::AndroidToolChainFactory::supportedLanguages() const
{
    return {ProjectExplorer::Constants::CXX_LANGUAGE_ID};
}

QList<ToolChain *> AndroidToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    return autodetectToolChainsForNdk(AndroidConfigurations::currentConfig().ndkLocation(), alreadyKnown);
}

bool AndroidToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::ANDROID_TOOLCHAIN_ID;
}

ToolChain *AndroidToolChainFactory::restore(const QVariantMap &data)
{
    AndroidToolChain *tc = new AndroidToolChain();
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

QList<AndroidToolChainFactory::AndroidToolChainInformation> AndroidToolChainFactory::toolchainPathsForNdk(const FileName &ndkPath)
{
    QList<AndroidToolChainInformation> result;
    if (ndkPath.isEmpty())
        return result;
    QRegExp versionRegExp(NDKGccVersionRegExp);
    FileName path = ndkPath;
    QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                    QStringList("*"), QDir::Dirs);
    while (it.hasNext()) {
        const QString &fileName = FileName::fromString(it.next()).fileName();
        int idx = versionRegExp.indexIn(fileName);
        if (idx == -1)
            continue;
        for (const Core::Id lang : {ProjectExplorer::Constants::CXX_LANGUAGE_ID,
                                    ProjectExplorer::Constants::C_LANGUAGE_ID}) {
            AndroidToolChainInformation ati;
            ati.language = lang;
            ati.version = fileName.mid(idx + 1);
            QString platform = fileName.left(idx);
            ati.abi = AndroidConfig::abiForToolChainPrefix(platform);
            if (ati.abi.architecture() == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
                continue;
            ati.compilerCommand = AndroidConfigurations::currentConfig().gccPath(ati.abi, lang, ati.version);
            result.append(ati);
        }
    }
    return result;
}

QList<int> AndroidToolChainFactory::versionNumberFromString(const QString &version)
{
    QList<int> result;
    int start = 0;
    int end = version.length();
    while (start <= end) {
        int index = version.indexOf(QLatin1Char('.'), start);
        if (index == -1)
            index = end;

        bool ok;
        int v = version.midRef(start, index - start).toInt(&ok);
        if (!ok) // unparseable, return what we have
            return result;

        result << v;
        start = index + 1;
    }
    return result;
}

bool AndroidToolChainFactory::versionCompareLess(const QList<int> &a, const QList<int> &b)
{
    int aend = a.length();
    int bend = b.length();
    int end = qMax(aend, bend);
    for (int i = 0; i < end; ++i) {
        int an = i < aend ? a.at(i) : 0;
        int bn = i < bend ? b.at(i) : 0;
        if (an < bn)
            return true;
        if (bn < an)
            return false;
    }
    return false;
}

bool AndroidToolChainFactory::versionCompareLess(QList<AndroidToolChain *> atc,
                                                 QList<AndroidToolChain *> btc)
{
    const QList<int> a = versionNumberFromString(atc.at(0)->ndkToolChainVersion());
    const QList<int> b = versionNumberFromString(btc.at(0)->ndkToolChainVersion());

    return versionCompareLess(a, b);
}

static AndroidToolChain *findToolChain(Utils::FileName &compilerPath, Core::Id lang,
                                       const QList<ToolChain *> &alreadyKnown)
{
    return static_cast<AndroidToolChain *>(
                Utils::findOrDefault(alreadyKnown, [compilerPath, lang](ToolChain *tc) {
                                                       return tc->typeId() == Constants::ANDROID_TOOLCHAIN_ID
                                                           && tc->language() == lang
                                                           && tc->compilerCommand() == compilerPath;
                                                   }));
}

QList<ToolChain *>
AndroidToolChainFactory::autodetectToolChainsForNdk(const FileName &ndkPath,
                                                    const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> result;
    if (ndkPath.isEmpty())
        return result;

    QRegExp versionRegExp(NDKGccVersionRegExp);
    FileName path = ndkPath;
    QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                    QStringList("*"), QDir::Dirs);
    QHash<Abi, QList<AndroidToolChain *>> newestToolChainForArch;

    while (it.hasNext()) {
        const QString &fileName = FileName::fromString(it.next()).fileName();
        int idx = versionRegExp.indexIn(fileName);
        if (idx == -1)
            continue;
        QString version = fileName.mid(idx + 1);
        QString platform = fileName.left(idx);
        Abi abi = AndroidConfig::abiForToolChainPrefix(platform);
        if (abi.architecture() == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
            continue;
        QList<AndroidToolChain *> toolChainBundle;
        for (Core::Id lang : {ProjectExplorer::Constants::CXX_LANGUAGE_ID, ProjectExplorer::Constants::C_LANGUAGE_ID}) {
            FileName compilerPath = AndroidConfigurations::currentConfig().gccPath(abi, lang, version);

            AndroidToolChain *tc = findToolChain(compilerPath, lang, alreadyKnown);
            if (!tc) {
                tc = new AndroidToolChain(abi, version, lang,
                                          ToolChain::AutoDetection);
                tc->resetToolChain(compilerPath);
            }
            result.append(tc);
            toolChainBundle.append(tc);
        }

        auto it = newestToolChainForArch.constFind(abi);
        if (it == newestToolChainForArch.constEnd())
            newestToolChainForArch.insert(abi, toolChainBundle);
        else if (versionCompareLess(it.value(), toolChainBundle))
            newestToolChainForArch[abi] = toolChainBundle;
    }

    foreach (ToolChain *tc, result) {
        AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);
        atc->setSecondaryToolChain(!newestToolChainForArch.value(atc->targetAbi()).contains(atc));
    }

    return result;
}

QList<int> AndroidToolChainFactory::newestToolChainVersionForArch(const Abi &abi)
{
    if (m_newestVersionForAbi.isEmpty()
            || m_ndkLocation != AndroidConfigurations::currentConfig().ndkLocation()) {
        QRegExp versionRegExp(NDKGccVersionRegExp);
        m_ndkLocation = AndroidConfigurations::currentConfig().ndkLocation();
        FileName path = m_ndkLocation;
        QDirIterator it(path.appendPath(QLatin1String("toolchains")).toString(),
                        QStringList("*"), QDir::Dirs);
        while (it.hasNext()) {
            const QString &fileName = FileName::fromString(it.next()).fileName();
            int idx = versionRegExp.indexIn(fileName);
            if (idx == -1)
                continue;
            QList<int> version = versionNumberFromString(fileName.mid(idx + 1));
            QString platform = fileName.left(idx);
            Abi abi = AndroidConfig::abiForToolChainPrefix(platform);
            if (abi.architecture() == Abi::UnknownArchitecture) // e.g. mipsel which is not yet supported
                continue;
            QHash<Abi, QList<int> >::const_iterator it
                    = m_newestVersionForAbi.constFind(abi);
            if (it == m_newestVersionForAbi.constEnd())
                m_newestVersionForAbi.insert(abi, version);
            else if (versionCompareLess(it.value(), version))
                m_newestVersionForAbi[abi] = version;
        }
    }
    return m_newestVersionForAbi.value(abi);
}

} // namespace Internal
} // namespace Android
