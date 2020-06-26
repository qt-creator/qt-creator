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

#include "toolchain.h"

#include "abi.h"
#include "headerpath.h"
#include "projectexplorerconstants.h"
#include "toolchainmanager.h"
#include "task.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QUuid>

static const char ID_KEY[] = "ProjectExplorer.ToolChain.Id";
static const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ToolChain.DisplayName";
static const char AUTODETECT_KEY[] = "ProjectExplorer.ToolChain.Autodetect";
static const char LANGUAGE_KEY_V1[] = "ProjectExplorer.ToolChain.Language"; // For QtCreator <= 4.2
static const char LANGUAGE_KEY_V2[] = "ProjectExplorer.ToolChain.LanguageV2"; // For QtCreator > 4.2

namespace ProjectExplorer {
namespace Internal {

static QList<ToolChainFactory *> g_toolChainFactories;

// --------------------------------------------------------------------------
// ToolChainPrivate
// --------------------------------------------------------------------------

class ToolChainPrivate
{
public:
    using Detection = ToolChain::Detection;

    explicit ToolChainPrivate(Utils::Id typeId) :
        m_id(QUuid::createUuid().toByteArray()),
        m_typeId(typeId),
        m_predefinedMacrosCache(new ToolChain::MacrosCache::element_type()),
        m_headerPathsCache(new ToolChain::HeaderPathsCache::element_type())
    {
        QTC_ASSERT(m_typeId.isValid(), return);
        QTC_ASSERT(!m_typeId.toString().contains(QLatin1Char(':')), return);
    }

    QByteArray m_id;
    QSet<Utils::Id> m_supportedLanguages;
    mutable QString m_displayName;
    QString m_typeDisplayName;
    Utils::Id m_typeId;
    Utils::Id m_language;
    Detection m_detection = ToolChain::UninitializedDetection;

