/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerdiagnosticview.h"

#include "clangstaticanalyzerlogfilereader.h"
#include "clangstaticanalyzerutils.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QLabel>
#include <QVBoxLayout>

using namespace Analyzer;

namespace {

QLabel *createCommonLabel()
{
    QLabel *label = new QLabel;
    label->setWordWrap(true);
    label->setContentsMargins(0, 0, 0, 0);
    label->setMargin(0);
    label->setIndent(10);
    return label;
}

QString createSummaryText(const ClangStaticAnalyzer::Internal::Diagnostic &diagnostic,
                          const QPalette &palette)
{
    const QColor color = palette.color(QPalette::Text);
    const QString linkStyle = QString::fromLatin1("style=\"color:rgba(%1, %2, %3, %4);\"")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(int(0.7 * 255));
    const QString fileName = QFileInfo(diagnostic.location.filePath).fileName();
    const QString location = fileName + QLatin1Char(' ')
            + QString::number(diagnostic.location.line);
    return QString::fromLatin1("%1&nbsp;&nbsp;<span %3>%2</span>")
                                    .arg(diagnostic.description, location, linkStyle);
}

QLabel *createSummaryLabel(const ClangStaticAnalyzer::Internal::Diagnostic &diagnostic)
{
    QLabel *label = createCommonLabel();
    QPalette palette = label->palette();
    palette.setBrush(QPalette::Text, palette.highlightedText());
    label->setPalette(palette);
    label->setText(createSummaryText(diagnostic, palette));
    return label;
}

QLabel *createExplainingStepLabel(const QFont &font, bool useAlternateRowPalette)
{
    QLabel *label = createCommonLabel();

    // Font
    QFont fixedPitchFont = font;
    fixedPitchFont.setFixedPitch(true);
    label->setFont(fixedPitchFont);

    // Background
    label->setAutoFillBackground(true);
    if (useAlternateRowPalette) {
        QPalette p = label->palette();
        p.setBrush(QPalette::Base, p.alternateBase());
        label->setPalette(p);
    }

    return label;
}

QString createLocationString(const ClangStaticAnalyzer::Internal::Location &location,
                             bool withMarkup)
{
    const QString filePath = location.filePath;
    const QString fileName = QFileInfo(filePath).fileName();
    const QString lineNumber = QString::number(location.line);
    const QString columnNumber = QString::number(location.column - 1);
    const QString fileNameAndLine = fileName + QLatin1Char(':') + lineNumber;

    if (withMarkup) {
        return QLatin1String("in <a href=\"file://")
                + filePath + QLatin1Char(':') + lineNumber + QLatin1Char(':') + columnNumber
                + QLatin1String("\">")
                + fileNameAndLine
                + QLatin1String("</a>");
    } else {
        return QLatin1String("in ") + fileNameAndLine;
    }
}

QString createExplainingStepNumberString(int number)
{
    const int fieldWidth = 2;
    return QString::fromLatin1("<code style='white-space:pre'>%1:</code>").arg(number, fieldWidth);
}

QString createExplainingStepToolTipString(const ClangStaticAnalyzer::Internal::ExplainingStep &step)
{
    if (step.message == step.extendedMessage)
        return createFullLocationString(step.location);

    typedef QPair<QString, QString> StringPair;
    QList<StringPair> lines;

    if (!step.message.isEmpty()) {
        lines << qMakePair(
            QCoreApplication::translate("ClangStaticAnalyzer::ExplainingStep", "Message:"),
                step.message);
    }
    if (!step.extendedMessage.isEmpty()) {
        lines << qMakePair(
            QCoreApplication::translate("ClangStaticAnalyzer::ExplainingStep", "Extended Message:"),
                step.extendedMessage);
    }

    lines << qMakePair(
        QCoreApplication::translate("ClangStaticAnalyzer::ExplainingStep", "Location:"),
                createFullLocationString(step.location));

    QString html = QLatin1String("<html>"
                   "<head>"
                   "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>\n"
                   "<body><dl>");

    foreach (const StringPair &pair, lines) {
        html += QLatin1String("<dt>");
        html += pair.first;
        html += QLatin1String("</dt><dd>");
        html += pair.second;
        html += QLatin1String("</dd>\n");
    }
    html += QLatin1String("</dl></body></html>");
    return html;
}

} // anonymous namespace

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerDiagnosticDelegate::ClangStaticAnalyzerDiagnosticDelegate(QListView *parent)
    : DetailedErrorDelegate(parent)
{
}

DetailedErrorDelegate::SummaryLineInfo ClangStaticAnalyzerDiagnosticDelegate::summaryInfo(
        const QModelIndex &index) const
{
    const Diagnostic diagnostic = index.data(Qt::UserRole).value<Diagnostic>();
    QTC_ASSERT(diagnostic.isValid(), return SummaryLineInfo());

    DetailedErrorDelegate::SummaryLineInfo info;
    info.errorText = diagnostic.description;
    info.errorLocation = createLocationString(diagnostic.location, /*withMarkup=*/ false);
    return info;
}

void ClangStaticAnalyzerDiagnosticDelegate::copy()
{
    qDebug() << Q_FUNC_INFO;
}

QWidget *ClangStaticAnalyzerDiagnosticDelegate::createDetailsWidget(const QFont &font,
                                                                    const QModelIndex &index,
                                                                    QWidget *parent) const
{
    QWidget *widget = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout;

    const Diagnostic diagnostic = index.data(Qt::UserRole).value<Diagnostic>();
    if (!diagnostic.isValid())
        return widget;

    // Add summary label
    QLabel *summaryLineLabel = createSummaryLabel(diagnostic);
    connect(summaryLineLabel, &QLabel::linkActivated,
            this, &ClangStaticAnalyzerDiagnosticDelegate::openLinkInEditor);
    layout->addWidget(summaryLineLabel);

    // Add labels for explaining steps
    int explainingStepNumber = 1;
    foreach (const ExplainingStep &explainingStep, diagnostic.explainingSteps) {
        const QString text = createExplainingStepNumberString(explainingStepNumber++)
                + QLatin1Char(' ')
                + explainingStep.extendedMessage
                + QLatin1Char(' ')
                + createLocationString(explainingStep.location, /*withMarkup=*/ true);

        QLabel *label = createExplainingStepLabel(font, explainingStepNumber % 2 == 0);
        label->setParent(widget);
        label->setText(text);
        label->setToolTip(createExplainingStepToolTipString(explainingStep));
        connect(label, &QLabel::linkActivated,
                this, &ClangStaticAnalyzerDiagnosticDelegate::openLinkInEditor);
        layout->addWidget(label);
    }

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    widget->setLayout(layout);
    return widget;
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
