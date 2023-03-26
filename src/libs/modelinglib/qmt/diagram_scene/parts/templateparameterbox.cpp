// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    for (const QString &parameter : std::as_const(m_templateParameters)) {
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

