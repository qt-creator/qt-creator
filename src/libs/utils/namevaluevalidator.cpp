/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "namevaluevalidator.h"
#include "namevaluemodel.h"
#include "tooltip/tooltip.h"

#include <QTreeView>

namespace Utils {

NameValueValidator::NameValueValidator(QWidget *parent,
                                       Utils::NameValueModel *model,
                                       QTreeView *view,
                                       const QModelIndex &index,
                                       const QString &toolTipText)
    : QValidator(parent)
    , m_toolTipText(toolTipText)
    , m_model(model)
    , m_view(view)
    , m_index(index)
{
    m_hideTipTimer.setInterval(2000);
    m_hideTipTimer.setSingleShot(true);
    connect(&m_hideTipTimer, &QTimer::timeout, this, []() { Utils::ToolTip::hide(); });
}

QValidator::State NameValueValidator::validate(QString &in, int &pos) const
{
    Q_UNUSED(pos)
    QModelIndex idx = m_model->variableToIndex(in);
    if (idx.isValid() && idx != m_index)
        return QValidator::Intermediate;
    Utils::ToolTip::hide();
    m_hideTipTimer.stop();
    return QValidator::Acceptable;
}

void NameValueValidator::fixup(QString &input) const
{
    Q_UNUSED(input)

    QPoint pos = m_view->mapToGlobal(m_view->visualRect(m_index).topLeft());
    pos -= Utils::ToolTip::offsetFromPosition();
    Utils::ToolTip::show(pos, m_toolTipText);
    m_hideTipTimer.start();
    // do nothing
}

} // namespace Utils
