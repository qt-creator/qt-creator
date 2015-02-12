/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "messageoutputwindow.h"
#include "outputwindow.h"
#include "icontext.h"
#include "coreconstants.h"
#include "find/basetextfind.h"

#include <aggregation/aggregate.h>

namespace Core {
namespace Internal {

MessageOutputWindow::MessageOutputWindow()
{
    m_widget = new OutputWindow(Context(Constants::C_GENERAL_OUTPUT_PANE));
    m_widget->setReadOnly(true);
    // Let selected text be colored as if the text edit was editable,
    // otherwise the highlight for searching is too light
    QPalette p = m_widget->palette();
    QColor activeHighlight = p.color(QPalette::Active, QPalette::Highlight);
    p.setColor(QPalette::Highlight, activeHighlight);
    QColor activeHighlightedText = p.color(QPalette::Active, QPalette::HighlightedText);
    p.setColor(QPalette::HighlightedText, activeHighlightedText);
    m_widget->setPalette(p);
    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_widget);
    agg->add(new BaseTextFind(m_widget));
}

MessageOutputWindow::~MessageOutputWindow()
{
    delete m_widget;
}

bool MessageOutputWindow::hasFocus() const
{
    return m_widget->window()->focusWidget() == m_widget;
}

bool MessageOutputWindow::canFocus() const
{
    return true;
}

void MessageOutputWindow::setFocus()
{
    m_widget->setFocus();
}

void MessageOutputWindow::clearContents()
{
    m_widget->clear();
}

QWidget *MessageOutputWindow::outputWidget(QWidget *parent)
{
    m_widget->setParent(parent);
    return m_widget;
}

QString MessageOutputWindow::displayName() const
{
    return tr("General Messages");
}

void MessageOutputWindow::visibilityChanged(bool /*b*/)
{
}

void MessageOutputWindow::append(const QString &text)
{
    m_widget->appendText(text);
}

int MessageOutputWindow::priorityInStatusBar() const
{
    return -1;
}

bool MessageOutputWindow::canNext() const
{
    return false;
}

bool MessageOutputWindow::canPrevious() const
{
    return false;
}

void MessageOutputWindow::goToNext()
{

}

void MessageOutputWindow::goToPrev()
{

}

bool MessageOutputWindow::canNavigate() const
{
    return false;
}

} // namespace Internal
} // namespace Core
