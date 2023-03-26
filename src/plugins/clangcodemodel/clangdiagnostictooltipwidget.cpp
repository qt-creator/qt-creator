// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnostictooltipwidget.h"

#include "clangcodemodeltr.h"
#include "clangfixitoperation.h"
#include "clangutils.h"

#include <coreplugin/editormanager/editormanager.h>

#include <cppeditor/cppeditorconstants.h>

#include <utils/qtcassert.h>
#include <utils/tooltip/tooltip.h>

#include <QApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHash>
#include <QLabel>
#include <QScreen>
#include <QTextDocument>
#include <QUrl>

namespace ClangCodeModel {
namespace Internal {

namespace {

const char LINK_ACTION_GOTO_LOCATION[] = "#gotoLocation";
const char LINK_ACTION_APPLY_FIX[] = "#applyFix";

QString fileNamePrefix(const QString &mainFilePath, const Utils::Link &location)
{
    const QString filePath = location.targetFilePath.toString();
    if (!filePath.isEmpty() && filePath != mainFilePath)
        return QFileInfo(filePath).fileName() + QLatin1Char(':');

    return QString();
}

QString locationToString(const Utils::Link &location)
{
    if (location.targetLine <= 0 || location.targetColumn <= 0)
        return {};
    return QString::number(location.targetLine)
         + QStringLiteral(":")
         + QString::number(location.targetColumn + 1);
}

void applyFixit(const ClangDiagnostic &diagnostic)
{
    ClangFixItOperation operation({}, diagnostic.fixIts);
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
        bool allowTextSelection;
    };

    WidgetFromDiagnostics(const DisplayHints &displayHints)
        : m_displayHints(displayHints)
    {
    }

    QWidget *createWidget(const QList<ClangDiagnostic> &diagnostics,
                          const std::function<bool()> &canApplyFixIt, const QString &source)
    {
        const QString text = htmlText(diagnostics, source);

        auto *label = new QLabel;
        label->setTextFormat(Qt::RichText);
        label->setText(text);
        if (m_displayHints.allowTextSelection) {
            label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        } else {
            label->setTextInteractionFlags(Qt::LinksAccessibleByMouse
                                           | Qt::LinksAccessibleByKeyboard);
        }

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
        QObject::connect(label, &QLabel::linkActivated, [table, hideToolTipAfterLinkActivation,
                         canApplyFixIt](const QString &action) {
            const ClangDiagnostic diagnostic = table.value(action);

            if (diagnostic == ClangDiagnostic())
                QDesktopServices::openUrl(QUrl(action));
            else if (action.startsWith(LINK_ACTION_GOTO_LOCATION)) {
                Core::EditorManager::openEditorAt(diagnostic.location);
            } else if (action.startsWith(LINK_ACTION_APPLY_FIX)) {
                if (canApplyFixIt && canApplyFixIt())
                    applyFixit(diagnostic);
            } else {
                QTC_CHECK(!"Link target cannot be handled.");
            }

            if (hideToolTipAfterLinkActivation)
                ::Utils::ToolTip::hideImmediately();
        });

        return label;
    }

    QString htmlText(const QList<ClangDiagnostic> &diagnostics, const QString &source)
    {
        // For debugging, add: style='border-width:1px;border-color:black'
        QString text = "<table cellspacing='0' cellpadding='0' width='100%'>";

        for (const ClangDiagnostic &diagnostic : diagnostics)
            text.append(tableRows(diagnostic));
        if (!source.isEmpty()) {
            text.append(QString::fromUtf8("<tr><td colspan='2' align='left'>"
                                          "<font color='gray'>%1</font></td></tr>")
                            .arg(Tr::tr("[Source: %1]").arg(source)));
        }

        text.append("</table>");

        return text;
    }

private:
    enum class IndentMode { Indent, DoNotIndent };

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
    static ClangDiagnostic supplementedDiagnostic(const ClangDiagnostic &diagnostic)
    {
        if (!diagnostic.category.isEmpty())
            return diagnostic; // OK, diagnostics from clang itself have this set.

        ClangDiagnostic supplementedDiagnostic = diagnostic;

        DiagnosticTextInfo info(diagnostic.text);
        supplementedDiagnostic.enableOption = info.option();
        supplementedDiagnostic.category = info.category();
        supplementedDiagnostic.text = info.textWithoutOption();

        for (auto &child : supplementedDiagnostic.children)
            child.text = DiagnosticTextInfo(diagnostic.text).textWithoutOption();

        return supplementedDiagnostic;
    }

    QString tableRows(const ClangDiagnostic &diagnostic)
    {
        m_mainFilePath = m_displayHints.showFileNameInMainDiagnostic
                ? QString()
                : diagnostic.location.targetFilePath.toString();

        const ClangDiagnostic diag = supplementedDiagnostic(diagnostic);

        QString text;
        if (m_displayHints.showCategoryAndEnableOption)
            text.append(diagnosticCategoryAndEnableOptionRow(diag));
        text.append(diagnosticRow(diag, IndentMode::DoNotIndent));
        text.append(diagnosticRowsForChildren(diag));

        return text;
    }

