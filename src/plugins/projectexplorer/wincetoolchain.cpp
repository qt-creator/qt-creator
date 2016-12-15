/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "wincetoolchain.h"

#include "msvcparser.h"
#include "projectexplorerconstants.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <QFormLayout>
#include <QLabel>

#include <QXmlStreamReader>

#define KEY_ROOT "ProjectExplorer.WinCEToolChain."
static const char msvcVerKeyC[] = KEY_ROOT"MSVCVer";
static const char ceVerKeyC[] = KEY_ROOT"CEVer";
static const char binPathKeyC[] = KEY_ROOT"BinPath";
static const char includePathKeyC[] = KEY_ROOT"IncludePath";
static const char libPathKeyC[] = KEY_ROOT"LibPath";
static const char supportedAbiKeyC[] = KEY_ROOT"SupportedAbi";
static const char vcVarsKeyC[] = KEY_ROOT"VCVars";

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

// Just decodes from the integer version to the string used in Qt mkspecs
static QString findMsvcVer(int version)
{
    if (version == 10)
        return QLatin1String("msvc2010");
    if (version == 9)
        return QLatin1String("msvc2008");
    return QLatin1String("msvc2005");
}


// Windows: Expand the delayed evaluation references returned by the
// SDK setup scripts: "PATH=$(Path);foo". Some values might expand
// to empty and should not be added
static QString winExpandDelayedEnvReferences(QString in, const Utils::Environment &env)
{
    const QString firstDelimit = QLatin1String("$(");
    const QChar secondDelimit = QLatin1Char(')');
    for (int pos = 0; pos < in.size(); ) {
        // Replace "$(REF)" by its value in process environment
        pos = in.indexOf(firstDelimit, pos);
        if (pos == -1)
            break;
        const int replaceStart = pos + firstDelimit.size();
        const int nextPos = in.indexOf(secondDelimit, replaceStart);
        if (nextPos == -1)
            break;
        const QString var = in.mid(replaceStart, nextPos - replaceStart);
        QString replacement = env.value(var.toUpper());
        if (replacement.isEmpty()) {
            qWarning() << "No replacement for var: " << var;
            pos = nextPos;
        } else {
            // Not sure about this, but we need to account for the case where
            // the end of the replacement doesn't have the directory separator and
            // neither does the start of the insert. This solution assumes:
            //  1) Having \\ in a path is valid (it is on WinXP)
            //  2) We're only replacing in paths. This will cause problems if there's
            //     a replace of a string
            if (!replacement.endsWith(QLatin1Char('\\')))
                replacement += QLatin1Char('\\');

            in.replace(pos, nextPos + 1 - pos, replacement);
            pos += replacement.size();
        }
    }
    return in;
}

// This is pretty much the same as the ReadEnvironmentSetting in the msvctoolchain.cpp, but
// this takes account of the library, binary and include paths to replace the vcvars versions
// with the ones for this toolchain.
Utils::Environment WinCEToolChain::readEnvironmentSetting(const Utils::Environment &env) const
{
    Utils::Environment result;
    QMap<QString, QString> envPairs;

    if (!generateEnvironmentSettings(env, m_vcvarsBat, QString(), envPairs))
        return env;

    for (auto envPairIter = envPairs.constBegin(); envPairIter != envPairs.constEnd(); ++envPairIter) {
        // Replace the env values with those from the WinCE SDK
        QString varValue = envPairIter.value();
        if (envPairIter.key() == QLatin1String("PATH"))
            varValue = m_binPath + QLatin1Char(';') + varValue;
        else if (envPairIter.key() == QLatin1String("INCLUDE"))
            varValue = m_includePath;
        else if (envPairIter.key() == QLatin1String("LIB"))
            varValue = m_libPath;

        if (!varValue.isEmpty())
            result.set(envPairIter.key(), varValue);
    }

    // Now loop round and do the delayed expansion
    for (auto envIter = result.constBegin(); envIter != result.constEnd(); ++envIter) {
        const QString key = result.key(envIter);
        const QString unexpandedValue = result.value(envIter);
        const QString expandedValue = winExpandDelayedEnvReferences(unexpandedValue, result);

        result.set(key, expandedValue);
    }

    if (debug) {
        const QStringList newVars = result.toStringList();
        const QStringList oldVars = env.toStringList();
        QDebug nsp = qDebug().nospace();
        foreach (const QString &n, newVars) {
            if (!oldVars.contains(n))
                nsp << n << '\n';
        }
    }
    return result;
}

