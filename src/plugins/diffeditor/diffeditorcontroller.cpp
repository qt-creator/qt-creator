/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffeditordocument.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QStringList>

namespace DiffEditor {

DiffEditorController::DiffEditorController(Core::IDocument *document) :
    QObject(document),
    m_document(qobject_cast<Internal::DiffEditorDocument *>(document)),
    m_isReloading(false),
    m_diffFileIndex(-1),
    m_chunkIndex(-1)
{
    QTC_ASSERT(m_document, return);
    m_document->setController(this);
}

bool DiffEditorController::isReloading() const
{
    return m_isReloading;
}

QString DiffEditorController::baseDirectory() const
{
    return m_document->baseDirectory();
}

int DiffEditorController::contextLineCount() const
{
    return m_document->contextLineCount();
}

bool DiffEditorController::ignoreWhitespace() const
{
    return m_document->ignoreWhitespace();
}

QString DiffEditorController::revisionFromDescription() const
{
    // TODO: This is specific for git and does not belong here at all!
    return m_document->description().mid(7, 12);
}

QString DiffEditorController::makePatch(bool revert, bool addPrefix) const
{
    return m_document->makePatch(m_diffFileIndex, m_chunkIndex, revert, addPrefix);
}

Core::IDocument *DiffEditorController::findOrCreateDocument(const QString &vcsId,
                                                            const QString &displayName)
{
    QString preferredDisplayName = displayName;
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                Constants::DIFF_EDITOR_ID, &preferredDisplayName, QByteArray(), vcsId);
    return editor ? editor->document() : 0;
}

DiffEditorController *DiffEditorController::controller(Core::IDocument *document)
{
    auto doc = qobject_cast<Internal::DiffEditorDocument *>(document);
    return doc ? doc->controller() : 0;
}

void DiffEditorController::setDiffFiles(const QList<FileData> &diffFileList,
                  const QString &workingDirectory)
{
    m_document->setDiffFiles(diffFileList, workingDirectory);
}

void DiffEditorController::setDescription(const QString &description)
{
    m_document->setDescription(description);
}

void DiffEditorController::informationForCommitReceived(const QString &output)
{
    // TODO: Git specific code...
    const QString branches = prepareBranchesForCommit(output);

    QString tmp = m_document->description();
    tmp.replace(QLatin1String(Constants::EXPAND_BRANCHES), branches);
    m_document->setDescription(tmp);
}

void DiffEditorController::requestMoreInformation()
{
    const QString rev = revisionFromDescription();
    if (!rev.isEmpty())
        emit requestInformationForCommit(rev);
}

QString DiffEditorController::prepareBranchesForCommit(const QString &output)
{
    // TODO: More git-specific code...
    QString moreBranches;
    QString branches;
    QStringList res;
    foreach (const QString &branch, output.split(QLatin1Char('\n'))) {
        const QString b = branch.mid(2).trimmed();
        if (!b.isEmpty())
            res << b;
    }
    const int branchCount = res.count();
    // If there are more than 20 branches, list first 10 followed by a hint
    if (branchCount > 20) {
        const int leave = 10;
        //: Displayed after the untranslated message "Branches: branch1, branch2 'and %n more'"
        //  in git show.
        moreBranches = QLatin1Char(' ') + tr("and %n more", 0, branchCount - leave);
        res.erase(res.begin() + leave, res.end());
    }
    branches = QLatin1String("Branches: ");
    if (res.isEmpty())
        branches += tr("<None>");
    else
        branches += res.join(QLatin1String(", ")) + moreBranches;

    return branches;
}

/**
 * @brief Force the lines of context to the given number.
 *
 * The user will not be able to change the context lines anymore. This needs to be set before
 * starting any operation or the flag will be ignored by the UI.
 *
 * @param lines Lines of context to display.
 */
void DiffEditorController::forceContextLineCount(int lines)
{
    m_document->forceContextLineCount(lines);
}

/**
 * @brief Request the diff data to be re-read.
 */
void DiffEditorController::requestReload()
{
    m_isReloading = true;
    m_document->beginReload();
    reload();
}

void DiffEditorController::reloadFinished(bool success)
{
    m_document->endReload(success);
    m_isReloading = false;
}

void DiffEditorController::requestChunkActions(QMenu *menu, int diffFileIndex, int chunkIndex)
{
    m_diffFileIndex = diffFileIndex;
    m_chunkIndex = chunkIndex;
    emit chunkActionsRequested(menu, diffFileIndex >= 0 && chunkIndex >= 0);
}

} // namespace DiffEditor
