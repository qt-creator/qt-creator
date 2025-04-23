// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dsstore.h"
#include "dsthememanager.h"

#include <generatedcomponentutils.h>
#include <import.h>
#include <plaintexteditmodifier.h>
#include <qmljs/parser/qmldirparser_p.h>
#include <qmljs/qmljsreformatter.h>
#include <rewriterview.h>
#include <rewritingexception.h>
#include <uniquename.h>
#include <utils/fileutils.h>

#include <QLoggingCategory>
#include <QPlainTextEdit>
#include <QScopeGuard>

using namespace Utils;

namespace {
Q_LOGGING_CATEGORY(dsLog, "qtc.designer.designSystem", QtInfoMsg)
constexpr char DesignModuleName[] = "DesignSystem";

std::optional<Utils::FilePath> dsModuleDir(QmlDesigner::ExternalDependenciesInterface &ed)
{
    auto componentsPath = QmlDesigner::GeneratedComponentUtils(ed).generatedComponentsPath();
    if (componentsPath.exists())
        return componentsPath.pathAppended(DesignModuleName);

    return {};
}

static Utils::FilePath dsFilePath(const Utils::FilePath &targetDir, const QString &typeName)
{
    return targetDir / (typeName + ".qml");
}

static QString themeInterfaceType(const QString &typeName)
{
    return QString("%1Theme").arg(typeName);
}

static QByteArray reformatQml(const QString &content)
{
    auto document = QmlJS::Document::create({}, QmlJS::Dialect::QmlQtQuick2);
    document->setSource(content);
    document->parseQml();
    if (document->isParsedCorrectly())
        return QmlJS::reformat(document).toUtf8();

    return content.toUtf8();
}

std::optional<QString> modelSerializeHelper(
    [[maybe_unused]] QmlDesigner::ProjectStorageDependencies &projectStorageDependencies,
    QmlDesigner::ExternalDependenciesInterface &ed,
    std::function<void(QmlDesigner::Model *)> callback,
    const FilePath &targetDir,
    const QString &typeName,
    bool isSingelton = false)
{
    QString qmlText{"import QtQuick\nQtObject {}\n"};
    if (isSingelton)
        qmlText.prepend("pragma Singleton\n");

#ifdef QDS_USE_PROJECTSTORAGE
    auto model = QmlDesigner::Model::create(projectStorageDependencies,
                                            "QtObject",
                                            {QmlDesigner::Import::createLibraryImport("QtQtuick")},
                                            QUrl::fromLocalFile(
                                                "/path/dummy.qml")); // the dummy file will most probably not work
#else
    auto model = QmlDesigner::Model::create("QtObject");
#endif

    QPlainTextEdit editor;
    editor.setPlainText(qmlText);
    QmlDesigner::NotIndentingTextEditModifier modifier(editor.document());
    QmlDesigner::RewriterView view(ed, QmlDesigner::RewriterView::Validate);
    view.setPossibleImportsEnabled(false);
    view.setCheckSemanticErrors(false);
    view.setTextModifier(&modifier);
    model->attachView(&view);

    view.executeInTransaction("DSStore::modelSerializeHelper", [&] { callback(model.get()); });

    FileSaver saver(dsFilePath(targetDir, typeName), QIODevice::Text);
    saver.write(reformatQml(modifier.text()));
    if (const Result<> res = saver.finalize(); !res)
        return res.error();

    return {};
}

std::optional<std::tuple<QStringView, QStringView, QStringView>> unpackDSBinding(QStringView binding)
{
    const auto parts = binding.split('.', Qt::SkipEmptyParts);
    if (parts.size() != 3)
        return {};

    return std::make_tuple(parts[0], parts[1], parts[2]);
}

} // namespace

