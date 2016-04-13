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

#include "jsonwizardfactory.h"

#include "jsonwizard.h"
#include "jsonwizardgeneratorfactory.h"
#include "jsonwizardpagefactory.h"

#include "../projectexplorerconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>

#include <QDebug>
#include <QDir>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUuid>

namespace ProjectExplorer {

static QList<JsonWizardPageFactory *> s_pageFactories;
static QList<JsonWizardGeneratorFactory *> s_generatorFactories;

static const char WIZARD_PATH[] = "templates/wizards";
static const char WIZARD_FILE[] = "wizard.json";

static const char VERSION_KEY[] = "version";
static const char ENABLED_EXPRESSION_KEY[] = "enabled";

static const char KIND_KEY[] = "kind";
static const char SUPPORTED_PROJECTS[] = "supportedProjectTypes";
static const char ID_KEY[] = "id";
static const char CATEGORY_KEY[] = "category";
static const char CATEGORY_NAME_KEY[] = "trDisplayCategory";
static const char DISPLAY_NAME_KEY[] = "trDisplayName";
static const char ICON_KEY[] = "icon";
static const char IMAGE_KEY[] = "image";
static const char DESCRIPTION_KEY[] = "trDescription";
static const char REQUIRED_FEATURES_KEY[] = "featuresRequired";
static const char SUGGESTED_FEATURES_KEY[] = "featuresSuggested";
static const char GENERATOR_KEY[] = "generators";
static const char PAGES_KEY[] = "pages";
static const char TYPE_ID_KEY[] = "typeId";
static const char DATA_KEY[] = "data";
static const char PAGE_SUB_TITLE_KEY[] = "trSubTitle";
static const char PAGE_SHORT_TITLE_KEY[] = "trShortTitle";
static const char PAGE_INDEX_KEY[] = "index";
static const char OPTIONS_KEY[] = "options";
static const char PLATFORM_INDEPENDENT_KEY[] = "platformIndependent";

int JsonWizardFactory::m_verbose = 0;

// Return locale language attribute "de_UTF8" -> "de", empty string for "C"
static QString languageSetting()
{
    QString name = Core::ICore::userInterfaceLanguage();
    const int underScorePos = name.indexOf(QLatin1Char('_'));
    if (underScorePos != -1)
        name.truncate(underScorePos);
    if (name.compare(QLatin1String("C"), Qt::CaseInsensitive) == 0)
        name.clear();
    return name;
}

template<class T>
static QString supportedTypeIds(const QList<T *> &factories)
{
    QStringList tmp;
    foreach (const T*f, factories) {
        foreach (Core::Id i, f->supportedIds())
            tmp.append(i.toString());
    }
    return tmp.join(QLatin1String("', '"));
}

static JsonWizardFactory::Generator parseGenerator(const QVariant &value, QString *errorMessage)
{
    JsonWizardFactory::Generator gen;

    if (value.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizardFactory", "Generator is not a object.");
        return gen;
    }

    QVariantMap data = value.toMap();
    QString strVal = data.value(QLatin1String(TYPE_ID_KEY)).toString();
    if (strVal.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizardFactory", "Generator has no typeId set.");
        return gen;
    }
    Core::Id typeId = Core::Id::fromString(QLatin1String(Constants::GENERATOR_ID_PREFIX) + strVal);
    JsonWizardGeneratorFactory *factory
            = Utils::findOr(s_generatorFactories, 0, [typeId](JsonWizardGeneratorFactory *f) { return f->canCreate(typeId); });
    if (!factory) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizardFactory",
                                                    "TypeId \"%1\" of generator is unknown. Supported typeIds are: \"%2\".")
                .arg(strVal)
                .arg(supportedTypeIds(s_generatorFactories).replace(QLatin1String(Constants::GENERATOR_ID_PREFIX), QLatin1String("")));
        return gen;
    }

    QVariant varVal = data.value(QLatin1String(DATA_KEY));
    if (!factory->validateData(typeId, varVal, errorMessage))
        return gen;

    gen.typeId = typeId;
    gen.data = varVal;

    return gen;
}

