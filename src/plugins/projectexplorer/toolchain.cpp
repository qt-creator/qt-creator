// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchain.h"

#include "abi.h"
#include "devicesupport/idevice.h"
#include "projectexplorerconstants.h"
#include "toolchainmanager.h"
#include "task.h"

#include <utils/qtcassert.h>

#include <QUuid>

#include <utility>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

const char ID_KEY[] = "ProjectExplorer.ToolChain.Id";
const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ToolChain.DisplayName";
const char AUTODETECT_KEY[] = "ProjectExplorer.ToolChain.Autodetect";
const char DETECTION_SOURCE_KEY[] = "ProjectExplorer.ToolChain.DetectionSource";
const char LANGUAGE_KEY_V1[] = "ProjectExplorer.ToolChain.Language"; // For QtCreator <= 4.2
const char LANGUAGE_KEY_V2[] = "ProjectExplorer.ToolChain.LanguageV2"; // For QtCreator > 4.2
const char CODE_MODEL_TRIPLE_KEY[] = "ExplicitCodeModelTargetTriple";

QList<ToolChainFactory *> &toolChainFactories()
{
    static QList<ToolChainFactory *> theToolChainFactories;
    return theToolChainFactories;
}

// --------------------------------------------------------------------------
// ToolChainPrivate
// --------------------------------------------------------------------------

class ToolChainPrivate
{
public:
    using Detection = ToolChain::Detection;

    explicit ToolChainPrivate(Id typeId) :
        m_id(QUuid::createUuid().toByteArray()),
        m_typeId(typeId),
        m_predefinedMacrosCache(new ToolChain::MacrosCache::element_type()),
        m_headerPathsCache(new ToolChain::HeaderPathsCache::element_type())
    {
        QTC_ASSERT(m_typeId.isValid(), return);
        QTC_ASSERT(!m_typeId.name().contains(':'), return);
    }

    QByteArray m_id;
    FilePath m_compilerCommand;
    Key m_compilerCommandKey;
    Abi m_targetAbi;
    Key m_targetAbiKey;
    QSet<Id> m_supportedLanguages;
    mutable QString m_displayName;
    QString m_typeDisplayName;
    Id m_typeId;
    Id m_language;
    Detection m_detection = ToolChain::UninitializedDetection;
    QString m_detectionSource;
    QString m_explicitCodeModelTargetTriple;

    ToolChain::MacrosCache m_predefinedMacrosCache;
    ToolChain::HeaderPathsCache m_headerPathsCache;
    std::optional<bool> m_isValid;
    bool m_hasError = false;
};


// Deprecated used from QtCreator <= 4.2

Id fromLanguageV1(int language)
{
    switch (language)
    {
    case Deprecated::Toolchain::C :
        return Id(Constants::C_LANGUAGE_ID);
    case Deprecated::Toolchain::Cxx:
        return Id(Constants::CXX_LANGUAGE_ID);
    case Deprecated::Toolchain::None:
    default:
        return {};
    }
}

} // namespace Internal

namespace Deprecated::Toolchain {

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
    return {};
}

} // namespace Deprecated::ToolChain

using namespace Internal;

/*!
    \class ProjectExplorer::ToolChain
    \brief The ToolChain class represents a tool chain.
    \sa ProjectExplorer::ToolChainManager
*/

// --------------------------------------------------------------------------

ToolChain::ToolChain(Id typeId) :
    d(std::make_unique<ToolChainPrivate>(typeId))
{
}

void ToolChain::setLanguage(Id language)
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

bool ToolChain::isAutoDetected() const
{
    return detection() == AutoDetection || detection() == AutoDetectionFromSdk;
}

ToolChain::Detection ToolChain::detection() const
{
    return d->m_detection;
}

QString ToolChain::detectionSource() const
{
    return d->m_detectionSource;
}

QByteArray ToolChain::id() const
{
    return d->m_id;
}

QStringList ToolChain::suggestedMkspecList() const
{
    return {};
}

Id ToolChain::typeId() const
{
    return d->m_typeId;
}

Abis ToolChain::supportedAbis() const
{
    return {targetAbi()};
}

bool ToolChain::isValid() const
{
    if (!d->m_isValid.has_value())
        d->m_isValid = !compilerCommand().isEmpty() && compilerCommand().isExecutableFile();

    return d->m_isValid.value_or(false);
}

FilePaths ToolChain::includedFiles(const QStringList &flags, const FilePath &directory) const
{
    Q_UNUSED(flags)
    Q_UNUSED(directory)
    return {};
}

