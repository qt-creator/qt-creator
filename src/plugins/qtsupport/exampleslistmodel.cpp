/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "exampleslistmodel.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QXmlStreamReader>

#include <QDebug>

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <qtsupport/qtversionmanager.h>

#include <algorithm>

using QtSupport::QtVersionManager;
using QtSupport::BaseQtVersion;

namespace QtSupport {

namespace Internal {

ExamplesListModel::ExamplesListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roleNames;
    roleNames[Name] = "name";
    roleNames[ProjectPath] = "projectPath";
    roleNames[ImageUrl] = "imageUrl";
    roleNames[Description] = "description";
    roleNames[DocUrl] = "docUrl";
    roleNames[FilesToOpen] = "filesToOpen";
    roleNames[Tags] = "tags";
    roleNames[Difficulty] = "difficulty";
    roleNames[Type] = "type";
    roleNames[HasSourceCode] = "hasSourceCode";
    roleNames[Dependencies] = "dependencies";
    roleNames[IsVideo] = "isVideo";
    roleNames[VideoUrl] = "videoUrl";
    roleNames[VideoLength] = "videoLength";
    setRoleNames(roleNames);

    connect(QtVersionManager::instance(), SIGNAL(updateExamples(QString,QString,QString)),
            SLOT(cacheExamplesPath(QString,QString,QString)));
    connect(Core::HelpManager::instance(), SIGNAL(setupFinished()),
            SLOT(helpInitialized()));
}

static inline QString fixStringForTags(const QString &string)
{
    QString returnString = string;
    returnString.remove(QLatin1String("<i>"));
    returnString.remove(QLatin1String("</i>"));
    returnString.remove(QLatin1String("<tt>"));
    returnString.remove(QLatin1String("</tt>"));
    return returnString;
}

QList<ExampleItem> ExamplesListModel::parseExamples(QXmlStreamReader* reader, const QString& projectsOffset)
{
    QList<ExampleItem> examples;
    ExampleItem item;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("example")) {
                item = ExampleItem();
                item.type = Example;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath.prepend('/');
                item.projectPath.prepend(projectsOffset);
                item.imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                item.docUrl = attributes.value(QLatin1String("docUrl")).toString();
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item.filesToOpen.append(projectsOffset + '/' + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("description")) {
                item.description =  fixStringForTags(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("dependency")) {
                item.dependencies.append(projectsOffset + '/' + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(",");
                m_tags.append(item.tags);
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("example"))
                examples.append(item);
            else if (reader->name() == QLatin1String("examples"))
                return examples;
            break;
        default: // nothing
            break;
        }
    }
    return examples;
}

QList<ExampleItem> ExamplesListModel::parseDemos(QXmlStreamReader* reader, const QString& projectsOffset)
{
    QList<ExampleItem> demos;
    ExampleItem item;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("demo")) {
                item = ExampleItem();
                item.type = Demo;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath.prepend('/');
                item.projectPath.prepend(projectsOffset);
                item.imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                item.docUrl = attributes.value(QLatin1String("docUrl")).toString();
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item.filesToOpen.append(projectsOffset + '/' + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("description")) {
                item.description =  fixStringForTags(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("dependency")) {
                item.dependencies.append(projectsOffset + '/' + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(",");
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("demo"))
                demos.append(item);
            else if (reader->name() == QLatin1String("demos"))
                return demos;
            break;
        default: // nothing
            break;
        }
    }
    return demos;
}

QList<ExampleItem> ExamplesListModel::parseTutorials(QXmlStreamReader* reader, const QString& projectsOffset)
{
    QList<ExampleItem> tutorials;
    ExampleItem item;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("tutorial")) {
                item = ExampleItem();
                item.type = Tutorial;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath.prepend('/');
                item.projectPath.prepend(projectsOffset);
                if (attributes.hasAttribute(QLatin1String("imageUrl")))
                    item.imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                if (attributes.hasAttribute(QLatin1String("docUrl")))
                    item.docUrl = attributes.value(QLatin1String("docUrl")).toString();
                if (attributes.hasAttribute(QLatin1String("isVideo")))
                    item.isVideo = attributes.value(QLatin1String("isVideo")).toString() == QLatin1String("true");
                if (attributes.hasAttribute(QLatin1String("videoUrl")))
                    item.videoUrl = attributes.value(QLatin1String("videoUrl")).toString();
                if (attributes.hasAttribute(QLatin1String("videoLength")))
                    item.videoLength = attributes.value(QLatin1String("videoLength")).toString();
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item.filesToOpen.append(projectsOffset + '/' + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("description")) {
                item.description =  fixStringForTags(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("dependency")) {
                item.dependencies.append(projectsOffset + '/' + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(",");
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("tutorial"))
                tutorials.append(item);
            else if (reader->name() == QLatin1String("tutorials"))
                return tutorials;
            break;
        default: // nothing
            break;
        }
    }

    return tutorials;
}

void ExamplesListModel::readNewsItems(const QString &examplesPath, const QString &demosPath, const QString & sourcePath)
{
    clear();
    foreach (const QString exampleSource, exampleSources()) {
        QFile exampleFile(exampleSource);
        if (!exampleFile.open(QIODevice::ReadOnly)) {
            qDebug() << Q_FUNC_INFO << "Could not open file" << exampleSource;
            return;
        }

        QFileInfo fi(exampleSource);
        QString offsetPath = fi.path();
        QDir examplesDir(offsetPath);
        QDir demosDir(offsetPath);
        if (offsetPath.startsWith(Core::ICore::instance()->resourcePath())) {
            // Try to get dir from first Qt Version, based on the Qt source directory
            // at first, since examplesPath / demosPath points at the build directory
            examplesDir = sourcePath + QLatin1String("/examples");
            demosDir = sourcePath + QLatin1String("/demos");
            // SDK case, folders might be called sth else (e.g. 'Examples' with uppercase E)
            // but examplesPath / demosPath is correct
            if (!examplesDir.exists() || !demosDir.exists()) {
                examplesDir = examplesPath;
                demosDir = demosPath;
            }
        }

        QXmlStreamReader reader(&exampleFile);
        while (!reader.atEnd())
            switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name() == QLatin1String("examples"))
                    addItems(parseExamples(&reader, examplesDir.path()));
                else if (reader.name() == QLatin1String("demos"))
                    addItems(parseDemos(&reader, demosDir.path()));
                else if (reader.name() == QLatin1String("tutorials"))
                    addItems(parseTutorials(&reader, examplesDir.path()));
                break;
            default: // nothing
                break;
            }