static JsonWizardFactory::Page parsePage(const QVariant &value, QString *errorMessage)
{
    JsonWizardFactory::Page p;

    if (value.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizardFactory", "Page is not an object.");
        return p;
    }

    QVariantMap data = value.toMap();
    QString strVal = data.value(QLatin1String(TYPE_ID_KEY)).toString();
    if (strVal.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizardFactory", "Page has no typeId set.");
        return p;
    }
    Core::Id typeId = Core::Id::fromString(QLatin1String(Constants::PAGE_ID_PREFIX) + strVal);

    JsonWizardPageFactory *factory
            = Utils::findOr(s_pageFactories, 0, [typeId](JsonWizardPageFactory *f) { return f->canCreate(typeId); });
    if (!factory) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizardFactory",
                                                    "TypeId \"%1\" of page is unknown. Supported typeIds are: \"%2\".")
                .arg(strVal)
                .arg(supportedTypeIds(s_pageFactories).replace(QLatin1String(Constants::PAGE_ID_PREFIX), QLatin1String("")));
        return p;
    }

    QString title = JsonWizardFactory::localizedString(data.value(QLatin1String(DISPLAY_NAME_KEY)));
    QString subTitle = JsonWizardFactory::localizedString(data.value(QLatin1String(PAGE_SUB_TITLE_KEY)));
    QString shortTitle = JsonWizardFactory::localizedString(data.value(QLatin1String(PAGE_SHORT_TITLE_KEY)));

    bool ok;
    int index = data.value(QLatin1String(PAGE_INDEX_KEY), -1).toInt(&ok);
    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizardFactory", "Page with typeId \"%1\" has invalid \"index\".")
                .arg(typeId.toString());
        return p;
    }

    QVariant enabled = data.value(QLatin1String(ENABLED_EXPRESSION_KEY), true);

    QVariant subData = data.value(QLatin1String(DATA_KEY));
    if (!factory->validateData(typeId, subData, errorMessage))
        return p;

    p.typeId = typeId;
    p.title = title;
    p.subTitle = subTitle;
    p.shortTitle = shortTitle;
    p.index = index;
    p.data = subData;
    p.enabled = enabled;

    return p;
}

QList<Core::IWizardFactory *> JsonWizardFactory::createWizardFactories()
{
    QString errorMessage;
    QString verboseLog;
    const QString wizardFileName = QLatin1String(WIZARD_FILE);

    QList <Core::IWizardFactory *> result;
    foreach (const Utils::FileName &path, searchPaths()) {
        if (path.isEmpty())
            continue;

        QDir dir(path.toString());
        if (!dir.exists()) {
            if (verbose())
                verboseLog.append(tr("Path \"%1\" does not exist when checking Json wizard search paths.\n")
                                  .arg(path.toUserOutput()));
            continue;
        }

        const QDir::Filters filters = QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot;
        const QDir::SortFlags sortflags = QDir::Name|QDir::IgnoreCase;
        QList<QFileInfo> dirs = dir.entryInfoList(filters, sortflags);

        while (!dirs.isEmpty()) {
            const QFileInfo dirFi = dirs.takeFirst();
            const QDir current(dirFi.absoluteFilePath());
            if (verbose())
                verboseLog.append(tr("Checking \"%1\" for %2.\n")
                                  .arg(QDir::toNativeSeparators(current.absolutePath()))
                                  .arg(wizardFileName));
            if (current.exists(wizardFileName)) {
                QFile configFile(current.absoluteFilePath(wizardFileName));
                configFile.open(QIODevice::ReadOnly);
                QJsonParseError error;
                QByteArray fileData = configFile.readAll();
                QJsonDocument json = QJsonDocument::fromJson(fileData, &error);
                configFile.close();

                if (error.error != QJsonParseError::NoError) {
                    int line = 1;
                    int column = 1;
                    for (int i = 0; i < error.offset; ++i) {
                        if (fileData.at(i) == '\n') {
                            ++line;
                            column = 1;
                        } else {
                            ++column;
                        }
                    }
                    verboseLog.append(tr("* Failed to parse \"%1\":%2:%3: %4\n")
                                      .arg(configFile.fileName())
                                      .arg(line).arg(column)
                                      .arg(error.errorString()));
                    continue;
                }

                if (!json.isObject()) {
                    verboseLog.append(tr("* Did not find a JSON object in \"%1\".\n")
                                      .arg(configFile.fileName()));
                    continue;
                }

                if (verbose())
                    verboseLog.append(tr("* Configuration found and parsed.\n"));

                QVariantMap data = json.object().toVariantMap();

                int version = data.value(QLatin1String(VERSION_KEY), 0).toInt();
                if (version < 1 || version > 1) {
                    verboseLog.append(tr("* Version %1 not supported.\n").arg(version));
                    continue;
                }

                JsonWizardFactory *factory = createWizardFactory(data, current, &errorMessage);
                if (!factory) {
                    verboseLog.append(tr("* Failed to create: %1\n").arg(errorMessage));
                    continue;
                }

                result << factory;
            } else {
                QList<QFileInfo> subDirs = current.entryInfoList(filters, sortflags);
                if (!subDirs.isEmpty()) {
                    // There is no QList::prepend(QList)...
                    dirs.swap(subDirs);
                    dirs.append(subDirs);
                } else if (verbose()) {
                    verboseLog.append(tr("JsonWizard: \"%1\" not found\n").arg(wizardFileName));
                }
            }
        }
    }

    if (verbose()) { // Print to output pane for Windows.
        qWarning("%s", qPrintable(verboseLog));
        Core::MessageManager::write(verboseLog, Core::MessageManager::ModeSwitch);
    }

    return result;
}

