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

#include "profileeditor.h"

#include "profilecompletionassist.h"
#include "profilehighlighter.h"
#include "profilehoverhandler.h"
#include "qmakenodes.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <extensionsystem/pluginmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textdocument.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QTextBlock>

#include <algorithm>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class ProFileEditorWidget : public TextEditorWidget
{
private:
    void findLinkAt(const QTextCursor &,
                    Utils::ProcessLinkCallback &&processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;
    void contextMenuEvent(QContextMenuEvent *) override;

    QString checkForPrfFile(const QString &baseName) const;
};

static bool isValidFileNameChar(const QChar &c)
{
    return c.isLetterOrNumber()
            || c == QLatin1Char('.')
            || c == QLatin1Char('_')
            || c == QLatin1Char('-')
            || c == QLatin1Char('/')
            || c == QLatin1Char('\\');
}

QString ProFileEditorWidget::checkForPrfFile(const QString &baseName) const
{
    const FilePath projectFile = textDocument()->filePath();
    const QmakePriFileNode *projectNode = nullptr;

    // FIXME: Remove this check once project nodes are fully "static".
    for (const Project * const project : SessionManager::projects()) {
        static const auto isParsing = [](const Project *project) {
            for (const Target * const t : project->targets()) {
                for (const BuildConfiguration * const bc : t->buildConfigurations()) {
                    if (bc->buildSystem()->isParsing())
                        return true;
                }
            }
            return false;
        };
        if (isParsing(project))
            continue;

        ProjectNode * const rootNode = project->rootProjectNode();
        QTC_ASSERT(rootNode, continue);
        projectNode = dynamic_cast<const QmakePriFileNode *>(rootNode
                ->findProjectNode([&projectFile](const ProjectNode *pn) {
            return pn->filePath() == projectFile;
        }));
        if (projectNode)
            break;
    }
    if (!projectNode)
        return QString();
    const QmakeProFileNode * const proFileNode = projectNode->proFileNode();
    if (!proFileNode)
        return QString();
    const QmakeProFile * const proFile = proFileNode->proFile();
    if (!proFile)
        return QString();
    for (const QString &featureRoot : proFile->featureRoots()) {
        const QFileInfo candidate(featureRoot + '/' + baseName + ".prf");
        if (candidate.exists())
            return candidate.filePath();
    }
    return QString();
}

void ProFileEditorWidget::findLinkAt(const QTextCursor &cursor,
                                     Utils::ProcessLinkCallback &&processLinkCallback,
                                     bool /*resolveTarget*/,
                                     bool /*inNextSplit*/)
{
    Link link;

    int line = 0;
    int column = 0;
    convertPosition(cursor.position(), &line, &column);
    const int positionInBlock = column - 1;

    const QString block = cursor.block().text();

    // check if the current position is commented out
    const int hashPos = block.indexOf(QLatin1Char('#'));
    if (hashPos >= 0 && hashPos < positionInBlock)
        return processLinkCallback(link);

    // find the beginning of a filename
    QString buffer;
    int beginPos = positionInBlock - 1;
    int endPos = positionInBlock;

    // Check is cursor is somewhere on $${PWD}:
    const int chunkStart = std::max(0, positionInBlock - 7);
    const int chunkLength = 14 + std::min(0, positionInBlock - 7);
    QString chunk = block.mid(chunkStart, chunkLength);

    const QString curlyPwd = "$${PWD}";
    const QString pwd = "$$PWD";
    const int posCurlyPwd = chunk.indexOf(curlyPwd);
    const int posPwd = chunk.indexOf(pwd);
    bool doBackwardScan = true;

    if (posCurlyPwd >= 0) {
        const int end = chunkStart + posCurlyPwd + curlyPwd.count();
        const int start = chunkStart + posCurlyPwd;
        if (start <= positionInBlock && end >= positionInBlock) {
            buffer = pwd;
            beginPos = chunkStart + posCurlyPwd - 1;
            endPos = end;
            doBackwardScan = false;
        }
    } else if (posPwd >= 0) {
        const int end = chunkStart + posPwd + pwd.count();
        const int start = chunkStart + posPwd;
        if (start <= positionInBlock && end >= positionInBlock) {
            buffer = pwd;
            beginPos = start - 1;
            endPos = end;
            doBackwardScan = false;
        }
    }

    while (doBackwardScan && beginPos >= 0) {
        QChar c = block.at(beginPos);
        if (isValidFileNameChar(c)) {
            buffer.prepend(c);
            beginPos--;
        } else {
            break;
        }
    }

    if (doBackwardScan
            && beginPos > 0
            && block.mid(beginPos - 1, pwd.count()) == pwd
            && (block.at(beginPos + pwd.count() - 1) == '/' || block.at(beginPos + pwd.count() - 1) == '\\')) {
        buffer.prepend("$$");
        beginPos -= 2;
    } else if (doBackwardScan
               && beginPos >= curlyPwd.count() - 1
               && block.mid(beginPos - curlyPwd.count() + 1, curlyPwd.count()) == curlyPwd) {
        buffer.prepend(pwd);
        beginPos -= curlyPwd.count();
    }

    // find the end of a filename
    while (endPos < block.count()) {
        QChar c = block.at(endPos);
        if (isValidFileNameChar(c)) {
            buffer.append(c);
            endPos++;
        } else {
            break;
        }
    }

    if (buffer.isEmpty())
        return processLinkCallback(link);

    // remove trailing '\' since it can be line continuation char
    if (buffer.at(buffer.size() - 1) == QLatin1Char('\\')) {
        buffer.chop(1);
        endPos--;
    }

    // if the buffer starts with $$PWD accept it
    if (buffer.startsWith("$$PWD/") || buffer.startsWith("$$PWD\\"))
        buffer = buffer.mid(6);

    QDir dir(textDocument()->filePath().toFileInfo().absolutePath());
    QString fileName = dir.filePath(buffer);
    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            QDir subDir(fi.absoluteFilePath());
            QString subProject = subDir.filePath(subDir.dirName() + QLatin1String(".pro"));
            if (QFileInfo::exists(subProject))
                fileName = subProject;
            else
                return processLinkCallback(link);
        }
        link.targetFileName = QDir::cleanPath(fileName);
    } else {
        link.targetFileName = checkForPrfFile(buffer);
    }
    if (!link.targetFileName.isEmpty()) {
        link.linkTextStart = cursor.position() - positionInBlock + beginPos + 1;
        link.linkTextEnd = cursor.position() - positionInBlock + endPos;
    }
    processLinkCallback(link);
}

void ProFileEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Constants::M_CONTEXT);
}

static TextDocument *createProFileDocument()
{
    auto doc = new TextDocument;
    doc->setId(Constants::PROFILE_EDITOR_ID);
    doc->setMimeType(QLatin1String(Constants::PROFILE_MIMETYPE));
    // qmake project files do not support UTF8-BOM
    // If the BOM would be added qmake would fail and Qt Creator couldn't parse the project file
    doc->setSupportsUtf8Bom(false);
    return doc;
}

//
// ProFileEditorFactory
//

ProFileEditorFactory::ProFileEditorFactory()
{
    setId(Constants::PROFILE_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::PROFILE_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::PROFILE_MIMETYPE);
    addMimeType(Constants::PROINCLUDEFILE_MIMETYPE);
    addMimeType(Constants::PROFEATUREFILE_MIMETYPE);
    addMimeType(Constants::PROCONFIGURATIONFILE_MIMETYPE);
    addMimeType(Constants::PROCACHEFILE_MIMETYPE);
    addMimeType(Constants::PROSTASHFILE_MIMETYPE);

    setDocumentCreator(createProFileDocument);
    setEditorWidgetCreator([]() { return new ProFileEditorWidget; });

    const auto completionAssistProvider = new KeywordsCompletionAssistProvider(qmakeKeywords());
    completionAssistProvider->setDynamicCompletionFunction(&TextEditor::pathComplete);
    setCompletionAssistProvider(completionAssistProvider);

    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setEditorActionHandlers(TextEditorActionHandler::UnCommentSelection
                | TextEditorActionHandler::JumpToFileUnderCursor);

    addHoverHandler(new ProFileHoverHandler);
    setSyntaxHighlighterCreator([]() { return new ProFileHighlighter; });

    const QString defaultOverlay = QLatin1String(ProjectExplorer::Constants::FILEOVERLAY_QT);
    Core::FileIconProvider::registerIconOverlayForSuffix(
                creatorTheme()->imageFile(Theme::IconOverlayPro, defaultOverlay), "pro");
    Core::FileIconProvider::registerIconOverlayForSuffix(
                creatorTheme()->imageFile(Theme::IconOverlayPri, defaultOverlay), "pri");
    Core::FileIconProvider::registerIconOverlayForSuffix(
                creatorTheme()->imageFile(Theme::IconOverlayPrf, defaultOverlay), "prf");
}

} // namespace Internal
} // namespace QmakeProjectManager