Id ToolChain::language() const
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
    for (ToolChainFactory *f : std::as_const(toolChainFactories())) {
        if (f->supportedToolChainType() == d->m_typeId) {
            ToolChain *tc = f->create();
            QTC_ASSERT(tc, return nullptr);
            Store data;
            toMap(data);
            tc->fromMap(data);
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

void ToolChain::toMap(Store &result) const
{
    AspectContainer::toMap(result);

    QString idToSave = d->m_typeId.toString() + QLatin1Char(':') + QString::fromUtf8(id());
    result.insert(ID_KEY, idToSave);
    result.insert(DISPLAY_NAME_KEY, displayName());
    result.insert(AUTODETECT_KEY, isAutoDetected());
    result.insert(DETECTION_SOURCE_KEY, d->m_detectionSource);
    result.insert(CODE_MODEL_TRIPLE_KEY, d->m_explicitCodeModelTargetTriple);
    // <Compatibility with QtC 4.2>
    int oldLanguageId = -1;
    if (language() == ProjectExplorer::Constants::C_LANGUAGE_ID)
        oldLanguageId = 1;
    else if (language() == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
        oldLanguageId = 2;
    if (oldLanguageId >= 0)
        result.insert(LANGUAGE_KEY_V1, oldLanguageId);
    // </Compatibility>
    result.insert(LANGUAGE_KEY_V2, language().toSetting());
    if (!d->m_targetAbiKey.isEmpty())
        result.insert(d->m_targetAbiKey, d->m_targetAbi.toString());
    if (!d->m_compilerCommandKey.isEmpty())
        result.insert(d->m_compilerCommandKey, d->m_compilerCommand.toSettings());
}

void ToolChain::toolChainUpdated()
{
    d->m_predefinedMacrosCache->invalidate();
    d->m_headerPathsCache->invalidate();

    ToolChainManager::notifyAboutUpdate(this);
}

void ToolChain::setDetection(ToolChain::Detection de)
{
    d->m_detection = de;
}

void ToolChain::setDetectionSource(const QString &source)
{
    d->m_detectionSource = source;
}

QString ToolChain::typeDisplayName() const
{
    return d->m_typeDisplayName;
}

Abi ToolChain::targetAbi() const
{
    return d->m_targetAbi;
}

void ToolChain::setTargetAbi(const Abi &abi)
{
    if (abi == d->m_targetAbi)
        return;

    d->m_targetAbi = abi;
    toolChainUpdated();
}

void ToolChain::setTargetAbiNoSignal(const Abi &abi)
{
    d->m_targetAbi = abi;
}

void ToolChain::setTargetAbiKey(const Key &abiKey)
{
    d->m_targetAbiKey = abiKey;
}

FilePath ToolChain::compilerCommand() const
{
    return d->m_compilerCommand;
}

void ToolChain::setCompilerCommand(const FilePath &command)
{
    d->m_isValid.reset();

    if (command == d->m_compilerCommand)
        return;
    d->m_compilerCommand = command;
    toolChainUpdated();
}

bool ToolChain::matchesCompilerCommand(const FilePath &command) const
{
    return compilerCommand().isSameExecutable(command);
}

void ToolChain::setCompilerCommandKey(const Key &commandKey)
{
    d->m_compilerCommandKey = commandKey;
}

void ToolChain::setTypeDisplayName(const QString &typeName)
{
    d->m_typeDisplayName = typeName;
}

/*!
    Used by the tool chain manager to load user-generated tool chains.

    Make sure to call this function when deriving.
*/

void ToolChain::fromMap(const Store &data)
{
    AspectContainer::fromMap(data);

    d->m_displayName = data.value(DISPLAY_NAME_KEY).toString();

    // make sure we have new style ids:
    const QString id = data.value(ID_KEY).toString();
    int pos = id.indexOf(QLatin1Char(':'));
    QTC_ASSERT(pos > 0, reportError(); return);
    d->m_typeId = Id::fromString(id.left(pos));
    d->m_id = id.mid(pos + 1).toUtf8();

    const bool autoDetect = data.value(AUTODETECT_KEY, false).toBool();
    d->m_detection = autoDetect ? AutoDetection : ManualDetection;
    d->m_detectionSource = data.value(DETECTION_SOURCE_KEY).toString();

    d->m_explicitCodeModelTargetTriple = data.value(CODE_MODEL_TRIPLE_KEY).toString();

    if (data.contains(LANGUAGE_KEY_V2)) {
        // remove hack to trim language id in 4.4: This is to fix up broken language
        // ids that happened in 4.3 master branch
        const QString langId = data.value(LANGUAGE_KEY_V2).toString();
        const int pos = langId.lastIndexOf('.');
        if (pos >= 0)
            d->m_language = Id::fromString(langId.mid(pos + 1));
        else
            d->m_language = Id::fromString(langId);
    } else if (data.contains(LANGUAGE_KEY_V1)) { // Import from old settings
        d->m_language = Internal::fromLanguageV1(data.value(LANGUAGE_KEY_V1).toInt());
    }

    if (!d->m_language.isValid())
        d->m_language = Id(Constants::CXX_LANGUAGE_ID);

    if (!d->m_targetAbiKey.isEmpty())
        d->m_targetAbi = Abi::fromString(data.value(d->m_targetAbiKey).toString());

    d->m_compilerCommand = FilePath::fromSettings(data.value(d->m_compilerCommandKey));
    d->m_isValid.reset();
}

void ToolChain::reportError()
{
    d->m_hasError = true;
}

bool ToolChain::hasError() const
{
    return d->m_hasError;
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
    if (dateAsByteArray.endsWith('L'))
        dateAsByteArray.chop(1); // Strip 'L'.

    bool success = false;
    const int result = dateAsByteArray.toLong(&success);
    QTC_CHECK(success);

    return result;
}

LanguageVersion ToolChain::cxxLanguageVersion(const QByteArray &cplusplusMacroValue)
{
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

LanguageVersion ToolChain::languageVersion(const Id &language, const Macros &macros)
{
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

FilePaths ToolChain::includedFiles(const QString &option,
                                   const QStringList &flags,
                                   const FilePath &directoryPath,
                                   PossiblyConcatenatedFlag possiblyConcatenated)
{
    FilePaths result;

    for (int i = 0; i < flags.size(); ++i) {
        QString includeFile;
        const QString flag = flags[i];
        if (possiblyConcatenated == PossiblyConcatenatedFlag::Yes
                && flag.startsWith(option)
                && flag.size() > option.size()) {
            includeFile = flag.mid(option.size());
        }
        if (includeFile.isEmpty() && flag == option && i + 1 < flags.size())
            includeFile = flags[++i];

        if (!includeFile.isEmpty())
            result.append(directoryPath.resolvePath(includeFile));
    }

    return result;
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
    return {};
}

QString ToolChain::explicitCodeModelTargetTriple() const
{
    return d->m_explicitCodeModelTargetTriple;
}

QString ToolChain::effectiveCodeModelTargetTriple() const
{
    const QString overridden = explicitCodeModelTargetTriple();
    if (!overridden.isEmpty())
        return overridden;
    return originalTargetTriple();
}

void ToolChain::setExplicitCodeModelTargetTriple(const QString &triple)
{
    d->m_explicitCodeModelTargetTriple = triple;
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
    \fn bool ProjectExplorer::ToolChainFactory::canRestore(const Store &data)
    Used by the tool chain manager to restore user-generated tool chains.
*/

ToolChainFactory::ToolChainFactory()
{
    toolChainFactories().append(this);
}

ToolChainFactory::~ToolChainFactory()
{
    toolChainFactories().removeOne(this);
}

const QList<ToolChainFactory *> ToolChainFactory::allToolChainFactories()
{
    return toolChainFactories();
}

Toolchains ToolChainFactory::autoDetect(const ToolchainDetector &detector) const
{
    Q_UNUSED(detector)
    return {};
}

Toolchains ToolChainFactory::detectForImport(const ToolChainDescription &tcd) const
{
    Q_UNUSED(tcd)
    return {};
}

bool ToolChainFactory::canCreate() const
{
    return m_userCreatable;
}

ToolChain *ToolChainFactory::create() const
{
    return m_toolchainConstructor ? m_toolchainConstructor() : nullptr;
}

ToolChain *ToolChainFactory::restore(const Store &data)
{
    if (!m_toolchainConstructor)
        return nullptr;

    ToolChain *tc = m_toolchainConstructor();
    QTC_ASSERT(tc, return nullptr);

    tc->fromMap(data);
    if (!tc->hasError())
        return tc;

    delete tc;
    return nullptr;
}

static QPair<QString, QString> rawIdData(const Store &data)
{
    const QString raw = data.value(ID_KEY).toString();
    const int pos = raw.indexOf(QLatin1Char(':'));
    QTC_ASSERT(pos > 0, return qMakePair(QString::fromLatin1("unknown"), QString::fromLatin1("unknown")));
    return {raw.mid(0, pos), raw.mid(pos + 1)};
}

QByteArray ToolChainFactory::idFromMap(const Store &data)
{
    return rawIdData(data).second.toUtf8();
}

Id ToolChainFactory::typeIdFromMap(const Store &data)
{
    return Id::fromString(rawIdData(data).first);
}

void ToolChainFactory::autoDetectionToMap(Store &data, bool detected)
{
    data.insert(AUTODETECT_KEY, detected);
}

ToolChain *ToolChainFactory::createToolChain(Id toolChainType)
{
    for (ToolChainFactory *factory : std::as_const(toolChainFactories())) {
        if (factory->m_supportedToolChainType == toolChainType) {
            if (ToolChain *tc = factory->create()) {
                tc->d->m_typeId = toolChainType;
                return tc;
            }
        }
    }
    return nullptr;
}

QList<Id> ToolChainFactory::supportedLanguages() const
{
    return m_supportsAllLanguages ? ToolChainManager::allLanguages() : m_supportedLanguages;
}

Id ToolChainFactory::supportedToolChainType() const
{
    return m_supportedToolChainType;
}

void ToolChainFactory::setSupportedToolChainType(const Id &supportedToolChain)
{
    m_supportedToolChainType = supportedToolChain;
}

void ToolChainFactory::setSupportedLanguages(const QList<Id> &supportedLanguages)
{
    m_supportedLanguages = supportedLanguages;
}

void ToolChainFactory::setSupportsAllLanguages(bool supportsAllLanguages)
{
    m_supportsAllLanguages = supportsAllLanguages;
}

void ToolChainFactory::setToolchainConstructor(const ToolChainConstructor &toolchainContructor)
{
    m_toolchainConstructor = toolchainContructor;
}

ToolChainFactory::ToolChainConstructor ToolChainFactory::toolchainConstructor() const
{
    return m_toolchainConstructor;
}

void ToolChainFactory::setUserCreatable(bool userCreatable)
{
    m_userCreatable = userCreatable;
}

ToolchainDetector::ToolchainDetector(const Toolchains &alreadyKnown,
                                     const IDevice::ConstPtr &device,
                                     const FilePaths &searchPaths)
    : alreadyKnown(alreadyKnown), device(device), searchPaths(searchPaths)
{
    QTC_CHECK(device);
}

BadToolchain::BadToolchain(const FilePath &filePath)
    : BadToolchain(filePath, filePath.symLinkTarget(), filePath.lastModified())
{}

BadToolchain::BadToolchain(const FilePath &filePath, const FilePath &symlinkTarget,
                           const QDateTime &timestamp)
    : filePath(filePath), symlinkTarget(symlinkTarget), timestamp(timestamp)
{}


static Key badToolchainFilePathKey() { return {"FilePath"}; }
static Key badToolchainSymlinkTargetKey() { return {"TargetFilePath"}; }
static Key badToolchainTimestampKey() { return {"Timestamp"}; }

Store BadToolchain::toMap() const
{
    return {{badToolchainFilePathKey(), filePath.toSettings()},
            {badToolchainSymlinkTargetKey(), symlinkTarget.toSettings()},
            {badToolchainTimestampKey(), timestamp.toMSecsSinceEpoch()}};
}

BadToolchain BadToolchain::fromMap(const Store &map)
{
    return {
        FilePath::fromSettings(map.value(badToolchainFilePathKey())),
        FilePath::fromSettings(map.value(badToolchainSymlinkTargetKey())),
        QDateTime::fromMSecsSinceEpoch(map.value(badToolchainTimestampKey()).toLongLong())
    };
}

BadToolchains::BadToolchains(const QList<BadToolchain> &toolchains)
    : toolchains(Utils::filtered(toolchains, [](const BadToolchain &badTc) {
          return badTc.filePath.lastModified() == badTc.timestamp
                  && badTc.filePath.symLinkTarget() == badTc.symlinkTarget;
      }))
{}

bool BadToolchains::isBadToolchain(const FilePath &toolchain) const
{
    return Utils::contains(toolchains, [toolchain](const BadToolchain &badTc) {
        return badTc.filePath == toolchain.absoluteFilePath()
                || badTc.symlinkTarget == toolchain.absoluteFilePath();
    });
}

QVariant BadToolchains::toVariant() const
{
    return Utils::transform<QVariantList>(toolchains, [](const BadToolchain &bdc) {
        return variantFromStore(bdc.toMap());
    });
}

BadToolchains BadToolchains::fromVariant(const QVariant &v)
{
    return Utils::transform<QList<BadToolchain>>(v.toList(),
            [](const QVariant &e) { return BadToolchain::fromMap(storeFromVariant(e)); });
}

} // namespace ProjectExplorer