JsonWizardFactory *JsonWizardFactory::createWizardFactory(const QVariantMap &data, const QDir &baseDir,
                                                          QString *errorMessage)
{
    JsonWizardFactory *factory = new JsonWizardFactory;
    if (!factory->initialize(data, baseDir, errorMessage)) {
        delete factory;
        factory = nullptr;
    }
    return factory;
}

Utils::FileNameList &JsonWizardFactory::searchPaths()
{
    static Utils::FileNameList m_searchPaths = Utils::FileNameList()
            << Utils::FileName::fromString(Core::ICore::userResourcePath() + QLatin1Char('/') +
                                           QLatin1String(WIZARD_PATH))
            << Utils::FileName::fromString(Core::ICore::resourcePath() + QLatin1Char('/') +
                                           QLatin1String(WIZARD_PATH));
    return m_searchPaths;
}

void JsonWizardFactory::addWizardPath(const Utils::FileName &path)
{
    searchPaths().append(path);
}

void JsonWizardFactory::setVerbose(int level)
{
    m_verbose = level;
}

int JsonWizardFactory::verbose()
{
    return m_verbose;
}

void JsonWizardFactory::registerPageFactory(JsonWizardPageFactory *factory)
{
    QTC_ASSERT(!s_pageFactories.contains(factory), return);
    s_pageFactories.append(factory);
}

void JsonWizardFactory::registerGeneratorFactory(JsonWizardGeneratorFactory *factory)
{
    QTC_ASSERT(!s_generatorFactories.contains(factory), return);
    s_generatorFactories.append(factory);
}

Utils::Wizard *JsonWizardFactory::runWizardImpl(const QString &path, QWidget *parent,
                                                Core::Id platform,
                                                const QVariantMap &variables)
{
    auto wizard = new JsonWizard(parent);
    wizard->setWindowIcon(icon());
    wizard->setWindowTitle(displayName());

    wizard->setValue(QStringLiteral("WizardDir"), m_wizardDir);
    QSet<Core::Id> tmp = requiredFeatures();
    tmp.subtract(pluginFeatures());
    wizard->setValue(QStringLiteral("RequiredFeatures"), Core::Id::toStringList(tmp));
    tmp = m_preferredFeatures;
    tmp.subtract(pluginFeatures());
    wizard->setValue(QStringLiteral("PreferredFeatures"), Core::Id::toStringList(tmp));

    wizard->setValue(QStringLiteral("Features"), Core::Id::toStringList(availableFeatures(platform)));
    wizard->setValue(QStringLiteral("Plugins"), Core::Id::toStringList(pluginFeatures()));

    // Add data to wizard:
    for (auto i = variables.constBegin(); i != variables.constEnd(); ++i)
        wizard->setValue(i.key(), i.value());

    wizard->setValue(QStringLiteral("InitialPath"), path);
    wizard->setValue(QStringLiteral("Platform"), platform.toString());

    QString kindStr = QLatin1String(Core::Constants::WIZARD_KIND_UNKNOWN);
    if (kind() == IWizardFactory::FileWizard)
        kindStr = QLatin1String(Core::Constants::WIZARD_KIND_FILE);
    else if (kind() == IWizardFactory::ProjectWizard)
        kindStr = QLatin1String(Core::Constants::WIZARD_KIND_PROJECT);
    wizard->setValue(QStringLiteral("kind"), kindStr);

    wizard->setValue(QStringLiteral("trDescription"), description());
    wizard->setValue(QStringLiteral("trDisplayName"), displayName());
    wizard->setValue(QStringLiteral("trDisplayCategory"), displayCategory());
    wizard->setValue(QStringLiteral("category"), category());
    wizard->setValue(QStringLiteral("id"), id().toString());

    Utils::MacroExpander *expander = wizard->expander();
    foreach (const JsonWizard::OptionDefinition &od, m_options) {
        if (od.condition(*expander))
            wizard->setValue(od.key(), od.value(*expander));
    }

    bool havePage = false;
    foreach (const Page &data, m_pages) {
        QTC_ASSERT(data.isValid(), continue);

        if (!JsonWizard::boolFromVariant(data.enabled, wizard->expander()))
            continue;

        havePage = true;
        JsonWizardPageFactory *factory = Utils::findOr(s_pageFactories, 0,
                                                       [&data](JsonWizardPageFactory *f) {
                                                            return f->canCreate(data.typeId);
                                                       });
        QTC_ASSERT(factory, continue);
        Utils::WizardPage *page = factory->create(wizard, data.typeId, data.data);
        QTC_ASSERT(page, continue);

        page->setTitle(data.title);
        page->setSubTitle(data.subTitle);
        page->setProperty(Utils::SHORT_TITLE_PROPERTY, data.shortTitle);

        if (data.index >= 0) {
            wizard->setPage(data.index, page);
            if (wizard->page(data.index) != page) // Failed to set page!
                delete page;
        } else {
            wizard->addPage(page);
        }
    }

    foreach (const Generator &data, m_generators) {
        QTC_ASSERT(data.isValid(), continue);
        JsonWizardGeneratorFactory *factory = Utils::findOr(s_generatorFactories, 0,
                                                            [&data](JsonWizardGeneratorFactory *f) {
                                                                 return f->canCreate(data.typeId);
                                                            });
        QTC_ASSERT(factory, continue);
        JsonWizardGenerator *gen = factory->create(data.typeId, data.data, path, platform, variables);
        QTC_ASSERT(gen, continue);

        wizard->addGenerator(gen);
    }

    if (!havePage) {
        wizard->accept();
        wizard->deleteLater();
        return 0;
    }

    wizard->show();
    return wizard;
}

