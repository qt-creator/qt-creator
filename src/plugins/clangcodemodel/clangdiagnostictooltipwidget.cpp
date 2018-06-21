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

#include "clangdiagnostictooltipwidget.h"
#include "clangfixitoperation.h"

#include <coreplugin/editormanager/editormanager.h>

#include <cpptools/cpptoolsconstants.h>

#include <utils/qtcassert.h>
#include <utils/tooltip/tooltip.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QHash>
#include <QLabel>
#include <QUrl>

using namespace ClangCodeModel;
using Internal::ClangDiagnosticWidget;

namespace {

// CLANG-UPGRADE-CHECK: Checks/update URLs.
//
// Once it gets dedicated documentation pages for released versions,
// use them instead of pointing to master, as checks might vanish.
const char CLAZY_DOCUMENTATION_URL_TEMPLATE[]
    = "https://github.com/KDE/clazy/blob/master/docs/checks/README-%1.md";

const char LINK_ACTION_GOTO_LOCATION[] = "#gotoLocation";
const char LINK_ACTION_APPLY_FIX[] = "#applyFix";

QString fileNamePrefix(const QString &mainFilePath,
                       const ClangBackEnd::SourceLocationContainer &location)
{
    const QString filePath = location.filePath.toString();
    if (filePath != mainFilePath)
        return QFileInfo(filePath).fileName() + QLatin1Char(':');

    return QString();
}

QString locationToString(const ClangBackEnd::SourceLocationContainer &location)
{
    return QString::number(location.line)
         + QStringLiteral(":")
         + QString::number(location.column);
}

void openEditorAt(const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    const ClangBackEnd::SourceLocationContainer &location = diagnostic.location;

    Core::EditorManager::openEditorAt(location.filePath.toString(),
                                      int(location.line),
                                      int(location.column - 1));
}

void applyFixit(const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    ClangCodeModel::ClangFixItOperation operation(Utf8String(), diagnostic.fixIts);

    operation.perform();
}

class WidgetFromDiagnostics
{
public:
    struct DisplayHints {
        bool showCategoryAndEnableOption;
        bool showFileNameInMainDiagnostic;
        bool enableClickableFixits;
        bool limitWidth;
        bool hideTooltipAfterLinkActivation;
    };

    static QWidget *create(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                           const DisplayHints &displayHints)
    {
        WidgetFromDiagnostics converter(displayHints);
        return converter.createWidget(diagnostics);
    }

private:
    enum class IndentMode { Indent, DoNotIndent };

    WidgetFromDiagnostics(const DisplayHints &displayHints)
        : m_displayHints(displayHints)
    {
    }

    QWidget *createWidget(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
    {
        const QString text = htmlText(diagnostics);

        auto *label = new QLabel;
        label->setTextFormat(Qt::RichText);
        label->setText(text);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);

        if (m_displayHints.limitWidth) {
            const int limit = widthLimit();
            // Using "setWordWrap(true)" alone will wrap the text already for small
            // widths, so do not require word wrapping until we hit limits.
            if (label->sizeHint().width() > limit) {
                label->setMaximumWidth(limit);
                label->setWordWrap(true);
            }
        } else {
            label->setWordWrap(true);
        }

        const TargetIdToDiagnosticTable table = m_targetIdsToDiagnostics;
        const bool hideToolTipAfterLinkActivation = m_displayHints.hideTooltipAfterLinkActivation;
        QObject::connect(label, &QLabel::linkActivated, [table, hideToolTipAfterLinkActivation]
                         (const QString &action) {
            const ClangBackEnd::DiagnosticContainer diagnostic = table.value(action);

            if (diagnostic == ClangBackEnd::DiagnosticContainer())
                QDesktopServices::openUrl(QUrl(action));
            else if (action.startsWith(LINK_ACTION_GOTO_LOCATION))
                openEditorAt(diagnostic);
            else if (action.startsWith(LINK_ACTION_APPLY_FIX))
                applyFixit(diagnostic);
            else
                QTC_CHECK(!"Link target cannot be handled.");

            if (hideToolTipAfterLinkActivation)
                Utils::ToolTip::hideImmediately();
        });

        return label;
    }

