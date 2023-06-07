// Copyright (C) 2016 Marc Reilly <marc.reilly+qtc@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourcepreviewhoverhandler.h"

#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <texteditor/texteditor.h>
#include <utils/fileutils.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/tooltip/tooltip.h>

#include <QPoint>
#include <QScopeGuard>
#include <QTextBlock>
#include <QXmlStreamReader>

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
    if (!reader.fetch(Utils::FilePath::fromString(filePathName)))
        return QString();

    const QByteArray contents = reader.data();
    QXmlStreamReader xmlr(contents);

    QStringList prefixStack;

    while (!xmlr.atEnd() && !xmlr.hasError()) {
        const QXmlStreamReader::TokenType token = xmlr.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xmlr.name() == QLatin1String("qresource")) {
                const QXmlStreamAttributes sa = xmlr.attributes();
                const QString prefixName = sa.value("prefix").toString();
                if (!prefixName.isEmpty())
                    prefixStack.push_back(prefixName);
            } else if (xmlr.name() == QLatin1String("file")) {
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
            if (xmlr.name() == QLatin1String("qresource")) {
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
        const Utils::FilePaths files = project->files(
            [](const ProjectExplorer::Node *n) { return n->filePath().endsWith(".qrc"); });
        for (const Utils::FilePath &file : files) {
            const QFileInfo fi = file.toFileInfo();
            if (!fi.isReadable())
                continue;
            const QString fileName = findResourceInFile(s, file.toString());
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

void ResourcePreviewHoverHandler::identifyMatch(TextEditorWidget *editorWidget,
                                                int pos,
                                                ReportPriority report)
{
    const QScopeGuard cleanup([this, report] { report(priority()); });

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
        Utils::ToolTip::show(point, tt, Qt::MarkdownText, editorWidget);
    else
        Utils::ToolTip::hide();
}

QString ResourcePreviewHoverHandler::makeTooltip() const
{
    if (m_resPath.isEmpty())
        return QString();

    QString ret;

    const Utils::MimeType mimeType = Utils::mimeTypeForFile(m_resPath);
    if (mimeType.name().startsWith("image", Qt::CaseInsensitive))
        ret += QString("![image](%1)  \n").arg(m_resPath);
    ret += QString("[%1](%2)").arg(QDir::toNativeSeparators(m_resPath), m_resPath);
    return ret;
}

} // namespace Internal
} // namespace CppEditor