QList<QVariant> JsonWizardFactory::objectOrList(const QVariant &data, QString *errorMessage)
{
    QList<QVariant> result;
    if (data.isNull())
        *errorMessage = tr("key not found.");
    else if (data.type() == QVariant::Map)
        result.append(data);
    else if (data.type() == QVariant::List)
        result = data.toList();
    else
        *errorMessage = tr("Expected an object or a list.");
    return result;
}

QString JsonWizardFactory::localizedString(const QVariant &value)
{
    if (value.isNull())
        return QString();
    if (value.type() == QVariant::Map) {
        QVariantMap tmp = value.toMap();
        const QString locale = languageSetting().toLower();
        QStringList locales;
        locales << locale << QLatin1String("en") << QLatin1String("C") << tmp.keys();
        foreach (const QString &locale, locales) {
            QString result = tmp.value(locale, QString()).toString();
            if (!result.isEmpty())
                return result;
        }
        return QString();
    }
    return QCoreApplication::translate("ProjectExplorer::JsonWizard", value.toByteArray());
}

bool JsonWizardFactory::isAvailable(Core::Id platformId) const
{
    if (!IWizardFactory::isAvailable(platformId)) // check for required features
        return false;

    Utils::MacroExpander expander;
    Utils::MacroExpander *e = &expander;
    expander.registerVariable("Platform", tr("The platform selected for the wizard."),
                              [platformId]() { return platformId.toString(); });
    expander.registerVariable("Features", tr("The features available to this wizard."),
                              [this, e, platformId]() { return JsonWizard::stringListToArrayString(Core::Id::toStringList(availableFeatures(platformId)), e); });
    expander.registerVariable("Plugins", tr("The plugins loaded."),
                              [this, e]() { return JsonWizard::stringListToArrayString(Core::Id::toStringList(pluginFeatures()), e); });

    return JsonWizard::boolFromVariant(m_enabledExpression, &expander);
}

void JsonWizardFactory::destroyAllFactories()
{
    qDeleteAll(s_pageFactories);
    s_pageFactories.clear();
    qDeleteAll(s_generatorFactories);
    s_generatorFactories.clear();
}