        if (reader.hasError())
            qDebug() << "error parsing file" <<  exampleSource << "as XML document";
    }

    m_tags.sort();
    m_tags.erase(std::unique(m_tags.begin(), m_tags.end()), m_tags.end());
    emit tagsUpdated();
}

QStringList ExamplesListModel::exampleSources() const
{
    QFileInfoList sources;
    const QStringList pattern(QLatin1String("*.xml"));

    // Read keys from SDK installer
    QSettings *settings = Core::ICore::instance()->settings(QSettings::SystemScope);
    int size = settings->beginReadArray("ExampleManifests");
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        sources.append(settings->value("Location").toString());
    }
    settings->endArray();


    bool anyQtVersionHasExamplesFolder = false;
    if (sources.isEmpty()) {
        // Try to get dir from first Qt Version
        QtVersionManager *versionManager = QtVersionManager::instance();
        foreach (BaseQtVersion *version, versionManager->validVersions()) {
            // There is no good solution for Qt 5 yet
            if (version->qtVersion().majorVersion != 4)
                continue;

            QDir examplesDir(version->examplesPath());
            if (examplesDir.exists()) {
                sources << examplesDir.entryInfoList(pattern);
                anyQtVersionHasExamplesFolder = true;
            }

            QDir demosDir(version->demosPath());
            if (demosDir.exists())
                sources << demosDir.entryInfoList(pattern);

            if (!sources.isEmpty())
                break;
        }
    }

    QString resourceDir = Core::ICore::instance()->resourcePath() + QLatin1String("/welcomescreen/");

    // Try Creator-provided XML file only
    if (sources.isEmpty() && anyQtVersionHasExamplesFolder) {
        // qDebug() << Q_FUNC_INFO << "falling through to Creator-provided XML file";
        sources << QString(resourceDir + QLatin1String("/examples_fallback.xml"));
    }

    sources <<  QString(resourceDir + QLatin1String("/qtcreator_tutorials.xml"));

    QStringList ret;
    foreach (const QFileInfo& source, sources)
        ret.append(source.filePath());

    return ret;
}