    QString htmlText(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
    {
        // For debugging, add: style='border-width:1px;border-color:black'
        QString text = "<table cellspacing='0' cellpadding='0' width='100%'>";

        foreach (const ClangBackEnd::DiagnosticContainer &diagnostic, diagnostics)
            text.append(tableRows(diagnostic));

        text.append("</table>");

        return text;
    }

    static bool isClazyOption(const QString &option) { return option.startsWith("-Wclazy"); }

    class DiagnosticTextInfo
    {
    public:
        DiagnosticTextInfo(const QString &text)
            : m_text(text)
            , m_squareBracketStartIndex(text.lastIndexOf('['))
        {}

        QString textWithoutOption() const
        {
            if (m_squareBracketStartIndex == -1)
                return m_text;

            return m_text.mid(0, m_squareBracketStartIndex - 1);
        }

        QString option() const
        {
            if (m_squareBracketStartIndex == -1)
                return QString();

            const int index = m_squareBracketStartIndex + 1;
            return m_text.mid(index, m_text.count() - index - 1);
        }

        QString category() const
        {
            if (m_squareBracketStartIndex == -1)
                return QString();

            const int index = m_squareBracketStartIndex + 1;
            if (isClazyOption(m_text.mid(index)))
                return QCoreApplication::translate("ClangDiagnosticWidget", "Clazy Issue");
            else
                return QCoreApplication::translate("ClangDiagnosticWidget", "Clang-Tidy Issue");
        }

    private:
        const QString m_text;
        const int m_squareBracketStartIndex;
    };

    // Diagnostics from clazy/tidy do not have any category or option set but
    // we will conclude them from the diagnostic message.
    //
    // Ideally, libclang should provide the correct category/option by default.
    // However, tidy and clazy diagnostics use "custom diagnostic ids" and
    // clang's static diagnostic table does not know anything about them.
    //
    // For clazy/tidy diagnostics, we expect something like "some text [some option]", e.g.:
    //  * clazy: "Use the static QFileInfo::exists() instead. It's documented to be faster. [-Wclazy-qfileinfo-exists]"
    //  * tidy:  "use emplace_back instead of push_back [modernize-use-emplace]"
    static ClangBackEnd::DiagnosticContainer supplementedDiagnostic(
        const ClangBackEnd::DiagnosticContainer &diagnostic)
    {
        if (!diagnostic.category.isEmpty())
            return diagnostic; // OK, diagnostics from clang itself have this set.

        ClangBackEnd::DiagnosticContainer supplementedDiagnostic = diagnostic;

        DiagnosticTextInfo info(diagnostic.text);
        supplementedDiagnostic.enableOption = info.option();
        supplementedDiagnostic.category = info.category();
        supplementedDiagnostic.text = info.textWithoutOption();

        for (auto &child : supplementedDiagnostic.children)
            child.text = DiagnosticTextInfo(diagnostic.text.toString()).textWithoutOption();

        return supplementedDiagnostic;
    }

    QString tableRows(const ClangBackEnd::DiagnosticContainer &diagnostic)
    {
        m_mainFilePath = m_displayHints.showFileNameInMainDiagnostic
                ? Utf8String()
                : diagnostic.location.filePath;

        const ClangBackEnd::DiagnosticContainer diag = supplementedDiagnostic(diagnostic);

        QString text;
        if (m_displayHints.showCategoryAndEnableOption)
            text.append(diagnosticCategoryAndEnableOptionRow(diag));
        text.append(diagnosticRow(diag, IndentMode::DoNotIndent));
        text.append(diagnosticRowsForChildren(diag));

        return text;
    }

    static QString documentationUrlForOption(const Utf8String &optionAsUtf8String)
    {
        if (optionAsUtf8String.isEmpty())
            return QString();

        QString option = optionAsUtf8String.toString();

        // Clazy
        if (isClazyOption(option)) {
            option = optionAsUtf8String.mid(8); // Remove "-Wclazy-" prefix.
            return QString::fromUtf8(CLAZY_DOCUMENTATION_URL_TEMPLATE).arg(option);
        }

        // Clang itself
        if (option.startsWith("-W"))
            return QString();

        // Clang-Tidy
        return QString::fromUtf8(CppTools::Constants::TIDY_DOCUMENTATION_URL_TEMPLATE).arg(option);
    }

    static QString maybeClickableOption(const Utf8String &option)
    {
        if (option.isEmpty())
            return option;

        const QString link = documentationUrlForOption(option);
        if (link.isEmpty())
            return option;

        return wrapInLink(option.toString(), link);
    }

    static QString diagnosticCategoryAndEnableOptionRow(
            const ClangBackEnd::DiagnosticContainer &diagnostic)
    {
        const QString text = QString::fromLatin1(
            "  <tr>"
            "    <td align='left'><b>%1</b></td>"
            "    <td align='right'>&nbsp;<font color='gray'>%2</font></td>"
            "  </tr>")
            .arg(diagnostic.category, maybeClickableOption(diagnostic.enableOption));

        return text;
    }

    QString diagnosticText(const ClangBackEnd::DiagnosticContainer &diagnostic)
    {
        const bool hasFixit = m_displayHints.enableClickableFixits
                && !diagnostic.fixIts.isEmpty();
        const QString diagnosticText = diagnostic.text.toString().toHtmlEscaped();
        const QString text = QString::fromLatin1("%1: %2")
            .arg(clickableLocation(diagnostic, m_mainFilePath),
                 clickableFixIt(diagnostic, diagnosticText, hasFixit));

        return text;
    }

    QString diagnosticRow(const ClangBackEnd::DiagnosticContainer &diagnostic,
                          IndentMode indentMode)
    {
        const QString text = QString::fromLatin1(
            "  <tr>"
            "    <td colspan='2' align='left' style='%1'>%2</td>"
            "  </tr>")
            .arg(indentModeToHtmlStyle(indentMode),
                 diagnosticText(diagnostic));

        return text;
    }

    QString diagnosticRowsForChildren(const ClangBackEnd::DiagnosticContainer &diagnostic)
    {
        const QVector<ClangBackEnd::DiagnosticContainer> &children = diagnostic.children;
        QString text;

        if (children.size() <= 10) {
            text += diagnosticRowsForChildren(children.begin(), children.end());
        } else {
            text += diagnosticRowsForChildren(children.begin(), children.begin() + 7);
            text += ellipsisRow();
            text += diagnosticRowsForChildren(children.end() - 3, children.end());
        }

        return text;
    }

    QString diagnosticRowsForChildren(
            const QVector<ClangBackEnd::DiagnosticContainer>::const_iterator first,
            const QVector<ClangBackEnd::DiagnosticContainer>::const_iterator last)
    {
        QString text;

        for (auto it = first; it != last; ++it)
            text.append(diagnosticRow(*it, IndentMode::Indent));

        return text;
    }

    QString clickableLocation(const ClangBackEnd::DiagnosticContainer &diagnostic,
                              const QString &mainFilePath)
    {
        const ClangBackEnd::SourceLocationContainer &location = diagnostic.location;

        const QString filePrefix = fileNamePrefix(mainFilePath, location);
        const QString lineColumn = locationToString(location);
        const QString linkText = filePrefix + lineColumn;
        const QString targetId = generateTargetId(LINK_ACTION_GOTO_LOCATION, diagnostic);

        return wrapInLink(linkText, targetId);
    }

    QString clickableFixIt(const ClangBackEnd::DiagnosticContainer &diagnostic,
                           const QString &text,
                           bool hasFixIt)
    {
        if (!hasFixIt)
            return text;

        QString clickableText = text;
        QString nonClickableCategory;
        const int colonPosition = text.indexOf(QStringLiteral(": "));

        if (colonPosition != -1) {
            nonClickableCategory = text.mid(0, colonPosition + 2);
            clickableText = text.mid(colonPosition + 2);
        }

        const QString targetId = generateTargetId(LINK_ACTION_APPLY_FIX, diagnostic);

        return nonClickableCategory + wrapInLink(clickableText, targetId);
    }

    QString generateTargetId(const QString &targetPrefix,
                             const ClangBackEnd::DiagnosticContainer &diagnostic)
    {
        const QString idAsString = QString::number(++m_targetIdCounter);
        const QString targetId = targetPrefix + idAsString;
        m_targetIdsToDiagnostics.insert(targetId, diagnostic);

        return targetId;
    }

    static QString wrapInLink(const QString &text, const QString &target)
    {
        return QStringLiteral("<a href='%1' style='text-decoration:none'>%2</a>").arg(target, text);
    }

    static QString ellipsisRow()
    {
        return QString::fromLatin1(
            "  <tr>"
            "    <td colspan='2' align='left' style='%1'>...</td>"
            "  </tr>")
            .arg(indentModeToHtmlStyle(IndentMode::Indent));
    }

    static QString indentModeToHtmlStyle(IndentMode indentMode)
    {
        return indentMode == IndentMode::Indent
             ? QString("padding-left:10px")
             : QString();
    }

    static int widthLimit()
    {
        return QApplication::desktop()->availableGeometry(QCursor::pos()).width() / 2;
    }

private:
    const DisplayHints m_displayHints;

    using TargetIdToDiagnosticTable = QHash<QString, ClangBackEnd::DiagnosticContainer>;
    TargetIdToDiagnosticTable m_targetIdsToDiagnostics;
    unsigned m_targetIdCounter = 0;

    QString m_mainFilePath;
};

} // anonymous namespace

namespace ClangCodeModel {
namespace Internal {

QWidget *ClangDiagnosticWidget::create(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
        const Destination &destination)
{
    WidgetFromDiagnostics::DisplayHints hints;

    if (destination == ToolTip) {
        hints.showCategoryAndEnableOption = true;
        hints.showFileNameInMainDiagnostic = false;
        hints.enableClickableFixits = true;
        hints.limitWidth = true;
        hints.hideTooltipAfterLinkActivation = true;
    } else { // Info Bar
        hints.showCategoryAndEnableOption = false;
        hints.showFileNameInMainDiagnostic = true;
        // Clickable fixits might change toolchain headers, so disable.
        hints.enableClickableFixits = false;
        hints.limitWidth = false;
        hints.hideTooltipAfterLinkActivation = false;
    }

    return WidgetFromDiagnostics::create(diagnostics, hints);
}

} // namespace Internal
} // namespace ClangCodeModel
