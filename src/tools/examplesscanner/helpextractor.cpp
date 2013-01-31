/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "helpextractor.h"

#include <QtWidgets>
#include <QDebug>

HelpExtractor::HelpExtractor()
{
    initHelpEngine();
}

void HelpExtractor::initHelpEngine()
{
    helpRootUrl = QString("qthelp://com.trolltech.qt/qdoc/");
    //        .arg(QT_VERSION >> 16).arg((QT_VERSION >> 8) & 0xFF)
    //        .arg(QT_VERSION & 0xFF);

    // Store help collection file in cache dir of assistant
    QString cacheDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation)
            + QLatin1String("/Trolltech/Assistant/");
    QString helpDataFile = QString(QLatin1String("qtdemo_%1.qhc")).arg(QLatin1String(QT_VERSION_STR));

    QDir dir;
    if (!dir.exists(cacheDir))
        dir.mkpath(cacheDir);

    // Create help engine (and new
    // helpDataFile if it does not exist):
    helpEngine = new QHelpEngineCore(cacheDir + helpDataFile);
    helpEngine->setupData();

    QString qtDocRoot = QLibraryInfo::location(QLibraryInfo::DocumentationPath) + QLatin1String("/qch");
    qtDocRoot = QDir(qtDocRoot).absolutePath();

    QStringList qchFiles;
    qchFiles << QLatin1String("/qt.qch")
             << QLatin1String("/designer.qch")
             << QLatin1String("/linguist.qch");

    QString oldDir = helpEngine->customValue(QLatin1String("docDir"), QString()).toString();
    if (oldDir != qtDocRoot) {
        foreach (const QString &qchFile, qchFiles)
            helpEngine->unregisterDocumentation(QHelpEngineCore::namespaceName(qtDocRoot + qchFile));
    }

    // If the data that the engine will work
    // on is not yet registered, do it now:
    foreach (const QString &qchFile, qchFiles)
        helpEngine->registerDocumentation(qtDocRoot + qchFile);

    helpEngine->setCustomValue(QLatin1String("docDir"), qtDocRoot);
}

void HelpExtractor::readXmlDocument()
{
    contentsDoc = new QDomDocument();
    QString errorStr;
    int errorLine;
    int errorColumn;

    QString qtDemoPath = QLibraryInfo::location(QLibraryInfo::DemosPath) + QLatin1String("/qtdemo");
    QFile file(qtDemoPath + "/xml/examples.xml");
    bool statusOK = contentsDoc->setContent(&file, true, &errorStr, &errorLine, &errorColumn);
    if (!statusOK) {
        qDebug() << QString::fromLatin1("DOM Parser: Could not read or find the contents document. Error at line %1, column %2:\n%3")
                    .arg(errorLine).arg(errorColumn).arg(errorStr);
        exit(-1);
    }
    //convertToSql(contentsDoc->documentElement());
    convertToAggregatableXml(contentsDoc->documentElement());
}