    static QString diagnosticCategoryAndEnableOptionRow(const ClangDiagnostic &diagnostic)
    {
        const QString text = QString::fromLatin1(
            "  <tr>"
            "    <td align='left'><b>%1</b></td>"
            "    <td align='right'>&nbsp;<font color='gray'>%2</font></td>"
            "  </tr>")
            .arg(diagnostic.category, diagnostic.enableOption);

        return text;
    }

    QString diagnosticText(const ClangDiagnostic &diagnostic)
    {
        const bool hasFixit = m_displayHints.enableClickableFixits
                && !diagnostic.fixIts.isEmpty();
        const QString diagnosticText = diagnostic.text.toHtmlEscaped();
        bool hasLocation = false;
        QString text = clickableLocation(diagnostic, m_mainFilePath, hasLocation);
        if (hasLocation)
            text += ": ";
        return text += clickableFixIt(diagnostic, diagnosticText, hasFixit);
    }

    QString diagnosticRow(const ClangDiagnostic &diagnostic, IndentMode indentMode)
    {
        const QString text = QString::fromLatin1(
            "  <tr>"
            "    <td colspan='2' align='left' style='%1'>%2</td>"
            "  </tr>")
            .arg(indentModeToHtmlStyle(indentMode),
                 diagnosticText(diagnostic));

        return text;
    }

    QString diagnosticRowsForChildren(const ClangDiagnostic &diagnostic)
    {
        const QList<ClangDiagnostic> &children = diagnostic.children;
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
            const QList<ClangDiagnostic>::const_iterator first,
            const QList<ClangDiagnostic>::const_iterator last)
    {
        QString text;

        for (auto it = first; it != last; ++it)
            text.append(diagnosticRow(*it, IndentMode::Indent));

        return text;
    }

    QString clickableLocation(const ClangDiagnostic &diagnostic, const QString &mainFilePath,
                              bool &hasContent)
    {
        const Utils::Link &location = diagnostic.location;

        const QString filePrefix = fileNamePrefix(mainFilePath, location);
        const QString lineColumn = locationToString(location);
        const QString linkText = filePrefix + lineColumn;
        const QString targetId = generateTargetId(LINK_ACTION_GOTO_LOCATION, diagnostic);
        hasContent = !linkText.isEmpty();

        return wrapInLink(linkText, targetId);
    }

    QString clickableFixIt(const ClangDiagnostic &diagnostic, const QString &text, bool hasFixIt)
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

    QString generateTargetId(const QString &targetPrefix, const ClangDiagnostic &diagnostic)
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
        auto screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        return screen->availableGeometry().width() / 2;
    }

private:
    const DisplayHints m_displayHints;

    using TargetIdToDiagnosticTable = QHash<QString, ClangDiagnostic>;
    TargetIdToDiagnosticTable m_targetIdsToDiagnostics;
    unsigned m_targetIdCounter = 0;

    QString m_mainFilePath;
};

WidgetFromDiagnostics::DisplayHints toHints(const ClangDiagnosticWidget::Destination &destination,
                                            const std::function<bool()> &canApplyFixIt)
{
    WidgetFromDiagnostics::DisplayHints hints;

    if (destination == ClangDiagnosticWidget::ToolTip) {
        hints.showCategoryAndEnableOption = true;
        hints.showFileNameInMainDiagnostic = false;
        hints.enableClickableFixits = canApplyFixIt && canApplyFixIt();
        hints.limitWidth = true;
        hints.hideTooltipAfterLinkActivation = true;
        hints.allowTextSelection = false;
    } else { // Info Bar
        hints.showCategoryAndEnableOption = false;
        hints.showFileNameInMainDiagnostic = true;
        // Clickable fixits might change toolchain headers, so disable.
        hints.enableClickableFixits = false;
        hints.limitWidth = false;
        hints.hideTooltipAfterLinkActivation = false;
        hints.allowTextSelection = true;
    }

    return hints;
}

} // anonymous namespace

QString ClangDiagnosticWidget::createText(
    const QList<ClangDiagnostic> &diagnostics,
    const ClangDiagnosticWidget::Destination &destination)
{
    const QString htmlText = WidgetFromDiagnostics(toHints(destination, {}))
            .htmlText(diagnostics, {});

    QTextDocument document;
    document.setHtml(htmlText);
    QString text = document.toPlainText();

    if (text.startsWith('\n'))
        text = text.mid(1);
    if (text.endsWith('\n'))
        text.chop(1);

    return text;
}

QWidget *ClangDiagnosticWidget::createWidget(
        const QList<ClangDiagnostic> &diagnostics,
        const Destination &destination, const std::function<bool()> &canApplyFixIt,
        const QString &source)
{
    return WidgetFromDiagnostics(toHints(destination, canApplyFixIt))
            .createWidget(diagnostics, canApplyFixIt, source);
}

} // namespace Internal
} // namespace ClangCodeModel