// Used to parse an SDK entry in the config file and extract information about this SDK
static bool parseSDK(QXmlStreamReader& theReader,
                     Abi::Architecture& sdkArch,
                     QString& sdkName,
                     QString& ceVer,
                     QString& binPath,
                     QString& includePath,
                     QString& libPath)
{
    sdkArch = Abi::UnknownArchitecture;
    sdkName.clear();

    // Loop through until either the end of the file or until is gets to the next
    // end element.
    while (!theReader.atEnd()) {
        theReader.readNext();

        if (theReader.isEndElement()) {
            // Got to the end element so return...
            if (theReader.name() == QLatin1String("Platform"))
                return (sdkArch!=Abi::UnknownArchitecture && !sdkName.isEmpty());
        } else if (theReader.isStartElement()) {
            const QStringRef elemName = theReader.name();
            if (elemName == QLatin1String("PlatformName")) {
                sdkName = theReader.readElementText();
            } else if (elemName == QLatin1String("Directories")) {
                // Populate the paths from this element. Note: we remove the
                // $(PATH) from the binPath as this will be pre-pended in code
                binPath = theReader.attributes().value(QLatin1String("Path")).toString();
                binPath.remove(QLatin1String("$(PATH)"));
                includePath = theReader.attributes().value(QLatin1String("Include")).toString();
                libPath = theReader.attributes().value(QLatin1String("Library")).toString();
            } else if (elemName == QLatin1String("OSMajorVersion")) {
                // Qt only supports CE5 and higher so drop out here if this version is
                // invalid
                ceVer = theReader.readElementText();
                if (ceVer.toInt() < 5) {
                    if (debug)
                        qDebug("Ignoring SDK '%s'. Windows CE version %d is unsupported.", qPrintable(sdkName), ceVer.toInt());
                    return false;
                }
            } else if (elemName == QLatin1String("Macro")) {
                // Pull out the architecture from the macro values.
                if (theReader.attributes().value(QLatin1String("Name")) == QLatin1String("ARCHFAM")) {
                    const QStringRef archFam = theReader.attributes().value(QLatin1String("Value"));

                    if (archFam == QLatin1String("ARM"))
                        sdkArch = Abi::ArmArchitecture;
                    else if (archFam == QLatin1String("x86"))
                        sdkArch = Abi::X86Architecture;
                    else if (archFam == QLatin1String("MIPS"))
                        sdkArch = Abi::MipsArchitecture;
                }
            }
        }
    }

    // If we've got to here then the end of the file has been reached before the
    // end of element tag, so return error.
    return false;
}

// --------------------------------------------------------------------------
// WinCEToolChain
// --------------------------------------------------------------------------

WinCEToolChain::WinCEToolChain(const QString &name,
                               const Abi &abi,
                               const QString &vcvarsBat,
                               const QString &msvcVer,
                               const QString &ceVer,
                               const QString &binPath,
                               const QString &includePath,
                               const QString &libPath,
                               Core::Id language,
                               Detection d) :
    AbstractMsvcToolChain(Constants::WINCE_TOOLCHAIN_TYPEID, language, d, abi, vcvarsBat),
    m_msvcVer(msvcVer),
    m_ceVer(ceVer),
    m_binPath(binPath),
    m_includePath(includePath),
    m_libPath(libPath)
{
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(!m_binPath.isEmpty());
    Q_ASSERT(!m_includePath.isEmpty());
    Q_ASSERT(!m_libPath.isEmpty());

    setDisplayName(name);
}

