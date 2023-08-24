// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectsmapdocument.h"

#include "objectsmaptreeitem.h"
#include "squishconstants.h"
#include "squishsettings.h"
#include "squishtr.h"

#include <utils/fileutils.h>
#include <utils/process.h>

#include <QtCore5Compat/QTextCodec>

namespace Squish {
namespace Internal {

static const char kItemSeparator = '\n';
static const char kPropertySeparator = '\t';

ObjectsMapDocument::ObjectsMapDocument()
    : m_contentModel(new ObjectsMapModel(this))
    , m_isModified(false)
{
    setMimeType(Constants::SQUISH_OBJECTSMAP_MIMETYPE);
    setId(Constants::OBJECTSMAP_EDITOR_ID);
    connect(m_contentModel, &ObjectsMapModel::modelChanged, this, [this] { setModified(true); });
}

Core::IDocument::OpenResult ObjectsMapDocument::open(QString *errorString,
                                                     const Utils::FilePath &fileName,
                                                     const Utils::FilePath &realFileName)
{
    OpenResult result = openImpl(errorString, fileName, realFileName);
    if (result == OpenResult::Success) {
        setFilePath(fileName);
        setModified(fileName != realFileName);
    }
    return result;
}

bool ObjectsMapDocument::saveImpl(QString *errorString,
                                  const Utils::FilePath &filePath,
                                  bool autoSave)
{
    if (filePath.isEmpty())
        return false;

    const bool writeOk = writeFile(filePath);
    if (!writeOk) {
        if (errorString)
            *errorString = Tr::tr("Failed to write \"%1\"").arg(filePath.toUserOutput());
        return false;
    }

    if (!autoSave) {
        setModified(false);
        setFilePath(filePath);
    }
    return true;
}

Utils::FilePath ObjectsMapDocument::fallbackSaveAsPath() const
{
    return {};
}

QString ObjectsMapDocument::fallbackSaveAsFileName() const
{
    return "objects.map";
}

void ObjectsMapDocument::setModified(bool modified)
{
    m_isModified = modified;
    emit changed();
}

bool ObjectsMapDocument::reload(QString *errorString,
                                Core::IDocument::ReloadFlag flag,
                                Core::IDocument::ChangeType type)
{
    Q_UNUSED(type);
    if (flag == FlagIgnore)
        return true;
    emit aboutToReload();
    const bool success = (openImpl(errorString, filePath(), filePath()) == OpenResult::Success);
    if (success)
        setModified(false);
    emit reloadFinished(success);
    return success;
}

bool ObjectsMapDocument::buildObjectsMapTree(const QByteArray &contents)
{
    QMap<QString, ObjectsMapTreeItem *> itemForName;

    // get names and their properties as we don't have correct (creation) order inside objects.map
    const QList<QByteArray> lines = contents.split(kItemSeparator);
    for (const QByteArray &line : lines) {
        if (line.isEmpty())
            continue;

        const int tabPosition = line.indexOf(kPropertySeparator);
        const QString objectName = QString::fromUtf8(line.left(tabPosition).trimmed());
        if (!objectName.startsWith(ObjectsMapTreeItem::COLON)) {
            qDeleteAll(itemForName);
            return false;
        }

        ObjectsMapTreeItem *item = new ObjectsMapTreeItem(objectName,
                                                          Qt::ItemIsEnabled | Qt::ItemIsSelectable
                                                              | Qt::ItemIsEditable);

        item->setPropertiesContent(line.mid(tabPosition + 1).trimmed());

        itemForName.insert(objectName, item);
        item->initPropertyModelConnections(m_contentModel);
    }
    // now build the tree
    ObjectsMapTreeItem *root = new ObjectsMapTreeItem(QString());

    QMap<QString, ObjectsMapTreeItem *>::iterator end = itemForName.end();
    for (ObjectsMapTreeItem *item : std::as_const(itemForName)) {
        const QString &parentName = item->parentName();
        auto parent = itemForName.find(parentName);
        if (parent != end)
            parent.value()->appendChild(item);
        else
            root->appendChild(item);
    }

    m_contentModel->changeRootItem(root);
    return true;
}

bool ObjectsMapDocument::setContents(const QByteArray &contents)
{
    return buildObjectsMapTree(contents);
}

QByteArray ObjectsMapDocument::contents() const
{
    QByteArray result;
    QMap<QString, PropertyList> objects;
    m_contentModel->forAllItems([&objects](ObjectsMapTreeItem *item) {
        if (item->parent())
            objects.insert(item->data(0, Qt::DisplayRole).toString(), item->properties());
    });
    const QStringList &keys = objects.keys();

    for (const QString &objName : keys) {
        result.append(objName.toUtf8());
        result.append(kPropertySeparator);

        const PropertyList properties = objects.value(objName);
        // ensure to store invalid properties content as is instead of an empty {}
        if (properties.isEmpty()) {
            if (Utils::TreeItem *item = m_contentModel->findItem(objName)) {
                ObjectsMapTreeItem *objMapItem = static_cast<ObjectsMapTreeItem *>(item);
                if (!objMapItem->isValid()) {
                    result.append(objMapItem->propertiesContent()).append(kItemSeparator);
                    continue;
                }
            }
        }
        result.append('{');
        for (const Property &property : properties) {
            result.append(property.toString().toUtf8());
            result.append(' ');
        }
        // remove the last space added by the last property
        if (result.at(result.size() - 1) == ' ')
            result.chop(1);
        result.append('}');
        result.append(kItemSeparator);
    }

    return result;
}

Core::IDocument::OpenResult ObjectsMapDocument::openImpl(QString *error,
                                                         const Utils::FilePath &fileName,
                                                         const Utils::FilePath &realFileName)
{
    if (fileName.isEmpty())
        return OpenResult::CannotHandle;

    QByteArray text;
    if (realFileName.fileName() == "objects.map") {
        Utils::FileReader reader;
        if (!reader.fetch(realFileName, QIODevice::Text, error))
            return OpenResult::ReadError;

        text = reader.data();
    } else {
        const Utils::FilePath base = settings().squishPath();
        if (base.isEmpty()) {
            if (error)
                error->append(Tr::tr("Incomplete Squish settings. "
                                     "Missing Squish installation path."));
            return OpenResult::ReadError;
        }
        const Utils::FilePath exe = base.pathAppended("lib/exec/objectmaptool").withExecutableSuffix();
        if (!exe.isExecutableFile()) {
            if (error)
                error->append(Tr::tr("objectmaptool not found."));
            return OpenResult::ReadError;
        }

        Utils::Process objectMapReader;
        objectMapReader.setCommand({exe, {"--scriptMap", "--mode", "read",
                                          "--scriptedObjectMapPath", realFileName.toUserOutput()}});
        objectMapReader.setCodec(QTextCodec::codecForName("UTF-8"));
        objectMapReader.start();
        objectMapReader.waitForFinished();
        text = objectMapReader.cleanedStdOut().toUtf8();
    }
    if (!setContents(text)) {
        if (error)
            error->append(Tr::tr("Failure while parsing objects.map content."));
        return OpenResult::ReadError;
    }
    return OpenResult::Success;
}

bool ObjectsMapDocument::writeFile(const Utils::FilePath &fileName) const
{
    if (fileName.endsWith("objects.map")) {
        Utils::FileSaver saver(fileName);
        return saver.write(contents()) && saver.finalize();
    }

    // otherwise we need the objectmaptool to write the scripted object map again
    const Utils::FilePath base = settings().squishPath();
    if (base.isEmpty())
        return false;
    const Utils::FilePath exe = base.pathAppended("lib/exec/objectmaptool").withExecutableSuffix();
    if (!exe.isExecutableFile())
        return false;

    Utils::Process objectMapWriter;
    objectMapWriter.setCommand({exe, {"--scriptMap", "--mode", "write",
                                      "--scriptedObjectMapPath", fileName.toUserOutput()}});
    objectMapWriter.setWriteData(contents());
    objectMapWriter.start();
    objectMapWriter.waitForFinished();
    return objectMapWriter.result() == Utils::ProcessResult::FinishedWithSuccess;
}

} // namespace Internal
} // namespace Squish
