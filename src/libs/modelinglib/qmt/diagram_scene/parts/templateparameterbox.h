// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsRectItem>

namespace qmt {

class TemplateParameterBox : public QGraphicsRectItem
{
public:
    explicit TemplateParameterBox(QGraphicsItem *parent = nullptr);
    ~TemplateParameterBox() override;

    void setFont(const QFont &font);
    void setTextBrush(const QBrush &brush);
    void setTemplateParameters(const QList<QString> &templateParameters);
    void setBreakLines(bool breakLines);

private:
    void updateText();
    void update();

    QList<QString> m_templateParameters;
    bool m_breakLines = false;
    QGraphicsSimpleTextItem *m_parametersText = nullptr;
};

} // namespace qmt