WinCEToolChain::WinCEToolChain() :
    AbstractMsvcToolChain(Constants::WINCE_TOOLCHAIN_TYPEID, ManualDetection)
{
}

WinCEToolChain *WinCEToolChain::readFromMap(const QVariantMap &data)
{
    auto tc = new WinCEToolChain;
    if (tc->fromMap(data))
        return tc;
    delete tc;
    return nullptr;
}

QString WinCEToolChain::typeDisplayName() const
{
    return WinCEToolChainFactory::tr("WinCE");
}

Utils::FileNameList WinCEToolChain::suggestedMkspecList() const
{
    const QChar specSeperator(QLatin1Char('-'));

    QString specString = QLatin1String("wince");

    specString += m_ceVer;
    specString += specSeperator;
    specString += Abi::toString(m_abi.architecture());
    specString += specSeperator;
    specString += m_msvcVer;

    return Utils::FileNameList() << Utils::FileName::fromString(specString);
}


QString WinCEToolChain::ceVer() const
{
    return m_ceVer;
}


QVariantMap WinCEToolChain::toMap() const
{
    QVariantMap data = AbstractMsvcToolChain::toMap();

    data.insert(QLatin1String(msvcVerKeyC), m_msvcVer);
    data.insert(QLatin1String(ceVerKeyC), m_ceVer);
    data.insert(QLatin1String(binPathKeyC), m_binPath);
    data.insert(QLatin1String(includePathKeyC), m_includePath);
    data.insert(QLatin1String(libPathKeyC), m_libPath);
    data.insert(QLatin1String(vcVarsKeyC), m_vcvarsBat);

    data.insert(QLatin1String(supportedAbiKeyC), m_abi.toString());
    return data;
}

bool WinCEToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_msvcVer = data.value(QLatin1String(msvcVerKeyC)).toString();
    m_ceVer = data.value(QLatin1String(ceVerKeyC)).toString();
    m_binPath = data.value(QLatin1String(binPathKeyC)).toString();
    m_includePath = data.value(QLatin1String(includePathKeyC)).toString();
    m_libPath = data.value(QLatin1String(libPathKeyC)).toString();
    m_vcvarsBat = data.value(QLatin1String(vcVarsKeyC)).toString();

    const QString abiString = data.value(QLatin1String(supportedAbiKeyC)).toString();
    m_abi = Abi(abiString);

    return isValid();
}

ToolChainConfigWidget *WinCEToolChain::configurationWidget()
{
    return new WinCEToolChainConfigWidget(this);
}

ToolChain *WinCEToolChain::clone() const
{
    return new WinCEToolChain(*this);
}



// --------------------------------------------------------------------------
// WinCEToolChainFactory
// --------------------------------------------------------------------------

WinCEToolChainFactory::WinCEToolChainFactory()
{
    setDisplayName(tr("WinCE"));
}

QSet<Core::Id> WinCEToolChainFactory::supportedLanguages() const
{
    return { Constants::CXX_LANGUAGE_ID };
}

static ToolChain *findOrCreateToolChain(const QList<ToolChain *> &alreadyKnown,
                                        const QString &name, const Abi &abi,
                                        const QString &vcvarsBat, const QString &msvcVer,
                                        const QString &ceVer, const QString &binPath,
                                        const QString &includePath, const QString &libPath,
                                        ToolChain::Detection d = ToolChain::ManualDetection)
{
    ToolChain *tc
            = Utils::findOrDefault(alreadyKnown, [&](ToolChain *tc) -> bool {
                                       if (tc->typeId() != Constants::WINCE_TOOLCHAIN_TYPEID)
                                           return false;
                                       auto cetc = static_cast<WinCEToolChain *>(tc);
                                       return cetc->targetAbi() == abi
                                                  && cetc->varsBat() == vcvarsBat
                                                  && cetc->msvcVer() == msvcVer
                                                  && cetc->ceVer() == ceVer
                                                  && cetc->binPath() == binPath
                                                  && cetc->includePath() == includePath
                                                  && cetc->libPath() == libPath;
                                         });
    if (!tc)
        tc = new WinCEToolChain(name, abi, vcvarsBat, msvcVer, ceVer, binPath, includePath, libPath,
                                Constants::CXX_LANGUAGE_ID, d);
    return tc;
}

