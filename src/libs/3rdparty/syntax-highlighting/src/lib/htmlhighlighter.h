/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_HTMLHIGHLIGHTER_H
#define KSYNTAXHIGHLIGHTING_HTMLHIGHLIGHTER_H

#include "abstracthighlighter.h"
#include "ksyntaxhighlighting_export.h"

#include <QIODevice>
#include <QString>

#include <memory>

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

protected:
    void applyFormat(int offset, int length, const Format &format) override;

private:
    std::unique_ptr<HtmlHighlighterPrivate> d;
};
}

#endif // KSYNTAXHIGHLIGHTING_HTMLHIGHLIGHTER_H
