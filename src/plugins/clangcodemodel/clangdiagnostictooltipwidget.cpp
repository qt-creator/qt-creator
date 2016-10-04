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

#include <utils/tooltip/tooltip.h>

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

const char LINK_ACTION_GOTO_LOCATION[] = "#gotoLocation";
const char LINK_ACTION_APPLY_FIX[] = "#applyFix";
const int childIndentationOnTheLeftInPixel = 10;

QString wrapInBoldTags(const QString &text)
{
    return QStringLiteral("<b>") + text + QStringLiteral("</b>");
}

QString wrapInLink(const QString &text, const QString &target)
{
    return QStringLiteral("<a href='%1' style='text-decoration:none'>%2</a>").arg(target, text);
}

QString wrapInColor(const QString &text, const QByteArray &color)
{
    return QStringLiteral("<font color='%2'>%1</font>").arg(text, QString::fromUtf8(color));
}

QString fileNamePrefix(const QString &mainFilePath,
                       const ClangBackEnd::SourceLocationContainer &location)
{
    const QString filePath = location.filePath().toString();
    if (filePath != mainFilePath)
        return QFileInfo(filePath).fileName() + QLatin1Char(':');

    return QString();
}

QString locationToString(const ClangBackEnd::SourceLocationContainer &location)
{
    return QString::number(location.line())
         + QStringLiteral(":")
         + QString::number(location.column());
}

QString clickableLocation(const QString &mainFilePath,
                          const ClangBackEnd::SourceLocationContainer &location)
{
    const QString filePrefix = fileNamePrefix(mainFilePath, location);
    const QString lineColumn = locationToString(location);
    const QString linkText = filePrefix + lineColumn;

    return wrapInLink(linkText, QLatin1String(LINK_ACTION_GOTO_LOCATION));
}

QString clickableFixIt(const QString &text, bool hasFixIt)
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

    return nonClickableCategory + wrapInLink(clickableText, QLatin1String(LINK_ACTION_APPLY_FIX));
}

void openEditorAt(const ClangBackEnd::SourceLocationContainer &location)
{
    Core::EditorManager::openEditorAt(location.filePath().toString(),
                                      int(location.line()),
                                      int(location.column() - 1));
}

void applyFixit(const QVector<ClangBackEnd::FixItContainer> &fixits)
{
    ClangCodeModel::ClangFixItOperation operation(Utf8String(), fixits);

    operation.perform();
}

template <typename LayoutType>
LayoutType *createLayout()
{
    auto *layout = new LayoutType;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    return layout;
}

enum IndentType { IndentDiagnostic, DoNotIndentDiagnostic };

QWidget *createDiagnosticLabel(const ClangBackEnd::DiagnosticContainer &diagnostic,
                               const QString &mainFilePath,
                               IndentType indentType = DoNotIndentDiagnostic,
                               bool enableClickableFixits = true)
{
    const bool hasFixit = enableClickableFixits ? !diagnostic.fixIts().isEmpty() : false;
    const QString diagnosticText = diagnostic.text().toString().toHtmlEscaped();
    const QString text = clickableLocation(mainFilePath, diagnostic.location())
                       + QStringLiteral(": ")
                       + clickableFixIt(diagnosticText, hasFixit);
    const ClangBackEnd::SourceLocationContainer location = diagnostic.location();
    const QVector<ClangBackEnd::FixItContainer> fixits = diagnostic.fixIts();

    auto *label = new QLabel(text);
    if (indentType == IndentDiagnostic)
        label->setContentsMargins(childIndentationOnTheLeftInPixel, 0, 0, 0);
    label->setTextFormat(Qt::RichText);
    QObject::connect(label, &QLabel::linkActivated, [location, fixits](const QString &action) {
        if (action == QLatin1String(LINK_ACTION_APPLY_FIX))
            applyFixit(fixits);
        else
            openEditorAt(location);

        Utils::ToolTip::hideImmediately();
    });

    return label;
}

class MainDiagnosticWidget : public QWidget
{
    Q_OBJECT
public:
    MainDiagnosticWidget(const ClangBackEnd::DiagnosticContainer &diagnostic,
                         const ClangCodeModel::Internal::DisplayHints &displayHints)
    {
        setContentsMargins(0, 0, 0, 0);
        auto *mainLayout = createLayout<QVBoxLayout>();

        const ClangBackEnd::SourceLocationContainer location = diagnostic.location();

        // Set up header row: category + responsible option
        if (displayHints.showMainDiagnosticHeader) {
            const QString category = diagnostic.category();
            const QString responsibleOption = diagnostic.enableOption();

            auto *headerLayout = createLayout<QHBoxLayout>();
            headerLayout->addWidget(new QLabel(wrapInBoldTags(category)), 1);

            auto *responsibleOptionLabel = new QLabel(wrapInColor(responsibleOption, "gray"));
            headerLayout->addWidget(responsibleOptionLabel, 0);
            mainLayout->addLayout(headerLayout);
        }

        // Set up main row: diagnostic text
        const Utf8String mainFilePath = displayHints.showFileNameInMainDiagnostic
                ? Utf8String()
                : location.filePath();
        mainLayout->addWidget(createDiagnosticLabel(diagnostic,
                                                    mainFilePath,
                                                    DoNotIndentDiagnostic,
                                                    displayHints.enableClickableFixits));

        setLayout(mainLayout);
    }
};

void addChildrenToLayout(const QString &mainFilePath,
                         const QVector<ClangBackEnd::DiagnosticContainer>::const_iterator first,
                         const QVector<ClangBackEnd::DiagnosticContainer>::const_iterator last,
                         bool enableClickableFixits,
                         QLayout &boxLayout)
{
    for (auto it = first; it != last; ++it) {
        boxLayout.addWidget(createDiagnosticLabel(*it,
                                                  mainFilePath,
                                                  IndentDiagnostic,
                                                  enableClickableFixits));
    }
}

void setupChildDiagnostics(const QString &mainFilePath,
                           const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                           bool enableClickableFixits,
                           QLayout &boxLayout)
{
    if (diagnostics.size() <= 10) {
        addChildrenToLayout(mainFilePath, diagnostics.begin(), diagnostics.end(),
                            enableClickableFixits, boxLayout);
    } else {
        addChildrenToLayout(mainFilePath, diagnostics.begin(), diagnostics.begin() + 7,
                            enableClickableFixits, boxLayout);

        auto ellipsisLabel = new QLabel(QStringLiteral("..."));
        ellipsisLabel->setContentsMargins(childIndentationOnTheLeftInPixel, 0, 0, 0);
        boxLayout.addWidget(ellipsisLabel);

        addChildrenToLayout(mainFilePath, diagnostics.end() - 3, diagnostics.end(),
                            enableClickableFixits, boxLayout);
    }
}

} // anonymous namespace

namespace ClangCodeModel {
namespace Internal {

void addToolTipToLayout(const ClangBackEnd::DiagnosticContainer &diagnostic,
                        QLayout *target,
                        const DisplayHints &displayHints)
{
    // Set up header and text row for main diagnostic
    target->addWidget(new MainDiagnosticWidget(diagnostic, displayHints));

    // Set up child rows for notes
    setupChildDiagnostics(diagnostic.location().filePath(),
                          diagnostic.children(),
                          displayHints.enableClickableFixits,
                          *target);
}

} // namespace Internal
} // namespace ClangCodeModel

#include "clangdiagnostictooltipwidget.moc"