void ExamplesListModel::clear()
{
    if (exampleItems.count() > 0) {
        beginRemoveRows(QModelIndex(), 0,  exampleItems.size()-1);
        exampleItems.clear();
        endRemoveRows();
    }
    m_tags.clear();
}

void ExamplesListModel::addItems(const QList<ExampleItem> &newItems)
{
    beginInsertRows(QModelIndex(), exampleItems.size(), exampleItems.size() - 1 + newItems.size());
    exampleItems.append(newItems);
    endInsertRows();
}

int ExamplesListModel::rowCount(const QModelIndex &) const
{
    return exampleItems.size();
}

QVariant ExamplesListModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid() || index.row()+1 > exampleItems.count()) {
        qDebug() << Q_FUNC_INFO << "invalid index requested";
        return QVariant();
    }

    ExampleItem item = exampleItems.at(index.row());
    switch (role)
    {
    case Qt::DisplayRole: // for search only
        return QString(item.name + ' ' + item.tags.join(" "));
    case Name:
        return item.name;
    case ProjectPath:
        return item.projectPath;
    case Description:
        return item.description;
    case ImageUrl:
        return item.imageUrl;
    case DocUrl:
        return item.docUrl;
    case FilesToOpen:
        return item.filesToOpen;
    case Tags:
        return item.tags;
    case Difficulty:
        return item.difficulty;
    case Dependencies:
        return item.dependencies;
    case HasSourceCode:
        return item.hasSourceCode;
    case Type:
        return item.type;
    case IsVideo:
        return item.isVideo;
    case VideoUrl:
        return item.videoUrl;
    case VideoLength:
        return item.videoLength;
    default:
        qDebug() << Q_FUNC_INFO << "role type not supported";
        return QVariant();
    }

}

void ExamplesListModel::cacheExamplesPath(const QString &examplesPath, const QString &demosPath, const QString &sourcePath)
{
    m_cache = QMakePathCache(examplesPath, demosPath, sourcePath);
}

void ExamplesListModel::helpInitialized()
{
    disconnect(this, SLOT(cacheExamplesPath(QString, QString, QString)));
    connect(QtVersionManager::instance(), SIGNAL(updateExamples(QString,QString,QString)),
            SLOT(readNewsItems(QString,QString,QString)));
    readNewsItems(m_cache.examplesPath, m_cache.demosPath, m_cache.sourcePath);
}


ExamplesListModelFilter::ExamplesListModelFilter(QObject *parent) :
    QSortFilterProxyModel(parent), m_showTutorialsOnly(true)
{
    connect(this, SIGNAL(showTutorialsOnlyChanged()), SLOT(updateFilter()));
}

void ExamplesListModelFilter::updateFilter()
{
    invalidateFilter();
}

bool containsSubString(const QStringList& list, const QString& substr, Qt::CaseSensitivity cs)
{
    foreach (const QString &elem, list) {
        if (elem.contains(substr, cs))
            return true;
    }

    return false;
}

