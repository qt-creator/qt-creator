// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchain.h"

#include "abi.h"
#include "devicesupport/idevice.h"
#include "gcctoolchain.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "toolchainmanager.h"
#include "task.h"

#include <utils/async.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QUuid>

#include <utility>

using namespace Utils;

namespace ProjectExplorer {

namespace Internal {

const char ID_KEY[] = "ProjectExplorer.ToolChain.Id";
const char BUNDLE_ID_KEY[] = "ProjectExplorer.ToolChain.BundleId";
const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ToolChain.DisplayName";
const char AUTODETECT_KEY[] = "ProjectExplorer.ToolChain.Autodetect";
const char DETECTION_SOURCE_KEY[] = "ProjectExplorer.ToolChain.DetectionSource";
const char LANGUAGE_KEY_V1[] = "ProjectExplorer.ToolChain.Language"; // For QtCreator <= 4.2
const char LANGUAGE_KEY_V2[] = "ProjectExplorer.ToolChain.LanguageV2"; // For QtCreator > 4.2
const char CODE_MODEL_TRIPLE_KEY[] = "ExplicitCodeModelTargetTriple";

QList<ToolchainFactory *> &toolchainFactories()
{
    static QList<ToolchainFactory *> theToolchainFactories;
    return theToolchainFactories;
}

// --------------------------------------------------------------------------
// ToolchainPrivate
// --------------------------------------------------------------------------

class ToolchainPrivate
{
public:
    using Detection = Toolchain::Detection;

    explicit ToolchainPrivate(Id typeId) :
        m_id(QUuid::createUuid().toByteArray()),
        m_typeId(typeId),
        m_predefinedMacrosCache(new Toolchain::MacrosCache::element_type()),
        m_headerPathsCache(new Toolchain::HeaderPathsCache::element_type())
    {
        QTC_ASSERT(m_typeId.isValid(), return);
        QTC_ASSERT(!m_typeId.name().contains(':'), return);
    }

    QByteArray m_id;
    Id m_bundleId;
    FilePath m_compilerCommand;
    Key m_compilerCommandKey;
    Abi m_targetAbi;
    Key m_targetAbiKey;
    QSet<Id> m_supportedLanguages;
    mutable QString m_displayName;
    QString m_typeDisplayName;
    Id m_typeId;
    Id m_language;
    Detection m_detection = Toolchain::UninitializedDetection;
    QString m_detectionSource;
    QString m_explicitCodeModelTargetTriple;

