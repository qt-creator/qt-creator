/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "toolchainconfigwidget.h"
#include "toolchain.h"

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QString>

#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

namespace {

const char DEFAULT_MKSPEC[] = "default";

QString mkspecListToString(const QList<Utils::FileName> &specList)
{
    QStringList specStrings;
    foreach (const Utils::FileName &spec, specList) {
        if (spec.isEmpty())
            specStrings.append(QLatin1String(DEFAULT_MKSPEC));
        else
            specStrings.append(spec.toUserOutput());
    }
    QString specString = specStrings.join(QChar::fromAscii(';'));
    if (specString.isEmpty())
        return QLatin1String(DEFAULT_MKSPEC);
    return specString;
}

QList<Utils::FileName> mkspecListFromString(const QString &specString)
{
    QStringList specList = specString.split(QLatin1Char(';'));
    QList<Utils::FileName> result;
    foreach (const QString &spec, specList) {
        QString trimmed = spec.trimmed();
        if (trimmed == QLatin1String(DEFAULT_MKSPEC))
            result.append(Utils::FileName());
        else
            result.append(Utils::FileName::fromUserInput(trimmed));
    }

    if (result.size() == 1 && result.at(0).isEmpty())
        return QList<Utils::FileName>();

    return result;
}


} // namespace

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// ToolChainConfigWidgetPrivate
// --------------------------------------------------------------------------

class ToolChainConfigWidgetPrivate
{
public:
    ToolChainConfigWidgetPrivate(ToolChain *tc) :
        m_toolChain(tc), m_debuggerPathChooser(0),
        m_mkspecLayout(0), m_mkspecEdit(0), m_mkspecResetButton(0), m_mkspecEdited(false),
        m_errorLabel(0)
    {
        QTC_CHECK(tc);
    }

    ToolChain *m_toolChain;
    Utils::PathChooser *m_debuggerPathChooser;
    QHBoxLayout *m_mkspecLayout;
    QLineEdit *m_mkspecEdit;
    QPushButton *m_mkspecResetButton;
    bool m_mkspecEdited;
    QLabel *m_errorLabel;
    QList<Utils::FileName> m_suggestedMkspec;
};

} // namespace Internal

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

ToolChainConfigWidget::ToolChainConfigWidget(ToolChain *tc) :
    d(new Internal::ToolChainConfigWidgetPrivate(tc))
{
}

void ToolChainConfigWidget::setDisplayName(const QString &name)
{
    d->m_toolChain->setDisplayName(name);
}

ToolChain *ToolChainConfigWidget::toolChain() const
{
    return d->m_toolChain;
}

void ToolChainConfigWidget::makeReadOnly()
{
    if (d->m_debuggerPathChooser)
        d->m_debuggerPathChooser->setEnabled(false);
    if (d->m_mkspecEdit)
        d->m_mkspecEdit->setEnabled(false);
    if (d->m_mkspecResetButton)
        d->m_mkspecResetButton->setEnabled(false);
}

void ToolChainConfigWidget::emitDirty()
{
    if (d->m_mkspecEdit)
        d->m_mkspecEdited = (mkspecListFromString(d->m_mkspecEdit->text()) != d->m_suggestedMkspec);
    if (d->m_mkspecResetButton)
        d->m_mkspecResetButton->setEnabled(d->m_mkspecEdited);
    emit dirty();
}

void ToolChainConfigWidget::resetMkspecList()
{
    if (!d->m_mkspecEdit || !d->m_mkspecEdited)
        return;
    d->m_mkspecEdit->setText(mkspecListToString(d->m_suggestedMkspec));
    d->m_mkspecEdited = false;
}

void ToolChainConfigWidget::addDebuggerCommandControls(QFormLayout *lt,
                                                       const QStringList &versionArguments)
{
    ensureDebuggerPathChooser(versionArguments);
    lt->addRow(tr("&Debugger:"), d->m_debuggerPathChooser);
}

void ToolChainConfigWidget::addDebuggerCommandControls(QGridLayout *lt,
                                                       int row, int column,
                                                       const QStringList &versionArguments)
{
    ensureDebuggerPathChooser(versionArguments);
    QLabel *label = new QLabel(tr("&Debugger:"));
    label->setBuddy(d->m_debuggerPathChooser);
    lt->addWidget(label, row, column);
    lt->addWidget(d->m_debuggerPathChooser, row, column + 1);
}

