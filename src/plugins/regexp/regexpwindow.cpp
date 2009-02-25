/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "regexpwindow.h"
#include "settings.h"

#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMenu>
#include <QtGui/QInputDialog>

using namespace RegExp::Internal;

RegExpWindow::RegExpWindow(QWidget *parent) :
   QWidget(parent),
   patternLabel(new QLabel(tr("&Pattern:"))),
   escapedPatternLabel(new QLabel(tr("&Escaped Pattern:"))),
   syntaxLabel(new QLabel(tr("&Pattern Syntax:"))),
   textLabel(new QLabel(tr("&Text:"))),
   patternComboBox (new QComboBox),
   escapedPatternLineEdit(new QLineEdit),
   textComboBox(new QComboBox),
   caseSensitiveCheckBox(new QCheckBox(tr("Case &Sensitive"))),
   minimalCheckBox(new QCheckBox(tr("&Minimal"))),
   syntaxComboBox(new QComboBox),
   indexLabel(new QLabel(tr("Index of Match:"))),
   matchedLengthLabel(new QLabel(tr("Matched Length:"))),
   indexEdit(new QLineEdit),
   matchedLengthEdit(new QLineEdit)
{
    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    QGridLayout *mainLayout = new QGridLayout;

    patternComboBox->setEditable(true);
    patternComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    patternLabel->setBuddy(patternComboBox);

    mainLayout->addWidget(patternLabel, 0, 0);
    mainLayout->addWidget(patternComboBox, 0, 1);

    escapedPatternLineEdit->setReadOnly(true);
    QPalette palette = escapedPatternLineEdit->palette();
    palette.setBrush(QPalette::Base, palette.brush(QPalette::Disabled, QPalette::Base));
    escapedPatternLineEdit->setPalette(palette);

    escapedPatternLabel->setBuddy(escapedPatternLineEdit);

    mainLayout->addWidget(escapedPatternLabel, 1, 0);
    mainLayout->addWidget(escapedPatternLineEdit, 1, 1);

    syntaxComboBox->addItem(tr("Regular expression v1"), QRegExp::RegExp);
    syntaxComboBox->addItem(tr("Regular expression v2"), QRegExp::RegExp2);
    syntaxComboBox->addItem(tr("Wildcard"), QRegExp::Wildcard);
    syntaxComboBox->addItem(tr("Fixed string"), QRegExp::FixedString);

    syntaxLabel->setBuddy(syntaxComboBox);

    mainLayout->addWidget(syntaxLabel, 2, 0);
    mainLayout->addWidget(syntaxComboBox, 2, 1);

    QHBoxLayout *checkBoxLayout = new QHBoxLayout;

    caseSensitiveCheckBox->setChecked(true);

    checkBoxLayout->addWidget(caseSensitiveCheckBox);
    checkBoxLayout->addWidget(minimalCheckBox);
    checkBoxLayout->addStretch(1);

    mainLayout->addLayout(checkBoxLayout, 3, 0, 1, 2);

    textComboBox->setEditable(true);
    textComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    textLabel->setBuddy(textComboBox);

    mainLayout->addWidget(textLabel, 4, 0);
    mainLayout->addWidget(textComboBox, 4, 1);

    indexEdit->setReadOnly(true);

    mainLayout->addWidget(indexLabel, 5, 0);
    mainLayout->addWidget(indexEdit, 5, 1);

    matchedLengthEdit->setReadOnly(true);

    mainLayout->addWidget(matchedLengthLabel, 6, 0);
    mainLayout->addWidget(matchedLengthEdit, 6, 1);

    vboxLayout->addLayout(mainLayout);
    vboxLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    for (int i = 0; i < MaxCaptures; ++i) {
        captureLabels[i] = new QLabel(tr("Capture %1:").arg(i));
        captureEdits[i] = new QLineEdit;
        captureEdits[i]->setReadOnly(true);
    }
    captureLabels[0]->setText(tr("Match:"));

    for (int j = 0; j < MaxCaptures; ++j) {
        mainLayout->addWidget(captureLabels[j], 7 + j, 0);
        mainLayout->addWidget(captureEdits[j], 7 + j, 1);
    }

    connect(patternComboBox, SIGNAL(editTextChanged(const QString &)), this, SLOT(refresh()));
    connect(textComboBox, SIGNAL(editTextChanged(const QString &)), this, SLOT(refresh()));
    connect(caseSensitiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(refresh()));
    connect(minimalCheckBox, SIGNAL(toggled(bool)), this, SLOT(refresh()));
    connect(syntaxComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(refresh()));

    setWindowTitle(tr("Regular Expression"));
    refresh();
}

static const char *escapedBackSlash = "\\\\";
static const char *escapedDoubleQuote = "\\\"";

static QString escapePattern(const QString &pattern)
{
    QString escaped = pattern;
    escaped.replace(QString(QLatin1Char('\\')) , QLatin1String(escapedBackSlash));
    const QChar doubleQuote(QLatin1Char('"'));
    escaped.replace(doubleQuote, QString(QLatin1String(escapedDoubleQuote)));
    escaped.prepend(doubleQuote);
    escaped.append(doubleQuote);
    return escaped;
}