    Toolchain::MacrosCache m_predefinedMacrosCache;
    Toolchain::HeaderPathsCache m_headerPathsCache;
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

} // namespace Deprecated::Toolchain

using namespace Internal;

/*!
    \class ProjectExplorer::Toolchain
    \brief The Toolchain class represents a tool chain.
    \sa ProjectExplorer::ToolchainManager
*/

// --------------------------------------------------------------------------

Toolchain::Toolchain(Id typeId) :
    d(std::make_unique<ToolchainPrivate>(typeId))
{
}

bool Toolchain::canShareBundleImpl(const Toolchain &other) const
{
    Q_UNUSED(other)
    return true;
}

void Toolchain::setLanguage(Id language)
{
    QTC_ASSERT(language.isValid(), return);
    QTC_ASSERT(ToolchainManager::isLanguageSupported(language), return);

    d->m_language = language;
}

Toolchain::~Toolchain() = default;

QString Toolchain::displayName() const
{
    if (d->m_displayName.isEmpty())
        return typeDisplayName();
    return d->m_displayName;
}

void Toolchain::setDisplayName(const QString &name)
{
    if (d->m_displayName == name)
        return;

    d->m_displayName = name;
    toolChainUpdated();
}

bool Toolchain::isAutoDetected() const
{
    return detection() == AutoDetection || detection() == AutoDetectionFromSdk;
}

Toolchain::Detection Toolchain::detection() const
{
    return d->m_detection;
}

QString Toolchain::detectionSource() const
{
    return d->m_detectionSource;
}

ToolchainFactory *Toolchain::factory() const
{
    return ToolchainFactory::factoryForType(typeId());
}

QByteArray Toolchain::id() const
{
    return d->m_id;
}

Id Toolchain::bundleId() const
{
    return d->m_bundleId;
}

void Toolchain::setBundleId(Utils::Id id)
{
    d->m_bundleId = id;
}

bool Toolchain::canShareBundle(const Toolchain &other) const
{
    QTC_ASSERT(typeId() == other.typeId(), return false);
    QTC_ASSERT(language() != other.language(), return false);

    if (int(factory()->supportedLanguages().size()) == 1)
        return false;
    if (detection() != other.detection())
        return false;

    if (typeId() != Constants::MSVC_TOOLCHAIN_TYPEID
            && typeId() != Constants::CLANG_CL_TOOLCHAIN_TYPEID
            && (targetAbi() != other.targetAbi() || supportedAbis() != other.supportedAbis()
                || extraCodeModelFlags() != other.extraCodeModelFlags()
                || suggestedMkspecList() != other.suggestedMkspecList() || sysRoot() != other.sysRoot()
                || correspondingCompilerCommand(other.language()) != other.compilerCommand())) {
        return false;
    }

    return canShareBundleImpl(other);
}

QStringList Toolchain::suggestedMkspecList() const
{
    return {};
}

Id Toolchain::typeId() const
{
    return d->m_typeId;
}

Abis Toolchain::supportedAbis() const
{
    return {targetAbi()};
}

bool Toolchain::isValid() const
{
    if (!d->m_isValid.has_value())
        d->m_isValid = !compilerCommand().isEmpty() && compilerCommand().isExecutableFile();

    return d->m_isValid.value_or(false);
}

FilePaths Toolchain::includedFiles(const QStringList &flags, const FilePath &directory) const
{
    Q_UNUSED(flags)
    Q_UNUSED(directory)
    return {};
}

Id Toolchain::language() const
{
    return d->m_language;
}

bool Toolchain::operator == (const Toolchain &tc) const
{
    if (this == &tc)
        return true;

    // We ignore displayname
    return typeId() == tc.typeId()
            && isAutoDetected() == tc.isAutoDetected()
            && language() == tc.language();
}

Toolchain *Toolchain::clone() const
{
    if (ToolchainFactory * const f = factory()) {
        Toolchain *tc = f->create();
        QTC_ASSERT(tc, return nullptr);
        Store data;
        toMap(data);
        tc->fromMap(data);
        // New ID for the clone. It's different.
        tc->d->m_id = QUuid::createUuid().toByteArray();
        return tc;
    }
    QTC_CHECK(false);
    return nullptr;
}

/*!
    Used by the tool chain manager to save user-generated tool chains.

    Make sure to call this function when deriving.
*/

void Toolchain::toMap(Store &result) const
{
    AspectContainer::toMap(result);

    QString idToSave = d->m_typeId.toString() + QLatin1Char(':') + QString::fromUtf8(id());
    result.insert(ID_KEY, idToSave);
    result.insert(BUNDLE_ID_KEY, d->m_bundleId.toSetting());
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

void Toolchain::toolChainUpdated()
{
    d->m_predefinedMacrosCache->invalidate();
    d->m_headerPathsCache->invalidate();

    ToolchainManager::notifyAboutUpdate(this);
}

void Toolchain::setDetection(Toolchain::Detection de)
{
    d->m_detection = de;
}

void Toolchain::setDetectionSource(const QString &source)
{
    d->m_detectionSource = source;
}

QString Toolchain::typeDisplayName() const
{
    return d->m_typeDisplayName;
}

Abi Toolchain::targetAbi() const
{
    return d->m_targetAbi;
}

void Toolchain::setTargetAbi(const Abi &abi)
{
    if (abi == d->m_targetAbi)
        return;

    d->m_targetAbi = abi;
    toolChainUpdated();
}

void Toolchain::setTargetAbiNoSignal(const Abi &abi)
{
    d->m_targetAbi = abi;
}

void Toolchain::setTargetAbiKey(const Key &abiKey)
{
    d->m_targetAbiKey = abiKey;
}

FilePath Toolchain::compilerCommand() const
{
    return d->m_compilerCommand;
}

void Toolchain::setCompilerCommand(const FilePath &command)
{
    d->m_isValid.reset();

    if (command == d->m_compilerCommand)
        return;
    d->m_compilerCommand = command;
    toolChainUpdated();
}

bool Toolchain::matchesCompilerCommand(const FilePath &command) const
{
    return compilerCommand().isSameExecutable(command);
}

void Toolchain::setCompilerCommandKey(const Key &commandKey)
{
    d->m_compilerCommandKey = commandKey;
}

void Toolchain::setTypeDisplayName(const QString &typeName)
{
    d->m_typeDisplayName = typeName;
}

/*!
    Used by the tool chain manager to load user-generated tool chains.

    Make sure to call this function when deriving.
*/

void Toolchain::fromMap(const Store &data)
{
    AspectContainer::fromMap(data);

    d->m_displayName = data.value(DISPLAY_NAME_KEY).toString();

    // make sure we have new style ids:
    const QString id = data.value(ID_KEY).toString();
    int pos = id.indexOf(QLatin1Char(':'));
    QTC_ASSERT(pos > 0, reportError(); return);
    d->m_typeId = Id::fromString(id.left(pos));
    d->m_id = id.mid(pos + 1).toUtf8();
    d->m_bundleId = Id::fromSetting(data.value(BUNDLE_ID_KEY));

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

void Toolchain::reportError()
{
    d->m_hasError = true;
}

bool Toolchain::hasError() const
{
    return d->m_hasError;
}

const Toolchain::HeaderPathsCache &Toolchain::headerPathsCache() const
{
    return d->m_headerPathsCache;
}

const Toolchain::MacrosCache &Toolchain::predefinedMacrosCache() const
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

LanguageVersion Toolchain::cxxLanguageVersion(const QByteArray &cplusplusMacroValue)
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

LanguageVersion Toolchain::languageVersion(const Id &language, const Macros &macros)
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

FilePath Toolchain::correspondingCompilerCommand(Utils::Id otherLanguage) const
{
    QTC_ASSERT(language() != otherLanguage, return compilerCommand());
    return factory()->correspondingCompilerCommand(compilerCommand(), otherLanguage);
}

FilePaths Toolchain::includedFiles(const QString &option,
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

Tasks Toolchain::validateKit(const Kit *) const
{
    return {};
}

QString Toolchain::sysRoot() const
{
    return {};
}

QString Toolchain::explicitCodeModelTargetTriple() const
{
    return d->m_explicitCodeModelTargetTriple;
}

QString Toolchain::effectiveCodeModelTargetTriple() const
{
    const QString overridden = explicitCodeModelTargetTriple();
    if (!overridden.isEmpty())
        return overridden;
    return originalTargetTriple();
}

void Toolchain::setExplicitCodeModelTargetTriple(const QString &triple)
{
    d->m_explicitCodeModelTargetTriple = triple;
}

/*!
    \class ProjectExplorer::ToolchainFactory
    \brief The ToolchainFactory class creates tool chains from settings or
    autodetects them.
*/

/*!
    \fn QString ProjectExplorer::ToolchainFactory::displayName() const = 0
    Contains the name used to display the name of the tool chain that will be
    created.
*/

/*!
    \fn QStringList ProjectExplorer::Toolchain::clangParserFlags(const QStringList &cxxflags) const = 0
    Converts tool chain specific flags to list flags that tune the libclang
    parser.
*/

/*!
    \fn bool ProjectExplorer::ToolchainFactory::canRestore(const Store &data)
    Used by the tool chain manager to restore user-generated tool chains.
*/

ToolchainFactory::ToolchainFactory()
{
    toolchainFactories().append(this);
}

ToolchainFactory::~ToolchainFactory()
{
    toolchainFactories().removeOne(this);
}

const QList<ToolchainFactory *> ToolchainFactory::allToolchainFactories()
{
    return toolchainFactories();
}

ToolchainFactory *ToolchainFactory::factoryForType(Id typeId)
{
    return Utils::findOrDefault(allToolchainFactories(), [typeId](ToolchainFactory *factory) {
        return factory->supportedToolchainType() == typeId;
    });
}

Toolchains ToolchainFactory::autoDetect(const ToolchainDetector &detector) const
{
    Q_UNUSED(detector)
    return {};
}

Toolchains ToolchainFactory::detectForImport(const ToolchainDescription &tcd) const
{
    Q_UNUSED(tcd)
    return {};
}

FilePath ToolchainFactory::correspondingCompilerCommand(
    const Utils::FilePath &srcPath, Utils::Id targetLang) const
{
    Q_UNUSED(targetLang)
    return srcPath;
}

bool ToolchainFactory::canCreate() const
{
    return m_userCreatable;
}

Toolchain *ToolchainFactory::create() const
{
    return m_toolchainConstructor ? m_toolchainConstructor() : nullptr;
}

Toolchain *ToolchainFactory::restore(const Store &data)
{
    if (!m_toolchainConstructor)
        return nullptr;

    Toolchain *tc = m_toolchainConstructor();
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

QByteArray ToolchainFactory::idFromMap(const Store &data)
{
    return rawIdData(data).second.toUtf8();
}

Id ToolchainFactory::typeIdFromMap(const Store &data)
{
    return Id::fromString(rawIdData(data).first);
}

void ToolchainFactory::autoDetectionToMap(Store &data, bool detected)
{
    data.insert(AUTODETECT_KEY, detected);
}

Toolchain *ToolchainFactory::createToolchain(Id toolchainType)
{
    for (ToolchainFactory *factory : std::as_const(toolchainFactories())) {
        if (factory->m_supportedToolchainType == toolchainType) {
            if (Toolchain *tc = factory->create()) {
                tc->d->m_typeId = toolchainType;
                return tc;
            }
        }
    }
    return nullptr;
}

QList<Id> ToolchainFactory::supportedLanguages() const
{
    return m_supportedLanguages;
}

LanguageCategory ToolchainFactory::languageCategory() const
{
    const QList<Id> langs = supportedLanguages();
    if (langs.size() == 1
        && (langs.first() == Constants::C_LANGUAGE_ID
            || langs.first() == Constants::CXX_LANGUAGE_ID)) {
        return {Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID};
    }
    return LanguageCategory(langs.cbegin(), langs.cend());
}

Id ToolchainFactory::supportedToolchainType() const
{
    return m_supportedToolchainType;
}

std::optional<AsyncToolchainDetector> ToolchainFactory::asyncAutoDetector(
    const ToolchainDetector &) const
{
    return {};
}

void ToolchainFactory::setSupportedToolchainType(const Id &supportedToolchainType)
{
    m_supportedToolchainType = supportedToolchainType;
}

void ToolchainFactory::setSupportedLanguages(const QList<Id> &supportedLanguages)
{
    m_supportedLanguages = supportedLanguages;
}

void ToolchainFactory::setToolchainConstructor(const ToolchainConstructor &toolchainContructor)
{
    m_toolchainConstructor = toolchainContructor;
}

ToolchainFactory::ToolchainConstructor ToolchainFactory::toolchainConstructor() const
{
    return m_toolchainConstructor;
}

void ToolchainFactory::setUserCreatable(bool userCreatable)
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

AsyncToolchainDetector::AsyncToolchainDetector(
    const ToolchainDetector &detector,
    const std::function<Toolchains(const ToolchainDetector &)> &func,
    const std::function<bool(const Toolchain *, const Toolchains &)> &alreadyRegistered)
    : m_detector(detector)
    , m_func(func)
    , m_alreadyRegistered(alreadyRegistered)
{
}

void AsyncToolchainDetector::run()
{
    auto watcher = new QFutureWatcher<Toolchains>();
    QObject::connect(watcher,
                     &QFutureWatcher<Toolchains>::finished,
                     [watcher,
                      alreadyRegistered = m_alreadyRegistered]() {
                         Toolchains existingTcs = ToolchainManager::toolchains();
                         Toolchains toRegister;
                         for (Toolchain *tc : watcher->result()) {
                             if (tc->isValid() && !alreadyRegistered(tc, existingTcs)) {
                                 toRegister << tc;
                                 existingTcs << tc;
                             } else {
                                 delete tc;
                             }
                         }
                         ToolchainManager::registerToolchains(toRegister);
                         watcher->deleteLater();
                     });
    watcher->setFuture(Utils::asyncRun(m_func, m_detector));
}

/*
 * PRE:
 *   - The list of toolchains is not empty.
 *   - All toolchains in the list have the same bundle id.
 *   - All toolchains in the list have the same type (and thus were created by the same factory)
 *     and are also otherwise "compatible" (target ABI etc).
 *   - No two toolchains in the list are for the same language.
 * POST:
 *   - PRE.
 *   - There is exactly one toolchain in the list for every language supported by the factory.
 *   - If there is a C compiler, it comes first in the list.
 */
ToolchainBundle::ToolchainBundle(const Toolchains &toolchains, AutoRegister autoRegister)
    : m_toolchains(toolchains)
{
    // Check pre-conditions.
    QTC_ASSERT(!m_toolchains.isEmpty(), return);
    QTC_ASSERT(m_toolchains.size() <= factory()->supportedLanguages().size(), return);
    for (const Toolchain * const tc : toolchains)
        QTC_ASSERT(factory()->supportedLanguages().contains(tc->language()), return);
    for (int i = 1; i < int(toolchains.size()); ++i) {
        const Toolchain * const tc = toolchains.at(i);
        QTC_ASSERT(tc->typeId() == toolchains.first()->typeId(), return);
        QTC_ASSERT(tc->bundleId() == toolchains.first()->bundleId(), return);
    }

    addMissingToolchains(autoRegister);

    // Check post-conditions.
    QTC_ASSERT(m_toolchains.size() == m_toolchains.first()->factory()->supportedLanguages().size(),
               return);
    for (auto i = toolchains.size(); i < m_toolchains.size(); ++i)
        QTC_ASSERT(m_toolchains.at(i)->typeId() == m_toolchains.first()->typeId(), return);

    Utils::sort(m_toolchains, [](const Toolchain *tc1, const Toolchain *tc2) {
        return tc1 != tc2 && tc1->language() == Constants::C_LANGUAGE_ID;
    });
}

QList<ToolchainBundle> ToolchainBundle::collectBundles(AutoRegister autoRegister)
{
    return collectBundles(ToolchainManager::toolchains(), autoRegister);
}

QList<ToolchainBundle> ToolchainBundle::collectBundles(
    const Toolchains &toolchains, AutoRegister autoRegister)
{
    QHash<Id, Toolchains> toolchainsPerBundleId;
    for (Toolchain * const tc : toolchains)
        toolchainsPerBundleId[tc->bundleId()] << tc;

    QList<ToolchainBundle> bundles;
    if (const auto unbundled = toolchainsPerBundleId.constFind(Id());
        unbundled != toolchainsPerBundleId.constEnd()) {
        bundles = bundleUnbundledToolchains(*unbundled, autoRegister);
        toolchainsPerBundleId.erase(unbundled);
    }

    for (const Toolchains &tcs : toolchainsPerBundleId)
        bundles.emplaceBack(tcs, autoRegister);
    return bundles;
}

ToolchainFactory *ToolchainBundle::factory() const
{
    QTC_ASSERT(!m_toolchains.isEmpty(), return nullptr);
    return m_toolchains.first()->factory();
}

QString ToolchainBundle::displayName() const
{
    if (!isAutoDetected() || !dynamic_cast<GccToolchain *>(m_toolchains.first()))
        return get(&Toolchain::displayName);

    // Auto-detected GCC toolchains encode language and compiler command in their display names.
    // We need to omit the language and we always want to use the C compiler command
    // for consistency.
    FilePath cmd;
    for (const Toolchain * const tc : std::as_const(m_toolchains)) {
        if (!tc->isValid())
            continue;
        cmd = tc->compilerCommand();
        if (tc->language() == Constants::C_LANGUAGE_ID)
            break;
    }

    QString name = typeDisplayName();
    const Abi abi = targetAbi();
    if (abi.architecture() != Abi::UnknownArchitecture)
        name.append(' ').append(Abi::toString(abi.architecture()));
    if (abi.wordWidth() != 0)
        name.append(' ').append(Abi::toString(abi.wordWidth()));
    if (!cmd.exists())
        return name;
    return Tr::tr("%1 at %2").arg(name, cmd.toUserOutput());
}

ToolchainBundle::Valid ToolchainBundle::validity() const
{
    if (Utils::allOf(m_toolchains, &Toolchain::isValid))
        return Valid::All;
    if (Utils::contains(m_toolchains, &Toolchain::isValid))
        return Valid::Some;
    return Valid::None;
}

QList<ToolchainBundle> ToolchainBundle::bundleUnbundledToolchains(
    const Toolchains &unbundled, AutoRegister autoRegister)
{
    QList<ToolchainBundle> bundles;
    QHash<Id, QHash<Id, Toolchains>> unbundledByTypeAndLanguage;
    for (Toolchain * const tc : unbundled)
        unbundledByTypeAndLanguage[tc->typeId()][tc->language()] << tc;
    for (const auto &tcsByLang : std::as_const(unbundledByTypeAndLanguage)) {
        QList<Toolchains> remainingUnbundled;
        for (const auto &tcs : tcsByLang)
            remainingUnbundled << tcs;
        while (true) {
            Toolchains nextBundle;
            for (Toolchains &list : remainingUnbundled) {
                for (auto it = list.begin(); it != list.end(); ++it) {
                    if (nextBundle.isEmpty() || nextBundle.first()->canShareBundle((**it))) {
                        nextBundle << *it;
                        list.erase(it);
                        break;
                    }
                }
            }
            if (nextBundle.isEmpty())
                break;
            const Id newBundleId = Id::generate();
            for (Toolchain * const tc : nextBundle)
                tc->setBundleId(newBundleId);
            bundles.emplaceBack(nextBundle, autoRegister);
        }
    }

    return bundles;
}

void ToolchainBundle::setCompilerCommand(Utils::Id language, const Utils::FilePath &cmd)
{
    for (Toolchain *const tc : std::as_const(m_toolchains)) {
        if (tc->language() == language) {
            tc->setCompilerCommand(cmd);
            break;
        }
    }
}

FilePath ToolchainBundle::compilerCommand(Utils::Id language) const
{
    for (Toolchain *const tc : std::as_const(m_toolchains)) {
        if (tc->language() == language)
            return tc->compilerCommand();
    }
    return {};
}

void ToolchainBundle::deleteToolchains()
{
    qDeleteAll(m_toolchains);
    m_toolchains.clear();
}

ToolchainBundle ToolchainBundle::clone() const
{
    const Toolchains clones = Utils::transform(m_toolchains, &Toolchain::clone);
    const Id newBundleId = Id::generate();
    for (Toolchain * const tc : clones)
        tc->setBundleId(newBundleId);
    return ToolchainBundle(clones, ToolchainBundle::AutoRegister::NotApplicable);
}

void ToolchainBundle::addMissingToolchains(AutoRegister autoRegister)
{
    const QList<Id> missingLanguages
        = Utils::filtered(m_toolchains.first()->factory()->supportedLanguages(), [this](Id lang) {
              return !Utils::contains(m_toolchains, [lang](const Toolchain *tc) {
                  return tc->language() == lang;
              });
          });
    Toolchains createdToolchains;
    for (const Id lang : missingLanguages) {
        Toolchain * const tc = m_toolchains.first()->clone();
        tc->setLanguage(lang);
        tc->setCompilerCommand(m_toolchains.first()->correspondingCompilerCommand(lang));
        m_toolchains << tc;
        createdToolchains << tc;
    }

    switch (autoRegister) {
    case AutoRegister::On:
        ToolchainManager::registerToolchains(createdToolchains);
    case AutoRegister::Off:
        break;
    case AutoRegister::NotApplicable:
        QTC_CHECK(createdToolchains.isEmpty());
        break;
    }
}

} // namespace ProjectExplorer