void ToolChainConfigWidget::ensureDebuggerPathChooser(const QStringList &versionArguments)
{
    if (d->m_debuggerPathChooser)
        return;
    d->m_debuggerPathChooser = new Utils::PathChooser;
    d->m_debuggerPathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    d->m_debuggerPathChooser->setCommandVersionArguments(versionArguments);
    connect(d->m_debuggerPathChooser, SIGNAL(changed(QString)), this, SLOT(emitDirty()));
}

void ToolChainConfigWidget::addDebuggerAutoDetection(QObject *receiver, const char *autoDetectSlot)
{
    QTC_ASSERT(d->m_debuggerPathChooser, return);
    d->m_debuggerPathChooser->addButton(tr("Autodetect"), receiver, autoDetectSlot);
}

Utils::FileName ToolChainConfigWidget::debuggerCommand() const
{
    QTC_ASSERT(d->m_debuggerPathChooser, return Utils::FileName());
    return d->m_debuggerPathChooser->fileName();
}

void ToolChainConfigWidget::setDebuggerCommand(const Utils::FileName &debugger)
{
    QTC_ASSERT(d->m_debuggerPathChooser, return);
    d->m_debuggerPathChooser->setFileName(debugger);
}

void ToolChainConfigWidget::addMkspecControls(QFormLayout *lt)
{
    ensureMkspecEdit();
    lt->addRow(tr("mkspec:"), d->m_mkspecLayout);
}

void ToolChainConfigWidget::addMkspecControls(QGridLayout *lt, int row, int column)
{
    ensureMkspecEdit();
    QLabel *label = new QLabel(tr("mkspec:"));
    label->setBuddy(d->m_mkspecEdit);
    lt->addWidget(label, row, column);
    lt->addLayout(d->m_mkspecLayout, row, column + 1);
}

void ToolChainConfigWidget::ensureMkspecEdit()
{
    if (d->m_mkspecEdit)
        return;

    QTC_CHECK(!d->m_mkspecLayout);
    QTC_CHECK(!d->m_mkspecResetButton);

    d->m_suggestedMkspec = d->m_toolChain->suggestedMkspecList();

    d->m_mkspecLayout = new QHBoxLayout;
    d->m_mkspecLayout->setMargin(0);

    d->m_mkspecEdit = new QLineEdit;
    d->m_mkspecEdit->setWhatsThis(tr("All possible mkspecs separated by a semicolon (';')."));
    d->m_mkspecResetButton = new QPushButton(tr("Reset"));
    d->m_mkspecResetButton->setEnabled(d->m_mkspecEdited);
    d->m_mkspecLayout->addWidget(d->m_mkspecEdit);
    d->m_mkspecLayout->addWidget(d->m_mkspecResetButton);

    connect(d->m_mkspecEdit, SIGNAL(textChanged(QString)), this, SLOT(emitDirty()));
    connect(d->m_mkspecResetButton, SIGNAL(clicked()), this, SLOT(resetMkspecList()));
}

QList<Utils::FileName> ToolChainConfigWidget::mkspecList() const
{
    QTC_ASSERT(d->m_mkspecEdit, return QList<Utils::FileName>());

    return mkspecListFromString(d->m_mkspecEdit->text());
}

void ToolChainConfigWidget::setMkspecList(const QList<Utils::FileName> &specList)
{
    QTC_ASSERT(d->m_mkspecEdit, return);

    d->m_mkspecEdit->setText(mkspecListToString(specList));

    emitDirty();
}

void ToolChainConfigWidget::addErrorLabel(QFormLayout *lt)
{
    if (!d->m_errorLabel) {
        d->m_errorLabel = new QLabel;
        d->m_errorLabel->setVisible(false);
    }
    lt->addRow(d->m_errorLabel);
}

void ToolChainConfigWidget::addErrorLabel(QGridLayout *lt, int row, int column, int colSpan)
{
    if (!d->m_errorLabel) {
        d->m_errorLabel = new QLabel;
        d->m_errorLabel->setVisible(false);
    }
    lt->addWidget(d->m_errorLabel, row, column, 1, colSpan);
}

void ToolChainConfigWidget::setErrorMessage(const QString &m)
{
    QTC_ASSERT(d->m_errorLabel, return);
    if (m.isEmpty()) {
        clearErrorMessage();
    } else {
        d->m_errorLabel->setText(m);
        d->m_errorLabel->setStyleSheet(QLatin1String("background-color: \"red\""));
        d->m_errorLabel->setVisible(true);
    }
}

void ToolChainConfigWidget::clearErrorMessage()
{
    QTC_ASSERT(d->m_errorLabel, return);
    d->m_errorLabel->clear();
    d->m_errorLabel->setStyleSheet(QString());
    d->m_errorLabel->setVisible(false);
}

} // namespace ProjectExplorer
