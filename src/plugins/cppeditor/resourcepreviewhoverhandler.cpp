/****************************************************************************
**
** Copyright (C) 2016 Marc Reilly <marc.reilly+qtc@gmail.com>
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

#include "resourcepreviewhoverhandler.h"

#include <coreplugin/icore.h>
#include <utils/tooltip/tooltip.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/mimetypes/mimedatabase.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>
#include <texteditor/texteditor.h>

#include <QPoint>
#include <QTextBlock>

using namespace Core;
using namespace TextEditor;

namespace CppEditor {
namespace Internal {

/*
 * finds a quoted sub-string around the pos in the given string
 *
 * note: the returned string includes the quotes.
 */
static QString extractQuotedString(const QString &s, int pos)
{
    if (s.length() < 2 || pos < 0 || pos >= s.length())
        return QString();

    const int firstQuote = s.lastIndexOf('"', pos);
    if (firstQuote >= 0) {
        int endQuote = s.indexOf('"', firstQuote + 1);
        if (endQuote > firstQuote)
            return s.mid(firstQuote, endQuote - firstQuote);
    }

    return QString();
}

static QString makeResourcePath(const QStringList &prefixList, const QString &file)
{
    QTC_ASSERT(!prefixList.isEmpty(), return QString());

    const QChar sep = '/';
    auto prefix = prefixList.join(sep);
    if (prefix == sep)
        return prefix + file;

    return prefix + sep + file;
}

/*
 * tries to match a resource within a given .qrc file, including by alias
 *
 * note: resource name should not have any semi-colon in front of it
 */
static QString findResourceInFile(const QString &resName, const QString &filePathName)
{
    Utils::FileReader reader;
    if (!reader.fetch(filePathName))
        return QString();

    const QByteArray contents = reader.data();
    QXmlStreamReader xmlr(contents);

    QStringList prefixStack;

    while (!xmlr.atEnd() && !xmlr.hasError()) {
        const QXmlStreamReader::TokenType token = xmlr.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xmlr.name() == "qresource") {
                const QXmlStreamAttributes sa = xmlr.attributes();
                const QString prefixName = sa.value("prefix").toString();
                if (!prefixName.isEmpty())
                    prefixStack.push_back(prefixName);
            } else if (xmlr.name() == "file") {
                const QXmlStreamAttributes sa = xmlr.attributes();
                const QString aliasName = sa.value("alias").toString();
                const QString fileName = xmlr.readElementText();

                if (!aliasName.isEmpty()) {
                    const QString fullAliasName = makeResourcePath(prefixStack, aliasName);
                    if (resName == fullAliasName)
                        return fileName;
                }

                const QString fullResName = makeResourcePath(prefixStack, fileName);
                if (resName == fullResName)
                    return fileName;
            }
        } else if (token == QXmlStreamReader::EndElement) {
            if (xmlr.name() == "qresource") {
                if (!prefixStack.isEmpty())
                    prefixStack.pop_back();
            }
        }
    }

    return QString();
}

/*
 * A more efficient way to do this would be to parse the relevant project files
 * before hand, or cache them as we go - but this works well enough so far.
 */
static QString findResourceInProject(const QString &resName)
{
    QString s = resName;
    s.remove('"');

    if (s.startsWith(":/"))
        s.remove(0, 1);
    else if (s.startsWith("qrc://"))
        s.remove(0, 5);
    else
        return QString();

    if (auto *project = ProjectExplorer::ProjectTree::currentProject()) {
        const QStringList files = project->files(ProjectExplorer::Project::AllFiles);
        for (const QString &file : files) {
            if (!file.endsWith(".qrc"))
                continue;
            const QFileInfo fi(file);
            if (!fi.isReadable())
                continue;
            const QString fileName = findResourceInFile(s, file);
            if (fileName.isEmpty())
                continue;

            QString ret = fi.absolutePath();
            if (!ret.endsWith('/'))
                ret.append('/');
            ret.append(fileName);
            return ret;
        }
    }

    return QString();
}

void ResourcePreviewHoverHandler::identifyMatch(TextEditorWidget *editorWidget, int pos)
{
    if (editorWidget->extraSelectionTooltip(pos).isEmpty()) {
        const QTextBlock tb = editorWidget->document()->findBlock(pos);
        const int tbpos = pos - tb.position();
        const QString tbtext = tb.text();

        const QString resPath = extractQuotedString(tbtext, tbpos);
        m_resPath = findResourceInProject(resPath);

        setPriority(m_resPath.isEmpty() ? Priority_None : Priority_Diagnostic + 1);
    }
}

void ResourcePreviewHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    const QString tt = makeTooltip();
    if (!tt.isEmpty())
        Utils::ToolTip::show(point, tt, editorWidget);
    else
        Utils::ToolTip::hide();
}

QString ResourcePreviewHoverHandler::makeTooltip() const
{
    if (m_resPath.isEmpty())
        return QString();

    QString ret;

    Utils::MimeDatabase mdb;
    const Utils::MimeType mimeType = mdb.mimeTypeForFile(m_resPath);
    if (mimeType.isValid()) {
        if (mimeType.name().startsWith("image", Qt::CaseInsensitive))
            ret += QString("<img src=\"file:///%1\" /><br/>").arg(m_resPath);

        ret += QString("<a href=\"file:///%1\">%2</a>")
                .arg(m_resPath)
                .arg(QDir::toNativeSeparators(m_resPath));
    }
    return ret;
}

} // namespace Internal
} // namespace CppEditor
