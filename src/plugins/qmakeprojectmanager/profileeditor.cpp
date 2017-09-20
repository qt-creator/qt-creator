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
#include "qmakeprojectmanager.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagerconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <extensionsystem/pluginmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textdocument.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QTextBlock>

#include <algorithm>

using namespace TextEditor;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class ProFileEditorWidget : public TextEditorWidget
{
protected:
    virtual Link findLinkAt(const QTextCursor &, bool resolveTarget = true,
                            bool inNextSplit = false) override;
    void contextMenuEvent(QContextMenuEvent *) override;
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

ProFileEditorWidget::Link ProFileEditorWidget::findLinkAt(const QTextCursor &cursor,
                                                          bool /*resolveTarget*/,
                                                          bool /*inNextSplit*/)
{
    Link link;

    int lineNumber = 0, positionInBlock = 0;
    convertPosition(cursor.position(), &lineNumber, &positionInBlock);

    const QString block = cursor.block().text();

    // check if the current position is commented out
    const int hashPos = block.indexOf(QLatin1Char('#'));
    if (hashPos >= 0 && hashPos < positionInBlock)
        return link;

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
        return link;

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
                return link;
        }
        link.targetFileName = QDir::cleanPath(fileName);
        link.linkTextStart = cursor.position() - positionInBlock + beginPos + 1;
        link.linkTextEnd = cursor.position() - positionInBlock + endPos;
    }
    return link;
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

    setCompletionAssistProvider(new KeywordsCompletionAssistProvider(qmakeKeywords()));

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
