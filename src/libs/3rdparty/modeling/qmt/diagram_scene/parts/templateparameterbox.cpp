/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "templateparameterbox.h"

#include <QFont>
#include <QBrush>

namespace qmt {

TemplateParameterBox::TemplateParameterBox(QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      _break_lines(false),
      _parameters_text(new QGraphicsSimpleTextItem(this))
{
    update();
}

TemplateParameterBox::~TemplateParameterBox()
{
}

void TemplateParameterBox::setFont(const QFont &font)
{
    if (font != _parameters_text->font()) {
        _parameters_text->setFont(font);
        update();
    }
}

void TemplateParameterBox::setTextBrush(const QBrush &brush)
{
    if (brush != _parameters_text->brush()) {
        _parameters_text->setBrush(brush);
        update();
    }
}

void TemplateParameterBox::setTemplateParameters(const QList<QString> &template_parameters)
{
    if (_template_parameters != template_parameters) {
        _template_parameters = template_parameters;
        updateText();
    }
}

void TemplateParameterBox::setBreakLines(bool break_lines)
{
    if (break_lines != _break_lines) {
        _break_lines = break_lines;
        updateText();
    }
}

void TemplateParameterBox::updateText()
{
    QString template_text;
    bool first = true;
    foreach (const QString &parameter, _template_parameters) {
        if (!first) {
            if (_break_lines) {
                template_text += QLatin1Char('\n');
            } else {
                template_text += QStringLiteral(", ");
            }
        }
        template_text += parameter;
        first = false;
    }
    if (template_text != _parameters_text->text()) {
        _parameters_text->setText(template_text);
        update();
    }
}

void TemplateParameterBox::update()
{
    QRectF rect = _parameters_text->boundingRect();
    setRect(0, 0, rect.width() + 4, rect.height() + 3);
    _parameters_text->setPos(2, 2);
}

} // namespace qmt

