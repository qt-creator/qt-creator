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
