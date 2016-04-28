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

#include "documentwarningwidget.h"

#include "designmodewidget.h"

#include <coreplugin/modemanager.h>
#include <coreplugin/coreconstants.h>

#include <QLabel>
#include <QVBoxLayout>

namespace QmlDesigner {
namespace Internal {

DocumentWarningWidget::DocumentWarningWidget(DesignModeWidget *parent) :
        Utils::FakeToolTip(parent),
        m_errorMessage(new QLabel(tr("Placeholder"), this)),
        m_goToError(new QLabel(this)),
        m_designModeWidget(parent)
{
    setWindowFlags(Qt::Widget); //We only want the visual style from a ToolTip
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setAutoFillBackground(true);

    m_errorMessage->setForegroundRole(QPalette::ToolTipText);
    m_goToError->setText(tr("<a href=\"goToError\">Go to error</a>"));
    m_goToError->setForegroundRole(QPalette::Link);
    connect(m_goToError, &QLabel::linkActivated, this, [=]() {
        m_designModeWidget->textEditor()->gotoLine(m_error.line(), m_error.column() - 1);
        Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
    });

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(20);
    layout->setSpacing(5);
    layout->addWidget(m_errorMessage);
    layout->addWidget(m_goToError, 1, Qt::AlignRight);
}

void DocumentWarningWidget::setError(const RewriterError &error)
{
    m_error = error;
    QString str;
    if (error.type() == RewriterError::ParseError) {
        str = tr("%3 (%1:%2)").arg(QString::number(error.line()), QString::number(error.column()), error.description());
        m_goToError->show();
    }  else if (error.type() == RewriterError::InternalError) {
        str = tr("Internal error (%1)").arg(error.description());
        m_goToError->hide();
    }

    str.prepend(tr("Cannot open this QML document because of an error in the QML file:\n\n"));

    m_errorMessage->setText(str);
    resize(layout()->totalSizeHint());
}

} // namespace Internal
} // namespace Designer
