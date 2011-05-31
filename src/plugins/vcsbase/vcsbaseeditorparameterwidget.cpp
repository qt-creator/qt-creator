/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "vcsbaseeditorparameterwidget.h"

#include <QtGui/QToolButton>
#include <QtGui/QHBoxLayout>

#include <QtCore/QDebug>

namespace VCSBase {

class VCSBaseEditorParameterWidgetPrivate
{
public:
    VCSBaseEditorParameterWidgetPrivate() : m_layout(0) {}

    QStringList m_baseArguments;
    QHBoxLayout *m_layout;
    QList<VCSBaseEditorParameterWidget::OptionMapping> m_optionMappings;
};

/*!
    \class VCSBase::VCSBaseEditorParameterWidget

    \brief A toolbar-like widget for use with VCSBase::VCSBaseEditor::setConfigurationWidget()
    influencing for example the generation of VCS diff output.

    The widget maintains a list of command line arguments (starting from baseArguments())
    which are set according to the state of the inside widgets. A change signal is provided
    that should trigger the rerun of the VCS operation.
*/

VCSBaseEditorParameterWidget::VCSBaseEditorParameterWidget(QWidget *parent) :
    QWidget(parent), d(new VCSBaseEditorParameterWidgetPrivate)
{
    d->m_layout = new QHBoxLayout(this);
    d->m_layout->setContentsMargins(3, 0, 3, 0);
    d->m_layout->setSpacing(2);
    connect(this, SIGNAL(argumentsChanged()), this, SLOT(handleArgumentsChanged()));
}

VCSBaseEditorParameterWidget::~VCSBaseEditorParameterWidget()
{
}

QStringList VCSBaseEditorParameterWidget::baseArguments() const
{
    return d->m_baseArguments;
}

void VCSBaseEditorParameterWidget::setBaseArguments(const QStringList &b)
{
    d->m_baseArguments = b;
}

QStringList VCSBaseEditorParameterWidget::arguments() const
{
    // Compile effective arguments
    QStringList args = baseArguments();
    foreach (const OptionMapping &mapping, optionMappings())
        args += argumentsForOption(mapping);
    return args;
}

QToolButton *VCSBaseEditorParameterWidget::addToggleButton(const QString &option,
                                                           const QString &label,
                                                           const QString &toolTip)
{
    QToolButton *tb = new QToolButton;
    tb->setText(label);
    tb->setToolTip(toolTip);
    tb->setCheckable(true);
    connect(tb, SIGNAL(toggled(bool)), this, SIGNAL(argumentsChanged()));
    d->m_layout->addWidget(tb);
    d->m_optionMappings.append(OptionMapping(option, tb));
    return tb;
}

QToolButton *VCSBaseEditorParameterWidget::addIgnoreWhiteSpaceButton(const QString &option)
{
    return addToggleButton(option, msgIgnoreWhiteSpaceLabel(), msgIgnoreWhiteSpaceToolTip());
}

QToolButton *VCSBaseEditorParameterWidget::addIgnoreBlankLinesButton(const QString &option)
{
    return addToggleButton(option, msgIgnoreBlankLinesLabel(), msgIgnoreBlankLinesToolTip());
}

QString VCSBaseEditorParameterWidget::msgIgnoreWhiteSpaceLabel()
{
    return tr("Ignore whitespace");
}

QString VCSBaseEditorParameterWidget::msgIgnoreWhiteSpaceToolTip()
{
    return tr("Ignore whitespace only changes");
}

QString VCSBaseEditorParameterWidget::msgIgnoreBlankLinesLabel()
{
    return tr("Ignore blank lines ");
}

QString VCSBaseEditorParameterWidget::msgIgnoreBlankLinesToolTip()
{
    return tr("Ignore changes in blank lines");
}

void VCSBaseEditorParameterWidget::executeCommand()
{
}

void VCSBaseEditorParameterWidget::handleArgumentsChanged()
{
    executeCommand();
}

VCSBaseEditorParameterWidget::OptionMapping::OptionMapping() :
    widget(0)
{
}

VCSBaseEditorParameterWidget::OptionMapping::OptionMapping(const QString &optName, QWidget *w) :
    optionName(optName), widget(w)
{
}

const QList<VCSBaseEditorParameterWidget::OptionMapping> &VCSBaseEditorParameterWidget::optionMappings() const
{
    return d->m_optionMappings;
}

QStringList VCSBaseEditorParameterWidget::argumentsForOption(const OptionMapping &mapping) const
{
    QStringList args;
    const QToolButton *tb = qobject_cast<const QToolButton *>(mapping.widget);
    if (tb != 0 && tb->isChecked())
        args += mapping.optionName;
    return args;
}

} // namespace VCSBase
