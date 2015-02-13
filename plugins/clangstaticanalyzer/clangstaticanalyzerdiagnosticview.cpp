/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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
                                    .arg(diagnostic.description.toHtmlEscaped(),
                                         location,
                                         linkStyle);
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
                             bool withMarkup, bool withAbsolutePath)
{
    const QString filePath = location.filePath;
    const QString lineNumber = QString::number(location.line);
    const QString columnNumber = QString::number(location.column - 1);
    const QString fileAndLine = (withAbsolutePath ? filePath : QFileInfo(filePath).fileName())
            + QLatin1Char(':') + lineNumber;

    if (withMarkup) {
        return QLatin1String("in <a href=\"file://")
                + filePath + QLatin1Char(':') + lineNumber + QLatin1Char(':') + columnNumber
                + QLatin1String("\">")
                + fileAndLine
                + QLatin1String("</a>");
    } else {
        return QLatin1String("in ") + fileAndLine;
    }
}

QString createExplainingStepNumberString(int number, bool withMarkup)
{
    const int fieldWidth = 2;
    const QString result = QString::fromLatin1("%1:").arg(number, fieldWidth);
    return withMarkup
        ? QLatin1String("<code style='white-space:pre'>") + result + QLatin1String("</code>")
        : result;
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
                step.message.toHtmlEscaped());
    }
    if (!step.extendedMessage.isEmpty()) {
        lines << qMakePair(
            QCoreApplication::translate("ClangStaticAnalyzer::ExplainingStep", "Extended Message:"),
                step.extendedMessage.toHtmlEscaped());
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

QString createExplainingStepString(
        const ClangStaticAnalyzer::Internal::ExplainingStep &explainingStep,
        int number, bool withMarkup, bool withAbsolutePath)
{
    return createExplainingStepNumberString(number, withMarkup)
            + QLatin1Char(' ')
            + (withMarkup
               ? explainingStep.extendedMessage.toHtmlEscaped()
               : explainingStep.extendedMessage)
            + QLatin1Char(' ')
            + createLocationString(explainingStep.location, withMarkup, withAbsolutePath);
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
    info.errorLocation = createLocationString(diagnostic.location,
                                              /*withMarkup=*/ false,
                                              /*withAbsolutePath=*/ false);
    return info;
}

QWidget *ClangStaticAnalyzerDiagnosticDelegate::createDetailsWidget(const QFont &font,
                                                                    const QModelIndex &index,
                                                                    QWidget *parent) const
{
    QWidget *widget = new QWidget(parent);

    const Diagnostic diagnostic = index.data(Qt::UserRole).value<Diagnostic>();
    if (!diagnostic.isValid())
        return widget;

    QVBoxLayout *layout = new QVBoxLayout;

    // Add summary label
    QLabel *summaryLineLabel = createSummaryLabel(diagnostic);
    connect(summaryLineLabel, &QLabel::linkActivated,
            this, &ClangStaticAnalyzerDiagnosticDelegate::openLinkInEditor);
    layout->addWidget(summaryLineLabel);

    // Add labels for explaining steps
    int explainingStepNumber = 1;
    foreach (const ExplainingStep &explainingStep, diagnostic.explainingSteps) {
        const QString text = createExplainingStepString(explainingStep,
                                                        explainingStepNumber++,
                                                        /*withMarkup=*/ true,
                                                        /*withAbsolutePath=*/ false);
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

QString ClangStaticAnalyzerDiagnosticDelegate::textualRepresentation() const
{
    QTC_ASSERT(m_detailsIndex.isValid(), return QString());

    const Diagnostic diagnostic = m_detailsIndex.data(Qt::UserRole).value<Diagnostic>();
    QTC_ASSERT(diagnostic.isValid(), return QString());

    // Create summary
    QString clipboardText = diagnostic.category + QLatin1String(": ") + diagnostic.type;
    if (diagnostic.type != diagnostic.description)
        clipboardText += QLatin1String(": ") + diagnostic.description;
    clipboardText += QLatin1Char('\n');

    // Create explaining steps
    int explainingStepNumber = 1;
    foreach (const ExplainingStep &explainingStep, diagnostic.explainingSteps) {
        clipboardText += createExplainingStepString(explainingStep,
                                                    explainingStepNumber++,
                                                    /*withMarkup=*/ false,
                                                    /*withAbsolutePath=*/ true) + QLatin1Char('\n');
    }

    clipboardText.chop(1); // Remove \n
    return clipboardText;
}

ClangStaticAnalyzerDiagnosticView::ClangStaticAnalyzerDiagnosticView(QWidget *parent)
    : Analyzer::DetailedErrorView(parent)
{
    ClangStaticAnalyzerDiagnosticDelegate *delegate
            = new ClangStaticAnalyzerDiagnosticDelegate(this);
    setItemDelegate(delegate);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
