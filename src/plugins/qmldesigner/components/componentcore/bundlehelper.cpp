// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bundlehelper.h"

#include "bundleimporter.h"

#include <abstractview.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <utils3d.h>

#include <solutions/zip/zipreader.h>
#include <solutions/zip/zipwriter.h>

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QWidget>

namespace QmlDesigner {

BundleHelper::BundleHelper(AbstractView *view, QWidget *widget)
    : m_view(view)
    , m_widget(widget)
{
    createImporter();
}

BundleHelper::~BundleHelper()
{}

void BundleHelper::createImporter()
{
    m_importer = Utils::makeUniqueObjectPtr<BundleImporter>();

#ifdef QDS_USE_PROJECTSTORAGE
    QObject::connect(m_importer, &BundleImporter::importFinished, m_widget,
        [&](const QmlDesigner::TypeName &typeName, const QString &bundleId) {
            QTC_ASSERT(typeName.size(), return);
            if (isMaterialBundle(bundleId)) {
                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    Utils3D::createMaterial(m_view, typeName);
                });
            } else if (isItemBundle(bundleId)) {
                ModelNode target = Utils3D::active3DSceneNode(m_view);
                if (!target)
                    target = m_view->rootModelNode();
                QTC_ASSERT(target, return);

                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    ModelNode newNode = m_view->createModelNode(typeName, -1, -1);
                    target.defaultNodeListProperty().reparentHere(newNode);
                    newNode.setIdWithoutRefactoring(m_view->model()->generateNewId(
                        newNode.simplifiedTypeName(), "node"));
                    m_view->clearSelectedModelNodes();
                    m_view->selectModelNode(newNode);
                });
            }
        });
#else
    QObject::connect(m_importer.get(), &BundleImporter::importFinished, m_widget,
        [&](const QmlDesigner::NodeMetaInfo &metaInfo, const QString &bundleId) {
            QTC_ASSERT(metaInfo.isValid(), return);
            if (isMaterialBundle(bundleId)) {
                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    Utils3D::createMaterial(m_view, metaInfo);
                });
            } else if (isItemBundle(bundleId)) {

                ModelNode target = Utils3D::active3DSceneNode(m_view);
                if (!target)
                    target = m_view->rootModelNode();
                QTC_ASSERT(target, return);

                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    ModelNode newNode = m_view->createModelNode(metaInfo.typeName(),
                                                                metaInfo.majorVersion(),
                                                                metaInfo.minorVersion());
                    target.defaultNodeListProperty().reparentHere(newNode);
                    newNode.setIdWithoutRefactoring(m_view->model()->generateNewId(
                        newNode.simplifiedTypeName(), "node"));
                    m_view->clearSelectedModelNodes();
                    m_view->selectModelNode(newNode);
                });
            }
        });
#endif
}

void BundleHelper::importBundleToProject()
{
    QString importPath = getImportPath();
    if (importPath.isEmpty())
        return;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    ZipReader zipReader(importPath);

    QByteArray bundleJsonContent = zipReader.fileData(Constants::BUNDLE_JSON_FILENAME);
    QTC_ASSERT(!bundleJsonContent.isEmpty(), return);

    const QJsonObject importedJsonObj = QJsonDocument::fromJson(bundleJsonContent).object();
    const QJsonArray importedItemsArr = importedJsonObj.value("items").toArray();
    QTC_ASSERT(!importedItemsArr.isEmpty(), return);

    QString bundleVersion = importedJsonObj.value("version").toString();
    bool bundleVersionOk = !bundleVersion.isEmpty() && bundleVersion == BUNDLE_VERSION;
    if (!bundleVersionOk) {
        QMessageBox::warning(m_widget, QObject::tr("Unsupported bundle file"),
                             QObject::tr("The chosen bundle was created with an incompatible version"
                                         " of Qt Design Studio"));
        return;
    }
    QString bundleId = importedJsonObj.value("id").toString();

    QTemporaryDir tempDir;
    QTC_ASSERT(tempDir.isValid(), return);
    auto bundlePath = Utils::FilePath::fromString(tempDir.path());

    const QStringList existingQmls = Utils::transform(compUtils.userBundlePath(bundleId)
                                                          .dirEntries(QDir::Files), [](const Utils::FilePath &path) {
                                                          return path.fileName();
                                                      });

    for (const QJsonValueConstRef &itemRef : importedItemsArr) {
        QJsonObject itemObj = itemRef.toObject();
        QString qml = itemObj.value("qml").toString();

        // confirm overwrite if an item with same name exists
        if (existingQmls.contains(qml)) {
            auto reply = QMessageBox::question(m_widget, QObject::tr("Component Exists"),
                                               QObject::tr("A component with the same name '%1' "
                                                           "already exists in the project, are you "
                                                           "sure you want to overwrite it?")
                                                   .arg(qml), QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No)
                continue;

            // TODO: before overwriting remove old item's dependencies (not harmful but for cleanup)
        }

        // add entry to model
        QStringList files = itemObj.value("files").toVariant().toStringList();
        QString icon = itemObj.value("icon").toString();

        // copy files
        QStringList allFiles = files;
        allFiles << qml << icon;
        for (const QString &file : std::as_const(allFiles)) {
            Utils::FilePath filePath = bundlePath.pathAppended(file);
            filePath.parentDir().ensureWritableDir();
            QTC_ASSERT_EXPECTED(filePath.writeFileContents(zipReader.fileData(file)),);
        }

        QString typePrefix = compUtils.userBundleType(bundleId);
        TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();

        QString err = m_importer->importComponent(bundlePath.toFSPathString(), type, qml, files);

        if (!err.isEmpty())
            qWarning() << __FUNCTION__ << err;
    }

    zipReader.close();
}

QString BundleHelper::getImportPath() const
{
    Utils::FilePath projectFP = DocumentManager::currentProjectDirPath();
    if (projectFP.isEmpty()) {
        projectFP = QmlDesignerPlugin::instance()->documentManager()
                        .currentDesignDocument()->fileName().parentDir();
    }

    return QFileDialog::getOpenFileName(m_widget, QObject::tr("Import Component"),
                                        projectFP.toFSPathString(),
                                        QObject::tr("Qt Design Studio Bundle Files (*.%1)")
                                            .arg(Constants::BUNDLE_SUFFIX));
}

bool BundleHelper::isMaterialBundle(const QString &bundleId) const
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    return bundleId == compUtils.materialsBundleId() || bundleId == compUtils.userMaterialsBundleId();
}

// item bundle includes effects and 3D components
bool BundleHelper::isItemBundle(const QString &bundleId) const
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    return bundleId == compUtils.effectsBundleId() || bundleId == compUtils.userEffectsBundleId()
           || bundleId == compUtils.user3DBundleId();
}

} // namespace QmlDesigner
