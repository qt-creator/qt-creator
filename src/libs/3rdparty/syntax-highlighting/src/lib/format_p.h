/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_FORMAT_P_H
#define KSYNTAXHIGHLIGHTING_FORMAT_P_H

#include "definitionref_p.h"
#include "textstyledata_p.h"
#include "theme.h"

#include <QSharedData>
#include <QString>

namespace KSyntaxHighlighting
{
class FormatPrivate : public QSharedData
{
public:
    FormatPrivate() = default;
    static FormatPrivate *detachAndGet(Format &format);

    TextStyleData styleOverride(const Theme &theme) const;
    void load(QXmlStreamReader &reader);

    DefinitionRef definition;
    QString name;
    TextStyleData style;
    Theme::TextStyle defaultStyle = Theme::Normal;
    quint16 id = 0;
    bool spellCheck = true;
};

}

#endif