    ToolChain::MacrosCache m_predefinedMacrosCache;
    ToolChain::HeaderPathsCache m_headerPathsCache;
};


// Deprecated used from QtCreator <= 4.2

Utils::Id fromLanguageV1(int language)
{
    switch (language)
    {
    case Deprecated::Toolchain::C :
        return Utils::Id(Constants::C_LANGUAGE_ID);
    case Deprecated::Toolchain::Cxx:
        return Utils::Id(Constants::CXX_LANGUAGE_ID);
    case Deprecated::Toolchain::None:
    default:
        return Utils::Id();
    }
}

} // namespace Internal

namespace Deprecated {
namespace Toolchain {
QString languageId(Language l)
{
    switch (l) {
    case Language::None:
        return QStringLiteral("None");
    case Language::C:
        return QStringLiteral("C");
    case Language::Cxx:
        return QStringLiteral("Cxx");
    };
    return QString();
}
} // namespace Toolchain
} // namespace Deprecated

/*!
    \class ProjectExplorer::ToolChain
    \brief The ToolChain class represents a tool chain.
    \sa ProjectExplorer::ToolChainManager
*/

// --------------------------------------------------------------------------

ToolChain::ToolChain(Utils::Id typeId) :
    d(std::make_unique<Internal::ToolChainPrivate>(typeId))
{
}

void ToolChain::setLanguage(Utils::Id language)
{
    QTC_ASSERT(!d->m_language.isValid() || isAutoDetected(), return);
    QTC_ASSERT(language.isValid(), return);
    QTC_ASSERT(ToolChainManager::isLanguageSupported(language), return);

    d->m_language = language;
}

ToolChain::~ToolChain() = default;

QString ToolChain::displayName() const
{
    if (d->m_displayName.isEmpty())
        return typeDisplayName();
    return d->m_displayName;
}

void ToolChain::setDisplayName(const QString &name)
{
    if (d->m_displayName == name)
        return;

    d->m_displayName = name;
    toolChainUpdated();
}

ToolChain::Detection ToolChain::detection() const
{
    return d->m_detection;
}

QByteArray ToolChain::id() const
{
    return d->m_id;
}

QStringList ToolChain::suggestedMkspecList() const
{
    return {};
}

Utils::Id ToolChain::typeId() const
{
    return d->m_typeId;
}

Abis ToolChain::supportedAbis() const
{
    return {targetAbi()};
}

Utils::Id ToolChain::language() const
{
    return d->m_language;
}

bool ToolChain::operator == (const ToolChain &tc) const
{
    if (this == &tc)
        return true;

    // We ignore displayname
    return typeId() == tc.typeId()
            && isAutoDetected() == tc.isAutoDetected()
            && language() == tc.language();
}

ToolChain *ToolChain::clone() const
{
    for (ToolChainFactory *f : Internal::g_toolChainFactories) {
        if (f->supportedToolChainType() == d->m_typeId) {
            ToolChain *tc = f->create();
            QTC_ASSERT(tc, return nullptr);
            tc->fromMap(toMap());
            // New ID for the clone. It's different.
            tc->d->m_id = QUuid::createUuid().toByteArray();
            return tc;
        }
    }
    QTC_CHECK(false);
    return nullptr;
}

/*!
    Used by the tool chain manager to save user-generated tool chains.

    Make sure to call this function when deriving.
*/

QVariantMap ToolChain::toMap() const
{
    QVariantMap result;
    QString idToSave = d->m_typeId.toString() + QLatin1Char(':') + QString::fromUtf8(id());
    result.insert(QLatin1String(ID_KEY), idToSave);
    result.insert(QLatin1String(DISPLAY_NAME_KEY), displayName());
    result.insert(QLatin1String(AUTODETECT_KEY), isAutoDetected());
    // <Compatibility with QtC 4.2>
    int oldLanguageId = -1;
    if (language() == ProjectExplorer::Constants::C_LANGUAGE_ID)
        oldLanguageId = 1;
    else if (language() == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
        oldLanguageId = 2;
    if (oldLanguageId >= 0)
        result.insert(LANGUAGE_KEY_V1, oldLanguageId);
    // </Compatibility>
    result.insert(QLatin1String(LANGUAGE_KEY_V2), language().toSetting());
    return result;
}

void ToolChain::toolChainUpdated()
{
    d->m_predefinedMacrosCache->invalidate();
    d->m_headerPathsCache->invalidate();

    ToolChainManager::notifyAboutUpdate(this);
}

void ToolChain::setDetection(ToolChain::Detection de)
{
    if (d->m_detection == de)
        return;
    if (d->m_detection == ToolChain::UninitializedDetection) {
        d->m_detection = de;
    } else {
        d->m_detection = de;
        toolChainUpdated();
    }
}

QString ToolChain::typeDisplayName() const
{
    return d->m_typeDisplayName;
}

void ToolChain::setTypeDisplayName(const QString &typeName)
{
    d->m_typeDisplayName = typeName;
}

/*!
    Used by the tool chain manager to load user-generated tool chains.

    Make sure to call this function when deriving.
*/

bool ToolChain::fromMap(const QVariantMap &data)
{
    d->m_displayName = data.value(QLatin1String(DISPLAY_NAME_KEY)).toString();

    // make sure we have new style ids:
    const QString id = data.value(QLatin1String(ID_KEY)).toString();
    int pos = id.indexOf(QLatin1Char(':'));
    QTC_ASSERT(pos > 0, return false);
    d->m_typeId = Utils::Id::fromString(id.left(pos));
    d->m_id = id.mid(pos + 1).toUtf8();

    const bool autoDetect = data.value(QLatin1String(AUTODETECT_KEY), false).toBool();
    d->m_detection = autoDetect ? AutoDetection : ManualDetection;

    if (data.contains(LANGUAGE_KEY_V2)) {
        // remove hack to trim language id in 4.4: This is to fix up broken language
        // ids that happened in 4.3 master branch
        const QString langId = data.value(QLatin1String(LANGUAGE_KEY_V2)).toString();
        const int pos = langId.lastIndexOf('.');
        if (pos >= 0)
            d->m_language = Utils::Id::fromString(langId.mid(pos + 1));
        else
            d->m_language = Utils::Id::fromString(langId);
    } else if (data.contains(LANGUAGE_KEY_V1)) { // Import from old settings
        d->m_language = Internal::fromLanguageV1(data.value(QLatin1String(LANGUAGE_KEY_V1)).toInt());
    }

    if (!d->m_language.isValid())
        d->m_language = Utils::Id(Constants::CXX_LANGUAGE_ID);

    return true;
}

const ToolChain::HeaderPathsCache &ToolChain::headerPathsCache() const
{
    return d->m_headerPathsCache;
}

const ToolChain::MacrosCache &ToolChain::predefinedMacrosCache() const
{
    return d->m_predefinedMacrosCache;
}

static long toLanguageVersionAsLong(QByteArray dateAsByteArray)
{
    dateAsByteArray.chop(1); // Strip 'L'.

    bool success = false;
    const int result = dateAsByteArray.toLong(&success);
    QTC_CHECK(success);

    return result;
}

Utils::LanguageVersion ToolChain::cxxLanguageVersion(const QByteArray &cplusplusMacroValue)
{
    using Utils::LanguageVersion;
    const long version = toLanguageVersionAsLong(cplusplusMacroValue);

    if (version > 201703L)
        return LanguageVersion::LatestCxx;
    if (version > 201402L)
        return LanguageVersion::CXX17;
    if (version > 201103L)
        return LanguageVersion::CXX14;
    if (version == 201103L)
        return LanguageVersion::CXX11;

    return LanguageVersion::CXX03;
}

Utils::LanguageVersion ToolChain::languageVersion(const Utils::Id &language, const Macros &macros)
{
    using Utils::LanguageVersion;

    if (language == Constants::CXX_LANGUAGE_ID) {
        for (const ProjectExplorer::Macro &macro : macros) {
            if (macro.key == "__cplusplus") // Check for the C++ identifying macro
                return cxxLanguageVersion(macro.value);
        }

        QTC_CHECK(false && "__cplusplus is not predefined, assuming latest C++ we support.");
        return LanguageVersion::LatestCxx;
    } else if (language == Constants::C_LANGUAGE_ID) {
        for (const ProjectExplorer::Macro &macro : macros) {
            if (macro.key == "__STDC_VERSION__") {
                const long version = toLanguageVersionAsLong(macro.value);

                if (version > 201710L)
                    return LanguageVersion::LatestC;
                if (version > 201112L)
                    return LanguageVersion::C18;
                if (version > 199901L)
                    return LanguageVersion::C11;
                if (version > 199409L)
                    return LanguageVersion::C99;

                return LanguageVersion::C89;
            }
        }

        // The __STDC_VERSION__ macro was introduced after C89.
        // We haven't seen it, so it must be C89.
        return LanguageVersion::C89;
    } else {
        QTC_CHECK(false && "Unexpected toolchain language, assuming latest C++ we support.");
        return LanguageVersion::LatestCxx;
    }
}

/*!
    Used by the tool chain kit information to validate the kit.
*/

Tasks ToolChain::validateKit(const Kit *) const
{
    return {};
}

QString ToolChain::sysRoot() const
{
    return QString();
}

/*!
    \class ProjectExplorer::ToolChainFactory
    \brief The ToolChainFactory class creates tool chains from settings or
    autodetects them.
*/

/*!
    \fn QString ProjectExplorer::ToolChainFactory::displayName() const = 0
    Contains the name used to display the name of the tool chain that will be
    created.
*/

/*!
    \fn QStringList ProjectExplorer::ToolChain::clangParserFlags(const QStringList &cxxflags) const = 0
    Converts tool chain specific flags to list flags that tune the libclang
    parser.
*/

/*!
    \fn bool ProjectExplorer::ToolChainFactory::canRestore(const QVariantMap &data)
    Used by the tool chain manager to restore user-generated tool chains.
*/

ToolChainFactory::ToolChainFactory()
{
    Internal::g_toolChainFactories.append(this);
}

ToolChainFactory::~ToolChainFactory()
{
    Internal::g_toolChainFactories.removeOne(this);
}

const QList<ToolChainFactory *> ToolChainFactory::allToolChainFactories()
{
    return Internal::g_toolChainFactories;
}

QList<ToolChain *> ToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    Q_UNUSED(alreadyKnown)
    return {};
}

QList<ToolChain *> ToolChainFactory::detectForImport(const ToolChainDescription &tcd)
{
    Q_UNUSED(tcd)
    return {};
}

bool ToolChainFactory::canCreate() const
{
    return m_userCreatable;
}

ToolChain *ToolChainFactory::create()
{
    return m_toolchainConstructor ? m_toolchainConstructor() : nullptr;
}

ToolChain *ToolChainFactory::restore(const QVariantMap &data)
{
    if (!m_toolchainConstructor)
        return nullptr;

    ToolChain *tc = m_toolchainConstructor();
    QTC_ASSERT(tc, return nullptr);

    if (tc->fromMap(data))
        return tc;

    delete tc;
    return nullptr;
}

static QPair<QString, QString> rawIdData(const QVariantMap &data)
{
    const QString raw = data.value(QLatin1String(ID_KEY)).toString();
    const int pos = raw.indexOf(QLatin1Char(':'));
    QTC_ASSERT(pos > 0, return qMakePair(QString::fromLatin1("unknown"), QString::fromLatin1("unknown")));
    return qMakePair(raw.mid(0, pos), raw.mid(pos + 1));
}

QByteArray ToolChainFactory::idFromMap(const QVariantMap &data)
{
    return rawIdData(data).second.toUtf8();
}

Utils::Id ToolChainFactory::typeIdFromMap(const QVariantMap &data)
{
    return Utils::Id::fromString(rawIdData(data).first);
}

void ToolChainFactory::autoDetectionToMap(QVariantMap &data, bool detected)
{
    data.insert(QLatin1String(AUTODETECT_KEY), detected);
}

ToolChain *ToolChainFactory::createToolChain(Utils::Id toolChainType)
{
    for (ToolChainFactory *factory : qAsConst(Internal::g_toolChainFactories)) {
        if (factory->m_supportedToolChainType == toolChainType) {
            if (ToolChain *tc = factory->create()) {
                tc->d->m_typeId = toolChainType;
                return tc;
            }
        }
    }
    return nullptr;
}

QSet<Utils::Id> ToolChainFactory::supportedLanguages() const
{
    return m_supportsAllLanguages ? ToolChainManager::allLanguages() : m_supportedLanguages;
}

Utils::Id ToolChainFactory::supportedToolChainType() const
{
    return m_supportedToolChainType;
}

void ToolChainFactory::setSupportedToolChainType(const Utils::Id &supportedToolChain)
{
    m_supportedToolChainType = supportedToolChain;
}

void ToolChainFactory::setSupportedLanguages(const QSet<Utils::Id> &supportedLanguages)
{
    m_supportedLanguages = supportedLanguages;
}

void ToolChainFactory::setSupportsAllLanguages(bool supportsAllLanguages)
{
    m_supportsAllLanguages = supportsAllLanguages;
}

void ToolChainFactory::setToolchainConstructor
    (const std::function<ToolChain *()> &toolchainContructor)
{
    m_toolchainConstructor = toolchainContructor;
}

void ToolChainFactory::setUserCreatable(bool userCreatable)
{
    m_userCreatable = userCreatable;
}

} // namespace ProjectExplorer
