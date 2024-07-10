// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extpropertiesmview.h"

#include "modeleditortr.h"

#include "qmt/model/mpackage.h"
#include "qmt/diagram/dobject.h"
#include "qmt/project/project.h"
#include "qmt/project_controller/projectcontroller.h"

#include <coreplugin/icore.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>

#include <QLabel>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QImageReader>

using Utils::FilePath;

namespace ModelEditor {
namespace Internal {

static QString imageNameFilterString()
{
    static QString result;
    if (result.isEmpty()) {
        QMimeDatabase mimeDatabase;
        const QString separator = QStringLiteral(";;");
        const auto mimeTypes = QImageReader::supportedMimeTypes();
        for (const QByteArray &mimeType : mimeTypes) {
            const QString filter = mimeDatabase.mimeTypeForName(QLatin1String(mimeType))
                                       .filterString();
            if (!filter.isEmpty()) {
                if (mimeType == QByteArrayLiteral("image/png")) {
                    if (!result.isEmpty())
                        result.prepend(separator);
                    result.prepend(filter);
                } else {
                    if (!result.isEmpty())
                        result.append(separator);
                    result.append(filter);
                }
            }
        }
    }
    return result;
}

/// Constructs an absolute FilePath from \a relativePath which
/// is interpreted as being relative to \a anchor.
FilePath absoluteFromRelativePath(const Utils::FilePath &relativePath,
                                         const Utils::FilePath &anchor)
{
    QDir anchorDir = QFileInfo(anchor.path()).absoluteDir();
    QString absoluteFilePath = QFileInfo(anchorDir, relativePath.path()).canonicalFilePath();
    return Utils::FilePath::fromString(absoluteFilePath);
}

ExtPropertiesMView::ExtPropertiesMView(qmt::PropertiesView *view)
    : qmt::PropertiesView::MView(view)
{
}

ExtPropertiesMView::~ExtPropertiesMView()
{
}

void ExtPropertiesMView::setProjectController(qmt::ProjectController *projectController)
{
    m_projectController = projectController;
}

void ExtPropertiesMView::visitMPackage(const qmt::MPackage *package)
{
    qmt::PropertiesView::MView::visitMPackage(package);
    if (m_modelElements.size() == 1 && !package->owner()) {
        qmt::Project *project = m_projectController->project();
        if (!m_configPath) {
            m_configPath = new Utils::PathChooser(m_topWidget);
            m_configPath->setPromptDialogTitle(Tr::tr("Select Custom Configuration Folder"));
            m_configPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
            m_configPath->setInitialBrowsePathBackup(project->fileName().absolutePath());
            addRow(Tr::tr("Config path:"), m_configPath, "configpath");
            connect(m_configPath, &Utils::PathChooser::textChanged,
                    this, &ExtPropertiesMView::onConfigPathChanged,
                    Qt::QueuedConnection);
        }
        if (!m_configPath->hasFocus()) {
            if (project->configPath().isEmpty()) {
                m_configPath->setFilePath({});
            } else {
                // make path absolute (may be relative to current project's directory)
                auto projectDir = project->fileName().absolutePath();
                m_configPath->setPath(projectDir.resolvePath(project->configPath()).toUserOutput());
            }
        }
        if (!m_configPathInfo) {
            m_configPathInfo = new QLabel(m_topWidget);
            addRow(QString(), m_configPathInfo, "configpathinfo");
        }
    }
}

void ExtPropertiesMView::visitMObjectBehind(const qmt::MObject *object)
{
    qmt::Project *project = m_projectController->project();
    QList<qmt::MObject *> selection = filter<qmt::MObject>(m_modelElements);
    bool isSingleSelection = selection.size() == 1;
    if (!m_filelinkPathChooser) {
        m_filelinkPathChooser = new Utils::PathChooser(m_topWidget);
        m_filelinkPathChooser->setPromptDialogTitle((Tr::tr("Select File Target")));
        m_filelinkPathChooser->setExpectedKind(Utils::PathChooser::File);
        m_filelinkPathChooser->setInitialBrowsePathBackup(project->fileName().absolutePath());
        addRow(Tr::tr("Linked file:"), m_filelinkPathChooser, "filelink");
        connect(m_filelinkPathChooser, &Utils::PathChooser::textChanged,
                this, &ExtPropertiesMView::onFileLinkPathChanged,
                Qt::QueuedConnection);
    }
    if (isSingleSelection) {
        if (!m_filelinkPathChooser->hasFocus()) {
            Utils::FilePath path = object->linkedFileName();
            if (path.isEmpty()) {
                m_filelinkPathChooser->setPath(QString());
            } else {
                Utils::FilePath relativePath = path;
                Utils::FilePath projectPath = project->fileName();
                QString filePath = absoluteFromRelativePath(relativePath, projectPath).toFSPathString();
                m_filelinkPathChooser->setPath(filePath);
            }
        }
    } else {
        m_filelinkPathChooser->setPath(QString());
    }
    if (m_filelinkPathChooser->isEnabled() != isSingleSelection)
        m_filelinkPathChooser->setEnabled(isSingleSelection);
}

void ExtPropertiesMView::visitDObjectBefore(const qmt::DObject *object)
{
    qmt::Project *project = m_projectController->project();
    QList<qmt::DObject *> selection = filter<qmt::DObject>(m_diagramElements);
    bool isSingleSelection = selection.size() == 1;
    if (!m_imagePathChooser) {
        m_imagePathChooser = new Utils::PathChooser(m_topWidget);
        m_imagePathChooser->setPromptDialogTitle(Tr::tr("Select Image File"));
        m_imagePathChooser->setExpectedKind(Utils::PathChooser::File);
        m_imagePathChooser->setPromptDialogFilter(imageNameFilterString());
        m_imagePathChooser->setInitialBrowsePathBackup(project->fileName().absolutePath());
        addRow(Tr::tr("Image:"), m_imagePathChooser, "imagepath");
        connect(m_imagePathChooser, &Utils::PathChooser::textChanged,
                this, &ExtPropertiesMView::onImagePathChanged,
                Qt::QueuedConnection);
    }
    if (isSingleSelection) {
        if (!m_imagePathChooser->hasFocus()) {
            Utils::FilePath path = object->imagePath();
            if (path.isEmpty()) {
                m_imagePathChooser->setPath(QString());
            } else {
                Utils::FilePath relativePath = path;
                QString filePath = absoluteFromRelativePath(relativePath, project->fileName()).toFSPathString();
                m_imagePathChooser->setPath(filePath);
            }
        }
    } else {
        m_imagePathChooser->setPath(QString());
    }
    if (m_imagePathChooser->isEnabled() != isSingleSelection)
        m_imagePathChooser->setEnabled(isSingleSelection);
}

void ExtPropertiesMView::onConfigPathChanged(const QString &path)
{
    bool modified = false;
    qmt::Project *project = m_projectController->project();
    if (path.isEmpty()) {
        if (!project->configPath().isEmpty()) {
            project->setConfigPath({ });
            m_projectController->setModified();
            modified = true;
        }
    } else {
        // make path relative to current project's directory
        Utils::FilePath absConfigPath = Utils::FilePath::fromString(path).absoluteFilePath();
        Utils::FilePath projectDir = project->fileName().absolutePath();
        Utils::FilePath configPath = absConfigPath.relativePathFrom(projectDir);
        if (configPath != project->configPath()) {
            project->setConfigPath(configPath);
            m_projectController->setModified();
            modified = true;
        }
    }
    if (modified && m_configPathInfo)
        m_configPathInfo->setText(Tr::tr("<font color=red>Model file must be reloaded.</font>"));
}

void ExtPropertiesMView::onFileLinkPathChanged(const QString &path)
{
    qmt::Project *project = m_projectController->project();
    if (path.isEmpty()) {
        assignModelElement<qmt::MObject, Utils::FilePath>(
            m_modelElements,
            SelectionSingle,
            {},
            &qmt::MObject::linkedFileName,
            &qmt::MObject::setLinkedFileName);
    } else {
        // make path relative to current project's directory
        Utils::FilePath filePath = Utils::FilePath::fromString(path);
        Utils::FilePath projectPath = project->fileName().absolutePath();
        Utils::FilePath relativeFilePath = filePath.relativePathFrom(projectPath);
        if (!relativeFilePath.isEmpty()) {
            assignModelElement<qmt::MObject, Utils::FilePath>(
                m_modelElements,
                SelectionSingle,
                relativeFilePath,
                &qmt::MObject::linkedFileName,
                &qmt::MObject::setLinkedFileName);
        }
    }
}

void ExtPropertiesMView::onImagePathChanged(const QString &path)
{
    qmt::Project *project = m_projectController->project();
    if (path.isEmpty()) {
        assignModelElement<qmt::DObject, Utils::FilePath>(
            m_diagramElements,
            SelectionSingle,
            {},
            &qmt::DObject::imagePath,
            &qmt::DObject::setImagePath);
        assignModelElement<qmt::DObject, QImage>(m_diagramElements, SelectionSingle, QImage(),
                                                 &qmt::DObject::image, &qmt::DObject::setImage);
    } else {
        // make path relative to current project's directory
        Utils::FilePath filePath = Utils::FilePath::fromString(path);
        Utils::FilePath projectPath = project->fileName().absolutePath();
        Utils::FilePath relativeFilePath = filePath.relativePathFrom(projectPath);
        if (!relativeFilePath.isEmpty()
            && isValueChanged<qmt::DObject, Utils::FilePath>(
                m_diagramElements, SelectionSingle, relativeFilePath, &qmt::DObject::imagePath)) {
            QImage image;
            if (image.load(path)) {
                assignModelElement<qmt::DObject, Utils::FilePath>(
                    m_diagramElements,
                    SelectionSingle,
                    relativeFilePath,
                    &qmt::DObject::imagePath,
                    &qmt::DObject::setImagePath);
                assignModelElement<qmt::DObject, QImage>(m_diagramElements, SelectionSingle, image,
                                                         &qmt::DObject::image, &qmt::DObject::setImage);
            } else {
                QMessageBox::critical(
                    Core::ICore::dialogParent(),
                    Tr::tr("Selecting Image"),
                    Tr::tr("Unable to read image file \"%1\".").arg(path));
            }
        }
    }
}

} // namespace Interal
} // namespace ModelEditor
