/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_HTMLHIGHLIGHTER_H
#define KSYNTAXHIGHLIGHTING_HTMLHIGHLIGHTER_H

#include "abstracthighlighter.h"
#include "ksyntaxhighlighting_export.h"
#include "theme.h"

#include <QString>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
class HtmlHighlighterPrivate;

class KSYNTAXHIGHLIGHTING_EXPORT HtmlHighlighter : public AbstractHighlighter
{
public:
    HtmlHighlighter();
    ~HtmlHighlighter() override;

    void highlightFile(const QString &fileName, const QString &title = QString());
    void highlightData(QIODevice *device, const QString &title = QString());

    void setOutputFile(const QString &fileName);
    void setOutputFile(FILE *fileHandle);

    void setBackgroundRole(Theme::EditorColorRole bgRole);

protected:
    void applyFormat(int offset, int length, const Format &format) override;

private:
    Q_DECLARE_PRIVATE(HtmlHighlighter)
};
}

#endif // KSYNTAXHIGHLIGHTING_HTMLHIGHLIGHTER_H
