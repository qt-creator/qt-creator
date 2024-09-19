// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dsstore.h"
#include "dsthememanager.h"

#include <generatedcomponentutils.h>
#include <plaintexteditmodifier.h>
#include <qmljs/parser/qmldirparser_p.h>
#include <qmljs/qmljsreformatter.h>
#include <rewriterview.h>
#include <rewritingexception.h>
#include <uniquename.h>
#include <utils/fileutils.h>

#include <QLoggingCategory>
#include <QPlainTextEdit>

namespace {

constexpr char DesignModuleName[] = "DesignSystem";

QString capitalize(QStringView str)
{
    if (str.isEmpty())
        return QString();
    QString tmp = str.toString();
    tmp[0] = str[0].toUpper();
    return tmp;
}

std::optional<Utils::FilePath> dsModuleDir(QmlDesigner::ExternalDependenciesInterface &ed)
{
    auto componentsPath = QmlDesigner::GeneratedComponentUtils(ed).generatedComponentsPath();
    if (componentsPath.exists())
        return componentsPath.pathAppended(DesignModuleName);

    return {};
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

std::optional<QString> modelSerializeHelper(QmlDesigner::ExternalDependenciesInterface &ed,
                                            std::function<void(QmlDesigner::Model *)> callback,
                                            const Utils::FilePath &targetDir,
                                            const QString &typeName,
                                            bool isSingelton = false)
{
    QString qmlText{"import QtQuick\nQtObject {}\n"};
    if (isSingelton)
        qmlText.prepend("pragma Singleton\n");

    QmlDesigner::ModelPointer model(QmlDesigner::Model::create("QtObject"));
    QPlainTextEdit editor;
    editor.setPlainText(qmlText);
    QmlDesigner::NotIndentingTextEditModifier modifier(&editor);
    QmlDesigner::RewriterView view(ed, QmlDesigner::RewriterView::Validate);
    view.setPossibleImportsEnabled(false);
    view.setCheckSemanticErrors(false);
    view.setTextModifier(&modifier);
    model->attachView(&view);

    try {
        callback(model.get());
    } catch (const QmlDesigner::RewritingException &e) {
        return e.description();
    }

    Utils::FileSaver saver(targetDir / (typeName + ".qml"), QIODevice::Text);
    saver.write(reformatQml(modifier.text()));
    if (!saver.finalize())
        return saver.errorString();

    return {};
}

} // namespace

namespace QmlDesigner {

DSStore::DSStore(ExternalDependenciesInterface &ed)
    : m_ed(ed)
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
    // read qmldir
    const auto qmldirFile = dsModuleDirPath / "qmldir";
    const Utils::expected_str<QByteArray> contents = qmldirFile.fileContents();
    if (!contents)
        return tr("Can not read Design System qmldir");

    m_collectionTypeNames.clear();
    m_collections.clear();

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

std::optional<QString> DSStore::save(bool mcuCompatible) const
{
    if (auto moduleDir = dsModuleDir(m_ed))
        return save(*moduleDir, mcuCompatible);

    return tr("Can not locate design system module");
}

std::optional<QString> DSStore::save(const Utils::FilePath &moduleDirPath, bool mcuCompatible) const
{
    if (!QDir().mkpath(moduleDirPath.absoluteFilePath().toString()))
        return tr("Can not create design system module directory %1.").arg(moduleDirPath.toString());

    // dump collections
    QStringList singletons;
    QStringList errors;
    for (auto &[typeName, collection] : m_collections) {
        if (auto err = writeQml(collection, typeName, moduleDirPath, mcuCompatible))
            errors << *err;
        singletons << QString("singleton %1 1.0 %1.qml").arg(typeName);
    }

    // Write qmldir
    Utils::FileSaver saver(moduleDirPath / "qmldir", QIODevice::Text);
    const QString qmldirContents = QString("Module %1\n%2").arg(moduleImportStr(), singletons.join("\n"));
    saver.write(qmldirContents.toUtf8());
    if (!saver.finalize())
        errors << tr("Can not write design system qmldir. %1").arg(saver.errorString());

    if (!errors.isEmpty())
        return errors.join("\n");

    return {};
}

DSThemeManager *DSStore::addCollection(const QString &qmlTypeName)
{
    const QString uniqueTypeName = UniqueName::generateId(qmlTypeName,
                                                          "designSystem",
                                                          [this](const QString &t) {
                                                              return m_collections.contains(t);
                                                          });

    const QString componentType = capitalize(uniqueTypeName);
    auto [itr, success] = m_collections.try_emplace(componentType, DSThemeManager{});
    if (success) {
        m_collectionTypeNames.insert({&itr->second, itr->first});
        return &itr->second;
    }
    return nullptr;
}

std::optional<QString> DSStore::typeName(DSThemeManager *collection) const
{
    auto itr = m_collectionTypeNames.find(collection);
    if (itr != m_collectionTypeNames.end())
        return itr->second;

    return {};
}

std::optional<QString> DSStore::loadCollection(const QString &typeName,
                                               const Utils::FilePath &qmlFilePath)
{
    Utils::FileReader reader;
    if (!reader.fetch(qmlFilePath, QFile::Text))
        return reader.errorString();

    ModelPointer model(QmlDesigner::Model::create("QtObject"));

    QPlainTextEdit editor;
    QString qmlContent = QString::fromUtf8(reader.data());
    editor.setPlainText(qmlContent);

    QmlDesigner::NotIndentingTextEditModifier modifier(&editor);
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
                                         bool mcuCompatible) const
{
    if (mgr.themeCount() == 0)
        return {};

    const QString themeInterfaceType = mcuCompatible ? QString("%1Theme").arg(typeName) : "QtObject";
    if (mcuCompatible) {
        auto decorateInterface = [&mgr](Model *interfaceModel) {
            mgr.decorateThemeInterface(interfaceModel->rootModelNode());
        };

        if (auto error = modelSerializeHelper(m_ed, decorateInterface, targetDir, themeInterfaceType))
            return tr("Can not write theme interface %1.\n%2").arg(themeInterfaceType, *error);
    }

    auto decorateCollection = [&](Model *collectionModel) {
        mgr.decorate(collectionModel->rootModelNode(), themeInterfaceType.toUtf8(), mcuCompatible);
    };

    if (auto error = modelSerializeHelper(m_ed, decorateCollection, targetDir, typeName, true))
        return tr("Can not write collection %1.\n%2").arg(typeName, *error);

    return {};
}
} // namespace QmlDesigner
