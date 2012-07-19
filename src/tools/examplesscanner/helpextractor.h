/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef HELPEXTRACTOR_H
#define HELPEXTRACTOR_H

#include <QtCore>
#include <QtXml>
#include <QtHelp>

typedef QHash<QString, QString> StringHash;
typedef QHash<QString, StringHash> HashHash;

class HelpExtractor
{
public:
    HelpExtractor();
    void readXmlDocument();

private:
    void initHelpEngine();
    void convertToAggregatableXml(const QDomElement &documentElement);
    QByteArray getResource(const QString &name);
    QByteArray getHtml(const QString &name);
    QByteArray getImage(const QString &name);
    QString getImageUrl(const QString &name);
    QString resolveDocUrl(const QString &name);
    QString resolveImageUrl(const QString &name);
    QString loadDescription(const QString &name);
    void readInfoAboutExample(const QDomElement &example);
    QString extractTextFromParagraph(const QDomNode &parentNode);
    bool isSummaryOf(const QString &text, const QString &example);

    HashHash info;
    QHelpEngineCore *helpEngine;
    QDomDocument *contentsDoc;
    QString helpRootUrl;
    QDir docDir;
    QDir imgDir;
    QString lastCategory;

};

#endif // HELPEXTRACTOR_H