QList<ToolChain *> WinCEToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> results;

    // 1) Installed WinCEs
    const QSettings vsRegistry(
#ifdef Q_OS_WIN64
                QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7"),
#else
                QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VC7"),
#endif
                QSettings::NativeFormat);

    foreach (const QString &vsName, vsRegistry.allKeys()) {
        // Scan for version major.minor
        const int dotPos = vsName.indexOf(QLatin1Char('.'));
        if (dotPos == -1)
            continue;

        const QString path = QDir::fromNativeSeparators(vsRegistry.value(vsName).toString());
        const int version = vsName.leftRef(dotPos).toInt();

        // Check existence of various install scripts
        const QString vcvars32bat = path + QLatin1String("bin/vcvars32.bat");
        QFile cePlatforms(path + QLatin1String("vcpackages/WCE.VCPlatform.config"));

        if (cePlatforms.exists()) {
            const QString msvcVer = findMsvcVer(version);
            cePlatforms.open(QIODevice::ReadOnly);
            QXmlStreamReader platformReader(&cePlatforms);

            // Rip through the config file getting all of the installed platforms.
            while (!platformReader.atEnd()) {
                platformReader.readNext();
                if (platformReader.isStartElement()) {
                    if (platformReader.name() == QLatin1String("Platform")) {
                        Abi::Architecture theArch;
                        QString thePlat;
                        QString binPath;
                        QString includePath;
                        QString libPath;
                        QString ceVer;

                        if (parseSDK(platformReader, theArch, thePlat, ceVer, binPath, includePath, libPath)) {
                            results.append(findOrCreateToolChain(alreadyKnown,
                                                                 thePlat,
                                                                 Abi(theArch, Abi::WindowsOS, Abi::WindowsCEFlavor, Abi::PEFormat, 32),
                                                                 vcvars32bat,
                                                                 msvcVer,
                                                                 ceVer,
                                                                 binPath,
                                                                 includePath,
                                                                 libPath,
                                                                 ToolChain::AutoDetection));
                        }
                    }
                }
            }
        }
    }

    return results;
}


QString WinCEToolChain::autoDetectCdbDebugger(QStringList *checkedDirectories)
{
    Q_UNUSED(checkedDirectories);
    return QString();
}

bool WinCEToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::WINCE_TOOLCHAIN_TYPEID;
}

bool WinCEToolChain::operator ==(const ToolChain &other) const
{
    if (!AbstractMsvcToolChain::operator ==(other))
        return false;

    auto ceTc = static_cast<const WinCEToolChain *>(&other);
    return m_ceVer == ceTc->m_ceVer;
}

ToolChain *WinCEToolChainFactory::restore(const QVariantMap &data)
{
    return WinCEToolChain::readFromMap(data);
}

// --------------------------------------------------------------------------
// WinCEToolChainConfigWidget
// --------------------------------------------------------------------------

WinCEToolChainConfigWidget::WinCEToolChainConfigWidget(ToolChain *tc) :
    ToolChainConfigWidget(tc)
{
    auto toolChain = static_cast<WinCEToolChain *>(tc);
    QTC_ASSERT(tc, return);

    m_mainLayout->addRow(tr("SDK:"), new QLabel(toolChain->displayName()));
    m_mainLayout->addRow(tr("WinCE Version:"), new QLabel(toolChain->ceVer()));
    m_mainLayout->addRow(tr("ABI:"), new QLabel(toolChain->targetAbi().toString()));
    addErrorLabel();
}

} // namespace Internal
} // namespace ProjectExplorer
