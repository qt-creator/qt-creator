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

void openEditorAt(const ClangBackEnd::SourceLocationContainer &location)
{
    Core::EditorManager::openEditorAt(location.filePath().toString(),
                                      int(location.line()),
                                      int(location.column() - 1));
}

void applyFixit(const ClangBackEnd::SourceLocationContainer &location,
                const QVector<ClangBackEnd::FixItContainer> &fixits)
{
    ClangCodeModel::ClangFixItOperation operation(location.filePath(), Utf8String(), fixits);

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

class MainDiagnosticWidget : public QWidget
{
    Q_OBJECT
public:
    MainDiagnosticWidget(const ClangBackEnd::DiagnosticContainer &diagnostic)
    {
        setContentsMargins(0, 0, 0, 0);
        auto *mainLayout = createLayout<QVBoxLayout>();

        // Set up header row: category + responsible option
        const QString category = diagnostic.category();
        const QString diagnosticText = diagnostic.text().toString().toHtmlEscaped();
        const QString responsibleOption = diagnostic.enableOption();
        const ClangBackEnd::SourceLocationContainer location = diagnostic.location();

        auto *headerLayout = createLayout<QHBoxLayout>();
        headerLayout->addWidget(new QLabel(wrapInBoldTags(category)), 1);

        auto *responsibleOptionLabel = new QLabel(wrapInColor(responsibleOption, "gray"));
        headerLayout->addWidget(responsibleOptionLabel, 0);
        mainLayout->addLayout(headerLayout);

        // Set up main row: diagnostic text
        const QString text = clickableLocation(location.filePath(), location)
                             + QStringLiteral(": ")
                             + diagnosticText;
        auto *mainTextLabel = new QLabel(text);
        mainTextLabel->setTextFormat(Qt::RichText);
        QObject::connect(mainTextLabel, &QLabel::linkActivated, [location](const QString &) {
            openEditorAt(location);
            Utils::ToolTip::hideImmediately();
        });
        mainLayout->addWidget(mainTextLabel);

        setLayout(mainLayout);
    }
};

QString clickableFixIt(const QString &text, bool hasFixIt)
{
    if (!hasFixIt)
        return text;

    const QString notePrefix = QStringLiteral("note: ");

    QString modifiedText = text;
    if (modifiedText.startsWith(notePrefix))
        modifiedText = modifiedText.mid(notePrefix.size());

    return notePrefix + wrapInLink(modifiedText, QLatin1String(LINK_ACTION_APPLY_FIX));
}

QWidget *createChildDiagnosticLabel(const ClangBackEnd::DiagnosticContainer &diagnostic,
                                    const QString &mainFilePath)
{
    const bool hasFixit = !diagnostic.fixIts().isEmpty();
    const QString diagnosticText = diagnostic.text().toString().toHtmlEscaped();
    const QString text = clickableLocation(mainFilePath, diagnostic.location())
                       + QStringLiteral(": ")
                       + clickableFixIt(diagnosticText, hasFixit);
    const ClangBackEnd::SourceLocationContainer location = diagnostic.location();
    const QVector<ClangBackEnd::FixItContainer> fixits = diagnostic.fixIts();

    auto *label = new QLabel(text);
    label->setContentsMargins(10, 0, 0, 0); // indent
    label->setTextFormat(Qt::RichText);
    QObject::connect(label, &QLabel::linkActivated, [location, fixits](const QString &action) {
        if (action == QLatin1String(LINK_ACTION_APPLY_FIX))
            applyFixit(location, fixits);
        else
            openEditorAt(location);

        Utils::ToolTip::hideImmediately();
    });

    return label;
}

} // anonymous namespace

namespace ClangCodeModel {
namespace Internal {

ClangDiagnosticToolTipWidget::ClangDiagnosticToolTipWidget(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
        QWidget *parent)
    : Utils::FakeToolTip(parent)
{
    auto *mainLayout = createLayout<QVBoxLayout>();

    foreach (const auto &diagnostic, diagnostics) {
        // Set up header and text row of main diagnostic
        mainLayout->addWidget(new MainDiagnosticWidget(diagnostic));

        // Set up child rows
        const QString mainFilePath = diagnostic.location().filePath();
        foreach (const auto &childDiagnostic, diagnostic.children())
            mainLayout->addWidget(createChildDiagnosticLabel(childDiagnostic, mainFilePath));
    }

    setLayout(mainLayout);
}

} // namespace Internal
} // namespace ClangCodeModel

#include "clangdiagnostictooltipwidget.moc"