void HelpExtractor::convertToAggregatableXml(const QDomElement &documentElement)
{
    QDomDocument outDocument;
    QDomElement root = outDocument.createElement("instructionals");
    QDomElement demos = outDocument.createElement("demos");
    QDomElement examples = outDocument.createElement("examples");
    QDomElement tutorials = outDocument.createElement("tutorials");
    root.setAttribute("module", "Qt");
    root.appendChild(demos);
    root.appendChild(examples);
    root.appendChild(tutorials);
    outDocument.appendChild(root);

    QDomNode currentNode = documentElement.firstChild();
    QDomElement step, steps, instructional;
    int id = 0;
    while (!currentNode.isNull()){
        //qDebug() << '\t' << label;
        QDomNode sub = currentNode.firstChild();
        while (!sub.isNull()) {
            QDomElement element = sub.toElement();
            readInfoAboutExample(element);
            QString exampleName = element.attribute("name");
            StringHash exampleInfo = info[exampleName];

            // type = category - last char
            QString category = exampleInfo["category"];
            QString categoryName = sub.parentNode().toElement().attribute("name");
            QString type = category;
            type.chop(1);

            QString dirName = exampleInfo["dirname"];

            if (category != "tutorial" || (category == "tutorial" && category != lastCategory)) {
                instructional = outDocument.createElement(type);
                if (category == "tutorial" && category != lastCategory)
                    instructional.setAttribute("name", categoryName);
                else
                    instructional.setAttribute("name", exampleName);
            }

            QString projectPath = dirName + '/' + exampleInfo["filename"] + '/' + exampleInfo["filename"];

            bool qml = (exampleInfo["qml"] == "true");
            if (qml)
                projectPath += ".qmlproject";
            else
                projectPath += ".pro";


            if (category == "tutorials")
            {
                if (category != lastCategory) {
                    steps = outDocument.createElement("steps");
                    instructional.appendChild(steps);
                }

                step = outDocument.createElement("step");
                step.setAttribute("projectPath", projectPath);
                step.setAttribute("imageUrl", getImageUrl(exampleName));
                step.setAttribute("docUrl", resolveDocUrl(exampleName));
                steps.appendChild(step);
            }

            instructional.setAttribute("projectPath", projectPath);
            instructional.setAttribute("imageUrl", getImageUrl(exampleName));
            instructional.setAttribute("docUrl", resolveDocUrl(exampleName));
            instructional.setAttribute("difficulty", "?");
            QDomElement description = outDocument.createElement("description");
            QString descriptionText = loadDescription(exampleName);
            description.appendChild(outDocument.createCDATASection(descriptionText));
            QDomElement tags = outDocument.createElement("tags");
            // TODO
            QStringList tagList;
            tagList << type << exampleInfo["filename"].split('/');
            if (dirName != ".")
                tagList << dirName.split('/');
            if (qml)
                tagList << "qml" << "qt quick";
            QRegExp ttText("<tt>(.*)</tt>");
            ttText.setMinimal(true); // non-greedy
            int index = 0;
            QStringList keywords;
            while ((index = ttText.indexIn(descriptionText, index)) != -1) {
                keywords << descriptionText.mid(index+4, ttText.matchedLength()-9);
                index = index + ttText.matchedLength();
            }

            // Blacklist Checking...
            QStringList blackList;
            blackList << "license" << "trafikanten";
            foreach (const QString& keyword, keywords)
                if (!keyword.isEmpty())  {
                    bool skip = false;
                    foreach (const QString& blackListItem, blackList)
                        if (keyword.contains(blackListItem, Qt::CaseInsensitive)) {
                            skip = true;
                            break;
                        }
                    if (!skip)
                        tagList << keyword.simplified().toLower();
                }

            tags.appendChild(outDocument.createTextNode(tagList.join(",")));
            instructional.appendChild(description);
            instructional.appendChild(tags);

            if (category != "tutorials") {
                if (category == "demos")
                    demos.appendChild(instructional);
                else
                    examples.appendChild(instructional);
            } else if (lastCategory != "tutorials")
                tutorials.appendChild(instructional);

            id++;
            sub = sub.nextSibling();
            lastCategory = category;

        }
        currentNode = currentNode.nextSibling();
    }


    QFile outFile("../../../share/qtcreator/welcomescreen/examples_fallback.xml");

    QByteArray xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    xml += outDocument.toByteArray();
    if (!outFile.open(QIODevice::WriteOnly))
        return;
    outFile.write(xml);
    //qDebug() << xml;
    outFile.close();

}

QString HelpExtractor::loadDescription(const QString &name)
{
    QByteArray ba = getHtml(name);
    QString errorMsg;
    int errorLine, errorColumn;

    QDomDocument exampleDoc;
    if (ba.isEmpty())
        qDebug() << "No documentation found for" << name << "Is the documentation built?";
    else if (!exampleDoc.setContent(ba, false, &errorMsg, &errorLine, &errorColumn))
        qDebug() << "Error loading documentation for " << name << ": " << errorMsg << errorLine << errorColumn;

    QDomNodeList paragraphs = exampleDoc.elementsByTagName("p");
    if (paragraphs.length() < 1)
        qDebug() << "- ExampleContent::loadDescription(): Could not load description:"
                 << info[name]["docfile"];
    QString description = QLatin1String("");
    for (int p = 0; p < int(paragraphs.length()); ++p) {
        description = extractTextFromParagraph(paragraphs.item(p));
        if (isSummaryOf(description, name))
            break;
    }
    return description;
}

