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

#include "qmlprojectnodes.h"
#include "qmlproject.h"

#include <coreplugin/idocument.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <projectexplorer/projectexplorer.h>
#include <texteditor/textdocument.h>
#include <utils/algorithm.h>
#include <utils/textfileformat.h>

#include <QRegularExpression>
#include <QTextCodec>

using namespace ProjectExplorer;

namespace QmlProjectManager {
namespace Internal {

QmlProjectNode::QmlProjectNode(QmlProject *project) : ProjectNode(project->projectDirectory()),
    m_project(project)
{
    setDisplayName(project->projectFilePath().toFileInfo().completeBaseName());

    static QIcon qmlProjectIcon = Core::FileIconProvider::directoryIcon(":/projectexplorer/images/fileoverlay_qml.png");
    setIcon(qmlProjectIcon);
}

bool QmlProjectNode::showInSimpleTree() const
{
    return true;
}

bool QmlProjectNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (action == AddNewFile || action == EraseFile)
        return true;
    QTC_ASSERT(node, return false);

    if (action == Rename && node->nodeType() == NodeType::File) {
        const FileNode *fileNode = node->asFileNode();
        QTC_ASSERT(fileNode, return false);
        return fileNode->fileType() != FileType::Project;
    }

    return false;
}

bool QmlProjectNode::addFiles(const QStringList &filePaths, QStringList * /*notAdded*/)
{
    return m_project->addFiles(filePaths);
}

bool QmlProjectNode::deleteFiles(const QStringList & /*filePaths*/)
{
    return true;
}

bool QmlProjectNode::renameFile(const QString & filePath, const QString & newFilePath)
{
    if (filePath.endsWith(m_project->mainFile())) {
        m_project->setMainFile(newFilePath);

        // make sure to change it also in the qmlproject file
        const QString qmlProjectFilePath = m_project->document()->filePath().toString();
        Core::FileChangeBlocker fileChangeBlocker(qmlProjectFilePath);
        const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForFilePath(qmlProjectFilePath);
        TextEditor::TextDocument *document = nullptr;
        if (!editors.isEmpty()) {
            document = qobject_cast<TextEditor::TextDocument*>(editors.first()->document());
            if (document && document->isModified())
                if (!Core::DocumentManager::saveDocument(document))
                    return false;
        }

        QString fileContent;
        QString error;
        Utils::TextFileFormat textFileFormat;
        const QTextCodec *codec = QTextCodec::codecForName("UTF-8"); // qml files are defined to be utf-8
        if (Utils::TextFileFormat::readFile(qmlProjectFilePath, codec, &fileContent, &textFileFormat, &error)
                != Utils::TextFileFormat::ReadSuccess) {
            qWarning() << "Failed to read file" << qmlProjectFilePath << ":" << error;
        }

        // find the mainFile and do the file name with brackets in a capture group and mask the . with \.
        QString originalFileName = QFileInfo(filePath).fileName();
        originalFileName.replace(".", "\\.");
        const QRegularExpression expression(QString("mainFile:\\s*\"(%1)\"").arg(originalFileName));
        const QRegularExpressionMatch match = expression.match(fileContent);

        fileContent.replace(match.capturedStart(1), match.capturedLength(1), QFileInfo(newFilePath).fileName());

        if (!textFileFormat.writeFile(qmlProjectFilePath, fileContent, &error))
            qWarning() << "Failed to write file" << qmlProjectFilePath << ":" << error;
        m_project->refresh(QmlProject::Everything);
    }

    return true;
}

} // namespace Internal
} // namespace QmlProjectManager