bool JsonWizardFactory::initialize(const QVariantMap &data, const QDir &baseDir, QString *errorMessage)
{
    QTC_ASSERT(errorMessage, return false);

    errorMessage->clear();

    m_wizardDir = baseDir.absolutePath();

    m_enabledExpression = data.value(QLatin1String(ENABLED_EXPRESSION_KEY), true);

    QSet<Core::Id> projectTypes = Core::Id::fromStringList(data.value(QLatin1String(SUPPORTED_PROJECTS)).toStringList());
    // FIXME: "kind" was relevant up to and including Qt Creator 3.6:
    const QString unsetKind = QUuid::createUuid().toString();
    QString strVal = data.value(QLatin1String(KIND_KEY), unsetKind).toString();
    if (strVal != unsetKind
            && strVal != QLatin1String("class")
            && strVal != QLatin1String("file")
            && strVal != QLatin1String("project")) {
        *errorMessage = tr("\"kind\" value \"%1\" is not \"class\" (deprecated), \"file\" or \"project\".").arg(strVal);
        return false;
    }
    if ((strVal == QLatin1String("file") || strVal == QLatin1String("class")) && !projectTypes.isEmpty()) {
        *errorMessage = tr("\"kind\" is \"file\" or \"class\" (deprecated) and \"%1\" is also set.").arg(QLatin1String(SUPPORTED_PROJECTS));
        return false;
    }
    if (strVal == QLatin1String("project") && projectTypes.isEmpty())
        projectTypes.insert("UNKNOWN_PROJECT");
    // end of legacy code
    setSupportedProjectTypes(projectTypes);

    strVal = data.value(QLatin1String(ID_KEY)).toString();
    if (strVal.isEmpty()) {
        *errorMessage = tr("No id set.");
        return false;
    }
    setId(Core::Id::fromString(strVal));

    strVal = data.value(QLatin1String(CATEGORY_KEY)).toString();
    if (strVal.isEmpty()) {
        *errorMessage = tr("No category is set.");
        return false;
    }
    setCategory(strVal);

    strVal = data.value(QLatin1String(ICON_KEY)).toString();
    if (!strVal.isEmpty()) {
        strVal = baseDir.absoluteFilePath(strVal);
        if (!QFileInfo::exists(strVal)) {
            *errorMessage = tr("Icon file \"%1\" not found.").arg(QDir::toNativeSeparators(strVal));
            return false;
        }
        setIcon(QIcon(strVal));
    }

    strVal = data.value(QLatin1String(IMAGE_KEY)).toString();
    if (!strVal.isEmpty()) {
        strVal = baseDir.absoluteFilePath(strVal);
        if (!QFileInfo::exists(strVal)) {
            *errorMessage = tr("Image file \"%1\" not found.").arg(QDir::toNativeSeparators(strVal));
            return false;
        }
        setDescriptionImage(strVal);
    }

    setRequiredFeatures(Core::Id::fromStringList(data.value(QLatin1String(REQUIRED_FEATURES_KEY)).toStringList()));
    m_preferredFeatures = Core::Id::fromStringList(data.value(QLatin1String(SUGGESTED_FEATURES_KEY)).toStringList());
    m_preferredFeatures.unite(requiredFeatures());

    strVal = localizedString(data.value(QLatin1String(DISPLAY_NAME_KEY)));
    if (strVal.isEmpty()) {
        *errorMessage = tr("No displayName set.");
        return false;
    }
    setDisplayName(strVal);

    strVal = localizedString(data.value(QLatin1String(CATEGORY_NAME_KEY)));
    if (strVal.isEmpty()) {
        *errorMessage = tr("No displayCategory set.");
        return false;
    }
    setDisplayCategory(strVal);

    strVal = localizedString(data.value(QLatin1String(DESCRIPTION_KEY)));
    if (strVal.isEmpty()) {
        *errorMessage = tr("No description set.");
        return false;
    }
    setDescription(strVal);

    // Generator:
    QVariantList list = objectOrList(data.value(QLatin1String(GENERATOR_KEY)), errorMessage);
    if (!errorMessage->isEmpty()) {
        *errorMessage = tr("When parsing \"generators\": %1").arg(*errorMessage);
        return false;
    }

    foreach (const QVariant &v, list) {
        Generator gen = parseGenerator(v, errorMessage);
        if (gen.isValid())
            m_generators.append(gen);
        else
            return false;
    }

    // Pages:
    list = objectOrList(data.value(QLatin1String(PAGES_KEY)), errorMessage);
    if (!errorMessage->isEmpty()) {
        *errorMessage = tr("When parsing \"pages\": %1").arg(*errorMessage);
        return false;
    }

    foreach (const QVariant &v, list) {
        Page p = parsePage(v, errorMessage);
        if (p.isValid())
            m_pages.append(p);
        else
            return false;
    }

    WizardFlags flags = 0;
    if (data.value(QLatin1String(PLATFORM_INDEPENDENT_KEY), false).toBool())
        flags |= PlatformIndependent;
    setFlags(flags);

    // Options:
    m_options = JsonWizard::parseOptions(data.value(QLatin1String(OPTIONS_KEY)), errorMessage);
    return errorMessage->isEmpty();
}

} // namespace ProjectExplorer