bool ExamplesListModelFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_showTutorialsOnly) {
        int type = sourceModel()->index(sourceRow, 0, sourceParent).data(Type).toInt();
        if (type != Tutorial)
            return false;
    }

    if (!m_showTutorialsOnly) {
        int type = sourceModel()->index(sourceRow, 0, sourceParent).data(Type).toInt();
        if (type != Example)
            return false;
    }

    const QStringList tags = sourceModel()->index(sourceRow, 0, sourceParent).data(Tags).toStringList();

    if (!m_filterTags.isEmpty()) {
        foreach(const QString &tag, m_filterTags) {
            if (!tags.contains(tag, Qt::CaseInsensitive))
                return false;
        }
        return true;
    }

    if (!m_searchString.isEmpty()) {
        const QString description = sourceModel()->index(sourceRow, 0, sourceParent).data(Description).toString();
        const QString name = sourceModel()->index(sourceRow, 0, sourceParent).data(Name).toString();


        foreach(const QString &subString, m_searchString) {
            bool wordMatch = false;
            wordMatch |= (bool)name.contains(subString, Qt::CaseInsensitive);
            if (wordMatch)
                continue;
            wordMatch |= containsSubString(tags, subString, Qt::CaseInsensitive);
            if (wordMatch)
                continue;
            wordMatch |= (bool)description.contains(subString, Qt::CaseInsensitive);
            if (!wordMatch)
                return false;
        }
    }

    bool ok = QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    if (!ok)
        return false;

    return true;
}

void ExamplesListModelFilter::setShowTutorialsOnly(bool showTutorialsOnly)
{
    m_showTutorialsOnly = showTutorialsOnly;
    emit showTutorialsOnlyChanged();
}

struct SearchStringLexer {
    QString code;
    const QChar *codePtr;
    QChar yychar;
    QString yytext;

    enum TokenKind {
        END_OF_STRING = 0,
        TAG,
        STRING_LITERAL,
        UNKNOWN
    };

    inline void yyinp() { yychar = *codePtr++; }

    SearchStringLexer(const QString &code)
        : code(code)
        , codePtr(code.unicode())
        , yychar(' ') { }

    int operator()() { return yylex(); }

    int yylex() {
        while (yychar.isSpace())
            yyinp(); // skip all the spaces

        yytext.clear();

        if (yychar.isNull())
            return END_OF_STRING;

        QChar ch = yychar;
        yyinp();

        switch (ch.unicode()) {
        case '"':
        case '\'':
        {
            const QChar quote = ch;
            yytext.clear();
            while (!yychar.isNull()) {
                if (yychar == quote) {
                    yyinp();
                    break;
                } if (yychar == '\\') {
                    yyinp();
                    switch (yychar.unicode()) {
                    case '"': yytext += '"'; yyinp(); break;
                    case '\'': yytext += '\''; yyinp(); break;
                    case '\\': yytext += '\\'; yyinp(); break;
                    }
                } else {
                    yytext += yychar;
                    yyinp();
                }
            }
            return STRING_LITERAL;
        }

        default:
            if (ch.isLetterOrNumber() || ch == '_') {
                yytext.clear();
                yytext += ch;
                while (yychar.isLetterOrNumber() || yychar == '_') {
                    yytext += yychar;
                    yyinp();
                }
                if (yychar == ':' && yytext == QLatin1String("tag")) {
                    yyinp();
                    return TAG;
                }
                return STRING_LITERAL;
            }
        }

        yytext += ch;
        return UNKNOWN;
    }
};

void ExamplesListModelFilter::parseSearchString(const QString &arg)
{
    QStringList tags;
    QStringList searchTerms;
    SearchStringLexer lex(arg);
    bool isTag = false;
    while (int tk = lex()) {
        if (tk == SearchStringLexer::TAG) {
            isTag = true;
            searchTerms.append(lex.yytext);
        }

        if (tk == SearchStringLexer::STRING_LITERAL) {
            if (isTag) {
                searchTerms.pop_back();
                tags.append(lex.yytext);
                isTag = false;
            } else {
                searchTerms.append(lex.yytext);
            }
        }
    }

    setSearchStrings(searchTerms);
    setFilterTags(tags);
    updateFilter();
}

} // namespace Internal
} // namespace QtSupport