static QString unescapePattern(QString escaped)
{
    // remove quotes
    const QChar doubleQuote(QLatin1Char('"'));
    if (escaped.endsWith(doubleQuote))
        escaped.truncate(escaped.size() - 1);
    if (escaped.startsWith(doubleQuote))
        escaped.remove(0, 1);

    const int size = escaped.size();
    if (!size)
        return QString();

    // parse out escapes. Do not just replace.
    QString pattern;
    const QChar backSlash = QLatin1Char('\\');
    bool escapeSeen = false;
    for (int  i = 0; i < size; i++) {
        const QChar c = escaped.at(i);
        if (c == backSlash && !escapeSeen)
            escapeSeen = true;
        else {
            pattern.push_back(c);
            escapeSeen = false;
        }
    }
    return pattern;
}

void RegExpWindow::refresh()
{
    setUpdatesEnabled(false);

    const QString pattern = patternComboBox->currentText();
    const QString text = textComboBox->currentText();

    escapedPatternLineEdit->setText(escapePattern(pattern));

    QRegExp rx(pattern);
    const Qt::CaseSensitivity cs = caseSensitiveCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    rx.setCaseSensitivity(cs);
    rx.setMinimal(minimalCheckBox->isChecked());
    const QRegExp::PatternSyntax syntax = QRegExp::PatternSyntax(
            syntaxComboBox->itemData(syntaxComboBox->currentIndex()).toInt());
    rx.setPatternSyntax(syntax);

    QPalette palette = patternComboBox->palette();
    if (rx.isValid()) {
        palette.setColor(QPalette::Text,
                         textComboBox->palette().color(QPalette::Text));
    } else {
        palette.setColor(QPalette::Text, Qt::red);
    }
    patternComboBox->setPalette(palette);

    indexEdit->setText(QString::number(rx.indexIn(text)));
    matchedLengthEdit->setText(QString::number(rx.matchedLength()));
    for (int i = 0; i < MaxCaptures; ++i) {
        const bool enabled = i <= rx.numCaptures();
        captureLabels[i]->setEnabled(enabled);
        captureEdits[i]->setEnabled(enabled);
        captureEdits[i]->setText(rx.cap(i));
    }

    setUpdatesEnabled(true);
}

static void saveTextCombo(const QComboBox *cb, QString &current, QStringList &items)
{
    current =  cb->currentText();
    items.clear();
    if (const int count = cb->count())
        for (int i = 0;i <  count; i++) {
            const QString text = cb->itemText(i);
            if (items.indexOf(text) == -1)
                items += text;
        }
}

Settings RegExpWindow::settings() const
{
    Settings rc;
    rc.m_patternSyntax = static_cast<QRegExp::PatternSyntax>(syntaxComboBox->itemData(syntaxComboBox->currentIndex()).toInt());
    rc.m_minimal = minimalCheckBox->checkState() == Qt::Checked;
    rc.m_caseSensitive = caseSensitiveCheckBox->checkState() == Qt::Checked;
    saveTextCombo(patternComboBox, rc.m_currentPattern, rc.m_patterns);
    saveTextCombo(textComboBox,  rc.m_currentMatch, rc.m_matches);
    return rc;
}

static void restoreTextCombo(const QString &current, const QStringList &items, QComboBox *cb)
{
    cb->clear();
    cb->addItems(items);
    cb->lineEdit()->setText(current);
}

void RegExpWindow::setSettings(const Settings &s)
{
    const int patternIndex = syntaxComboBox->findData(QVariant(s.m_patternSyntax));
    syntaxComboBox->setCurrentIndex(patternIndex);
    minimalCheckBox->setCheckState(s.m_minimal ? Qt::Checked : Qt::Unchecked);
    caseSensitiveCheckBox->setCheckState(s.m_caseSensitive ? Qt::Checked : Qt::Unchecked);
    restoreTextCombo(s.m_currentPattern, s.m_patterns, patternComboBox);
    restoreTextCombo(s.m_currentMatch, s.m_matches, textComboBox);
}

void RegExpWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *enterQuotedAction = menu.addAction(tr("Enter pattern from code..."));
    connect(enterQuotedAction, SIGNAL(triggered()), this, SLOT(enterEscaped()));
    menu.addSeparator();

    QAction *clearPatternsAction = menu.addAction(tr("Clear patterns"));
    connect(clearPatternsAction, SIGNAL(triggered()), patternComboBox, SLOT(clear()));

    QAction *clearTextsAction = menu.addAction(tr("Clear texts"));
    connect(clearTextsAction, SIGNAL(triggered()), textComboBox, SLOT(clear()));

    event->accept();
    menu.exec(event->globalPos());
}

void  RegExpWindow::enterEscaped()
{
    const QString escapedPattern = QInputDialog::getText (this, tr("Enter pattern from code"), tr("Pattern"));
    if ( escapedPattern.isEmpty())
        return;
    patternComboBox->lineEdit()->setText(unescapePattern(escapedPattern));

}
