// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectsmapdocument.h"

#include "objectsmaptreeitem.h"
#include "squishconstants.h"
#include "squishsettings.h"
#include "squishtr.h"

#include <utils/fileutils.h>
#include <utils/qtcprocess.h>

using namespace Core;
using namespace Utils;

namespace Squish::Internal {

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

Result<> ObjectsMapDocument::open(const FilePath &fileName, const FilePath &realFileName)
{
    Result<> result = openImpl(fileName, realFileName);
    if (result.has_value()) {
        setFilePath(fileName);
        setModified(fileName != realFileName);
    }
    return result;
}

Result<> ObjectsMapDocument::saveImpl(const FilePath &filePath, bool autoSave)
{
    if (filePath.isEmpty())
        return ResultError("ASSERT: ObjectsMapDocument: filePath.isEmpty()");

    const bool writeOk = writeFile(filePath);
    if (!writeOk)
        return ResultError(Tr::tr("Failed to write \"%1\"").arg(filePath.toUserOutput()));

    if (!autoSave) {
        setModified(false);
        setFilePath(filePath);
    }
    return ResultOk;
}

FilePath ObjectsMapDocument::fallbackSaveAsPath() const
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

Result<> ObjectsMapDocument::reload(IDocument::ReloadFlag flag, IDocument::ChangeType type)
{
    Q_UNUSED(type)
    if (flag == FlagIgnore)
        return ResultOk;
    emit aboutToReload();
    const Result<> result = openImpl(filePath(), filePath());
    if (result.has_value())
        setModified(false);
    emit reloadFinished(result.has_value());
    return result;
}

Result<> ObjectsMapDocument::buildObjectsMapTree(const QByteArray &contents)
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
            return ResultError(Tr::tr("Object name does not start with colon"));
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
    return ResultOk;
}

Result<> ObjectsMapDocument::setContents(const QByteArray &contents)
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

Result<> ObjectsMapDocument::openImpl(const FilePath &fileName, const FilePath &realFileName)
{
    if (fileName.isEmpty())
        return ResultError("File name is empty"); // FIXME: Find somethong better

    QByteArray text;
    if (realFileName.fileName() == "objects.map") {
        FileReader reader;
        if (const Result<> res = reader.fetch(realFileName); !res)
            return res;

        text = reader.text();
    } else {
        const FilePath base = settings().squishPath();
        if (base.isEmpty()) {
            return ResultError(Tr::tr("Incomplete Squish settings. "
                                      "Missing Squish installation path."));
        }
        const FilePath exe = base.pathAppended("lib/exec/objectmaptool").withExecutableSuffix();
        if (!exe.isExecutableFile())
            return ResultError(Tr::tr("objectmaptool not found."));


        Process objectMapReader;
        objectMapReader.setCommand({exe, {"--scriptMap", "--mode", "read",
                                          "--scriptedObjectMapPath", realFileName.toUserOutput()}});
        objectMapReader.setUtf8Codec();
        objectMapReader.start();
        objectMapReader.waitForFinished();
        text = objectMapReader.cleanedStdOut().toUtf8();
    }
    if (!setContents(text))
        return ResultError(Tr::tr("Failure while parsing objects.map content."));
    return ResultOk;
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

} // namespace Squish::Internal
