/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "templateparameterbox.h"

#include <QFont>
#include <QBrush>

namespace qmt {

TemplateParameterBox::TemplateParameterBox(QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      m_parametersText(new QGraphicsSimpleTextItem(this))
{
    update();
}

TemplateParameterBox::~TemplateParameterBox()
{
}

void TemplateParameterBox::setFont(const QFont &font)
{
    if (font != m_parametersText->font()) {
        m_parametersText->setFont(font);
        update();
    }
}

void TemplateParameterBox::setTextBrush(const QBrush &brush)
{
    if (brush != m_parametersText->brush()) {
        m_parametersText->setBrush(brush);
        update();
    }
}

void TemplateParameterBox::setTemplateParameters(const QList<QString> &templateParameters)
{
    if (m_templateParameters != templateParameters) {
        m_templateParameters = templateParameters;
        updateText();
    }
}

void TemplateParameterBox::setBreakLines(bool breakLines)
{
    if (breakLines != m_breakLines) {
        m_breakLines = breakLines;
        updateText();
    }
}

void TemplateParameterBox::updateText()
{
    QString templateText;
    bool first = true;
    foreach (const QString &parameter, m_templateParameters) {
        if (!first) {
            if (m_breakLines)
                templateText += QLatin1Char('\n');
            else
                templateText += ", ";
        }
        templateText += parameter;
        first = false;
    }
    if (templateText != m_parametersText->text()) {
        m_parametersText->setText(templateText);
        update();
    }
}

void TemplateParameterBox::update()
{
    QRectF rect = m_parametersText->boundingRect();
    setRect(0, 0, rect.width() + 4, rect.height() + 3);
    m_parametersText->setPos(2, 2);
}

} // namespace qmt