void HelpExtractor::readInfoAboutExample(const QDomElement &example)
{
    QString name = example.attribute("name");
    if (info.contains(name))
        qWarning() << "__WARNING: HelpExtractor::readInfoAboutExample: Demo/example with name"
                   << name << "appears twice in the xml-file!__";

    info[name]["filename"] = example.attribute("filename");
    QString dirName = example.parentNode().toElement().attribute("dirname");
    info[name]["dirname"] =  dirName;
    QString category;
    if (dirName.startsWith("tutorials"))
        category = "tutorials";
    else
        category = example.parentNode().toElement().tagName();

    if (category == "category")
        category = "examples";

    info[name]["category"] = category;
    info[name]["changedirectory"] = example.attribute("changedirectory");
    info[name]["image"] = example.attribute("image");
    info[name]["qml"] = example.attribute("qml");
}

QString HelpExtractor::resolveDocUrl(const QString &name)
{
    QString dirName = info[name]["dirname"];
    QString category = info[name]["category"];
    QString fileName = info[name]["filename"];

    if (category == "demos")
        return helpRootUrl + "demos-" + fileName.replace(QLatin1Char('/'), QLatin1Char('-')) + ".html";
    else
        return helpRootUrl + dirName.replace(QLatin1Char('/'), QLatin1Char('-')) + QLatin1Char('-') + fileName + ".html";
}


QString HelpExtractor::resolveImageUrl(const QString &name)
{
    return helpRootUrl + "images/" + name;
}

QByteArray HelpExtractor::getResource(const QString &name)
{
    return helpEngine->fileData(name);
}

QByteArray HelpExtractor::getHtml(const QString &name)
{
    return getResource(resolveDocUrl(name));
}

QByteArray HelpExtractor::getImage(const QString &name)
{
    QString imageName = this->info[name]["image"];
    QString category = this->info[name]["category"];
    QString fileName = this->info[name]["filename"];
    bool qml = (this->info[name]["qml"] == QLatin1String("true"));
    if (qml)
        fileName = QLatin1String("qml-") + fileName.split('/').last();

    if (imageName.isEmpty()){
        if (category == "demos")
            imageName = fileName + "-demo.png";
        else
            imageName = fileName + "-example.png";
        if ((getResource(resolveImageUrl(imageName))).isEmpty())
            imageName = fileName + ".png";
        if ((getResource(resolveImageUrl(imageName))).isEmpty())
            imageName = fileName + "example.png";
    }
    return getResource(resolveImageUrl(imageName));
}

QString HelpExtractor::getImageUrl(const QString &name)
{
    QString imageName = this->info[name]["image"];
    QString category = this->info[name]["category"];
    QString fileName = this->info[name]["filename"];
    bool qml = (this->info[name]["qml"] == QLatin1String("true"));
    if (qml)
        fileName = QLatin1String("qml-") + fileName.split('/').last();

    if (imageName.isEmpty()){
        if (category == "demos")
            imageName = fileName + "-demo.png";
        else
            imageName = fileName + "-example.png";
        if ((getResource(resolveImageUrl(imageName))).isEmpty())
            imageName = fileName + ".png";
        if ((getResource(resolveImageUrl(imageName))).isEmpty())
            imageName = fileName + "example.png";
        if ((getResource(resolveImageUrl(imageName))).isEmpty())
            return "";
    }
    return resolveImageUrl(imageName);
}

QString HelpExtractor::extractTextFromParagraph(const QDomNode &parentNode)
{
    QString description;
    QDomNode node = parentNode.firstChild();

    while (!node.isNull()) {
        QString beginTag;
        QString endTag;
        if (node.isText())
            description += node.nodeValue();
        else if (node.hasChildNodes()) {
            if (node.nodeName() == "b") {
                beginTag = "<b>";
                endTag = "</b>";
            } else if (node.nodeName() == "a") {
                beginTag = "<tt>";
                endTag = "</tt>";
            } else if (node.nodeName() == "i") {
                beginTag = "<i>";
                endTag = "</i>";
            } else if (node.nodeName() == "tt") {
                beginTag = "<tt>";
                endTag = "</tt>";
            }
            description += beginTag + extractTextFromParagraph(node) + endTag;
        }
        node = node.nextSibling();
    }

    return description;
}

bool HelpExtractor::isSummaryOf(const QString &text, const QString &example)
{
    return (!text.contains("[") &&
            text.indexOf(QRegExp(QString("(In )?((The|This) )?(%1 )?.*(tutorial|example|demo|application)").arg(example),
                                 Qt::CaseInsensitive)) != -1);
}