namespace QmlDesigner {

DSStore::DSStore(ExternalDependenciesInterface &ed,
                 ProjectStorageDependencies projectStorageDependencies)
    : m_ed(ed)
    , m_projectStorageDependencies(projectStorageDependencies)
{}

DSStore::~DSStore() {}

QString DSStore::moduleImportStr() const
{
    auto prefix = GeneratedComponentUtils(m_ed).generatedComponentTypePrefix();
    if (!prefix.isEmpty())
        return QString("%1.%2").arg(prefix).arg(DesignModuleName);

    return DesignModuleName;
}

std::optional<QString> DSStore::load()
{
    if (auto moduleDir = dsModuleDir(m_ed))
        return load(*moduleDir);

    return tr("Can not locate design system module");
}

std::optional<QString> DSStore::load(const Utils::FilePath &dsModuleDirPath)
{
    if (m_blockLoading)
        return {};

    m_collections.clear();

    // read qmldir
    const auto qmldirFile = dsModuleDirPath / "qmldir";
    const Utils::Result<QByteArray> contents = qmldirFile.fileContents();
    if (!contents)
        return tr("Can not read Design System qmldir");

    // Parse qmldir
    QString qmldirData = QString::fromUtf8(*contents);
    QmlDirParser qmlDirParser;
    qmlDirParser.parse(qmldirData);

    // load collections
    QStringList collectionErrors;
    auto addCollectionErr = [&collectionErrors](const QString &name, const QString &e) {
        collectionErrors << QString("Error loading collection %1. %2").arg(name, e);
    };
    for (auto component : qmlDirParser.components()) {
        if (!component.fileName.isEmpty()) {
            const auto collectionPath = dsModuleDirPath.pathAppended(component.fileName);
            if (auto err = loadCollection(component.typeName, collectionPath))
                addCollectionErr(component.typeName, *err);
        } else {
            addCollectionErr(component.typeName, tr("Can not find component file."));
        }
    }

    if (!collectionErrors.isEmpty())
        return collectionErrors.join("\n");

    return {};
}

std::optional<QString> DSStore::save(bool mcuCompatible)
{
    if (auto moduleDir = dsModuleDir(m_ed))
        return save(*moduleDir, mcuCompatible);

    return tr("Can not locate design system module");
}

std::optional<QString> DSStore::save(const FilePath &moduleDirPath, bool mcuCompatible)
{
    if (!QDir().mkpath(moduleDirPath.absoluteFilePath().toUrlishString()))
        return tr("Can not create design system module directory %1.").arg(moduleDirPath.toUrlishString());

    const QScopeGuard cleanup([&] { m_blockLoading = false; });
    m_blockLoading = true;

    // dump collections
    QStringList singletons;
    QStringList errors;
    for (auto &[typeName, collection] : m_collections) {
        if (auto err = writeQml(collection, typeName, moduleDirPath, mcuCompatible))
            errors << *err;
        singletons << QString("singleton %1 1.0 %1.qml").arg(typeName);
    }

    // Write qmldir
    FileSaver saver(moduleDirPath / "qmldir", QIODevice::Text);
    const QString qmldirContents = QString("Module %1\n%2").arg(moduleImportStr(), singletons.join("\n"));
    saver.write(qmldirContents.toUtf8());
    if (const Result<> res = saver.finalize(); !res)
        errors << tr("Can not write design system qmldir. %1").arg(res.error());

    if (!errors.isEmpty())
        return errors.join("\n");

    return {};
}

DSThemeManager *DSStore::addCollection(const QString &qmlTypeName)
{
    const QString componentType = uniqueCollectionName(qmlTypeName);

    auto [itr, success] = m_collections.try_emplace(componentType);
    if (success)
        return &itr->second;

    return nullptr;
}

std::optional<QString> DSStore::typeName(DSThemeManager *collection) const
{
    auto result = std::find_if(m_collections.cbegin(),
                               m_collections.cend(),
                               [collection](const auto &itr) { return &itr.second == collection; });

    if (result != m_collections.cend())
        return result->first;

    return {};
}

bool DSStore::removeCollection(const QString &name)
{
    if (auto toRemove = collection(name)) {
        for (auto &[_, currentCollection] : m_collections) {
            if (toRemove == &currentCollection)
                continue;
            breakBindings(&currentCollection, name);
        }
        save();
        removeCollectionFiles(name);
        return m_collections.erase(name);
    }
    return false;
}

bool DSStore::renameCollection(const QString &oldName, const QString &newName)
{
    auto itr = m_collections.find(oldName);
    if (itr == m_collections.end() || oldName == newName)
        return false;

    const QString uniqueTypeName = uniqueCollectionName(newName);

    // newName is mutated to make it unique or compatible. Bail.
    // Case update is tolerated.
    if (uniqueTypeName.toLower() != newName.toLower())
        return false;

    auto handle = m_collections.extract(oldName);
    handle.key() = uniqueTypeName;
    m_collections.insert(std::move(handle));

    refactorBindings(oldName, uniqueTypeName);
    save();
    removeCollectionFiles(oldName);
    return true;
}

std::optional<Utils::FilePath> DSStore::moduleDirPath() const
{
    return dsModuleDir(m_ed);
}

QStringList DSStore::collectionNames() const
{
    QStringList names;
    std::transform(m_collections.begin(),
                   m_collections.end(),
                   std::back_inserter(names),
                   [](const DSCollections::value_type &p) { return p.first; });
    return names;
}

std::optional<ThemeProperty> DSStore::resolvedDSBinding(QStringView binding,
                                                        std::optional<CollectionBinding> avoidValue) const
{
    if (auto parts = unpackDSBinding(binding)) {
        auto &[collectionName, groupId, propertyName] = *parts;
        return resolvedDSBinding({collectionName, propertyName}, groupId, avoidValue);
    }

    qCDebug(dsLog) << "Resolving binding failed. Unexpected binding" << binding;
    return {};
}

void DSStore::refactorBindings(QStringView oldCollectionName, QStringView newCollectionName)
{
    for (auto &[_, currentCollection] : m_collections) {
        for (const auto &[propName, themeId, gt, expression] : currentCollection.boundProperties()) {
            auto bindingParts = unpackDSBinding(expression);
            if (!bindingParts) {
                qCDebug(dsLog) << "Refactor binding error. Unexpected binding" << expression;
                continue;
            }

            const auto &[boundCollection, groupName, boundProp] = *bindingParts;
            if (boundCollection != oldCollectionName)
                continue;

            const auto newBinding = QString("%1.%2.%3").arg(newCollectionName, groupName, boundProp);
            currentCollection.updateProperty(themeId, gt, {propName, newBinding, true});
        }
    }
}

void DSStore::refactorBindings(DSThemeManager *srcCollection, PropertyName from, PropertyName to)
{
    auto srcCollectionName = typeName(srcCollection);
    if (!srcCollectionName)
        return;

    for (auto &[_, currentCollection] : m_collections) {
        for (const auto &[propName, themeId, gt, expression] : currentCollection.boundProperties()) {
            auto bindingParts = unpackDSBinding(expression);
            if (!bindingParts) {
                qCDebug(dsLog) << "Refactor binding error. Unexpected binding" << expression;
                continue;
            }

            const auto &[boundCollection, groupName, boundProp] = *bindingParts;
            if (boundCollection != srcCollectionName || from != boundProp.toLatin1())
                continue;

            const auto newBinding = QString("%1.%2.%3")
                                        .arg(boundCollection, groupName, QString::fromUtf8(to));
            currentCollection.updateProperty(themeId, gt, {propName, newBinding, true});
        }
    }
}

void DSStore::breakBindings(DSThemeManager *collection, PropertyName propertyName)
{
    auto collectionName = typeName(collection);
    if (!collectionName)
        return;

    for (auto &[_, currentCollection] : m_collections) {
        for (const auto &[propName, themeId, gt, expression] : currentCollection.boundProperties()) {
            auto bindingParts = unpackDSBinding(expression);
            if (!bindingParts) {
                qCDebug(dsLog) << "Error breaking binding. Unexpected binding" << expression;
                continue;
            }
            const auto &[boundCollection, groupId, boundProp] = *bindingParts;
            if (boundCollection != collectionName || propertyName != boundProp.toLatin1())
                continue;

            if (auto value = resolvedDSBinding({*collectionName, boundProp}, groupId))
                currentCollection.updateProperty(themeId, gt, {propName, value->value});
        }
    }
}

void DSStore::breakBindings(DSThemeManager *collection, QStringView removeCollection)
{
    for (const auto &[propName, themeId, gt, expression] : collection->boundProperties()) {
        auto bindingParts = unpackDSBinding(expression);
        if (!bindingParts) {
            qCDebug(dsLog) << "Error breaking binding. Unexpected binding" << expression;
            continue;
        }

        const auto &[boundCollection, groupId, boundProp] = *bindingParts;
        if (boundCollection != removeCollection)
            continue;

        if (auto value = resolvedDSBinding({boundCollection, boundProp}, groupId))
            collection->updateProperty(themeId, gt, {propName, value->value});
    }
}

QString DSStore::uniqueCollectionName(const QString &hint) const
{
    return UniqueName::generateTypeName(hint, "Collection", [this](const QString &t) {
        return m_collections.contains(t);
    });
}

std::optional<ThemeProperty> DSStore::boundProperty(const CollectionBinding &binding,
                                                    QStringView groupId) const
{
    auto bindingGroupType = groupIdToGroupType(groupId.toLatin1());
    if (!bindingGroupType)
        return {};

    auto itr = m_collections.find(binding.collection.toString());
    if (itr != m_collections.end()) {
        const DSThemeManager &boundCollection = itr->second;
        const auto propertyName = binding.propName.toLatin1();
        if (const auto group = boundCollection.groupType(propertyName)) {
            if (group != *bindingGroupType)
                return {}; // Found property has a different group.

            return boundCollection.property(boundCollection.activeTheme(), *group, propertyName);
        }
    }
    return {};
}

std::optional<ThemeProperty> DSStore::resolvedDSBinding(CollectionBinding binding,
                                                        QStringView groupId,
                                                        std::optional<CollectionBinding> avoidValue) const
{
    std::unordered_set<QStringView> visited;
    const auto hasCycle = [&visited](const CollectionBinding &binding) {
        // Return true if the dsBinding token exists, insert otherwise.
        const auto token = QString("%1.%2").arg(binding.collection, binding.propName);
        return !visited.emplace(token).second;
    };

    if (avoidValue) // Insert the extra binding for cycle detection
        hasCycle(*avoidValue);

    std::optional<ThemeProperty> resolvedProperty;
    do {
        if (hasCycle(binding)) {
            qCDebug(dsLog) << "Cyclic binding";
            resolvedProperty = {};
        } else {
            resolvedProperty = boundProperty(binding, groupId);
        }

        if (resolvedProperty && resolvedProperty->isBinding) {
            // The value is again a binding.
            if (auto bindingParts = unpackDSBinding(resolvedProperty->value.toString())) {
                std::tie(binding.collection, std::ignore, binding.propName) = *bindingParts;
            } else {
                qCDebug(dsLog) << "Invalid binding" << resolvedProperty->value.toString();
                resolvedProperty = {};
            }
        }
    } while (resolvedProperty && resolvedProperty->isBinding);

    if (!resolvedProperty)
        qCDebug(dsLog) << "Can not resolve binding." << binding.collection << binding.propName;

    return resolvedProperty;
}

DSThemeManager *DSStore::collection(const QString &typeName)
{
    auto itr = m_collections.find(typeName);
    if (itr != m_collections.end())
        return &itr->second;

    return nullptr;
}

std::optional<QString> DSStore::loadCollection(const QString &typeName,
                                               const Utils::FilePath &qmlFilePath)
{
    const Utils::Result<QByteArray> res = qmlFilePath.fileContents();
    if (!res)
        return res.error();

#ifdef QDS_USE_PROJECTSTORAGE
    auto model = QmlDesigner::Model::create(m_projectStorageDependencies,
                                            "QtObject",
                                            {QmlDesigner::Import::createLibraryImport("QtQtuick")},
                                            QUrl::fromLocalFile(
                                                "/path/dummy.qml")); // the dummy file will most probably not work
#else
    auto model = QmlDesigner::Model::create("QtObject");
#endif
    QPlainTextEdit editor;
    QString qmlContent = QString::fromUtf8(*res);
    editor.setPlainText(qmlContent);

    QmlDesigner::NotIndentingTextEditModifier modifier(editor.document());
    RewriterView view(m_ed, QmlDesigner::RewriterView::Validate);
    // QDS-8366
    view.setPossibleImportsEnabled(false);
    view.setCheckSemanticErrors(false);
    view.setTextModifier(&modifier);
    model->attachView(&view);

    if (auto dsMgr = addCollection(typeName))
        return dsMgr->load(model->rootModelNode());

    return {};
}

std::optional<QString> DSStore::writeQml(const DSThemeManager &mgr,
                                         const QString &typeName,
                                         const Utils::FilePath &targetDir,
                                         bool mcuCompatible)
{
    if (mgr.themeCount() == 0)
        return {};

    const QString interfaceType = mcuCompatible ? themeInterfaceType(typeName) : "QtObject";
    if (mcuCompatible) {
        auto decorateInterface = [&mgr](Model *interfaceModel) {
            mgr.decorateThemeInterface(interfaceModel->rootModelNode());
        };

        if (auto error = modelSerializeHelper(
                m_projectStorageDependencies, m_ed, decorateInterface, targetDir, interfaceType))
            return tr("Can not write theme interface %1.\n%2").arg(interfaceType, *error);
    }

    auto decorateCollection = [&](Model *collectionModel) {
        mgr.decorate(collectionModel->rootModelNode(), interfaceType.toUtf8(), mcuCompatible);
    };

    if (auto error = modelSerializeHelper(
            m_projectStorageDependencies, m_ed, decorateCollection, targetDir, typeName, true))
        return tr("Can not write collection %1.\n%2").arg(typeName, *error);

    return {};
}

void DSStore::removeCollectionFiles(const QString &typeName) const
{
    if (auto moduleDir = dsModuleDir(m_ed)) {
        const auto collectionFilePath = dsFilePath(*moduleDir, typeName);
        collectionFilePath.removeFile();

        const auto interfaceFile = dsFilePath(*moduleDir, themeInterfaceType(typeName));
        interfaceFile.removeFile();
    }
}
} // namespace QmlDesigner
