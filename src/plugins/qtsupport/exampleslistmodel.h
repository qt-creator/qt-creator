/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef EXAMPLESLISTMODEL_H
#define EXAMPLESLISTMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QXmlStreamReader>
#include <QSortFilterProxyModel>

namespace QtSupport {
namespace Internal {

enum ExampleRoles { Name=Qt::UserRole, ProjectPath, Description, ImageUrl,
                    DocUrl,  FilesToOpen, Tags, Difficulty, HasSourceCode,
                    Type, Dependencies, IsVideo, VideoUrl, VideoLength, Platforms };

enum InstructionalType { Example=0, Demo, Tutorial };

struct ExampleItem {
    ExampleItem(): difficulty(0), isVideo(false) {}
    InstructionalType type;
    QString name;
    QString projectPath;
    QString description;
    QString imageUrl;
    QString docUrl;
    QStringList filesToOpen;
    QStringList tags;
    QStringList dependencies;
    int difficulty;
    bool hasSourceCode;
    bool isVideo;
    QString videoUrl;
    QString videoLength;
    QStringList platforms;
};

class ExamplesListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit ExamplesListModel(QObject *parent);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QStringList tags() const;


    void ensureInitialized() const;

    void beginReset()
    { beginResetModel(); }

    void endReset()
    { endResetModel(); }

signals:
    void tagsUpdated();

public slots:
    void handleQtVersionsChanged();
    void updateExamples();
    void helpInitialized();

private:
    void addItems(const QList<ExampleItem> &items);
    QList<ExampleItem> parseExamples(QXmlStreamReader* reader, const QString& projectsOffset);
    QList<ExampleItem> parseDemos(QXmlStreamReader* reader, const QString& projectsOffset);
    QList<ExampleItem> parseTutorials(QXmlStreamReader* reader, const QString& projectsOffset);
    void clear();
    QStringList exampleSources(QString *examplesFallback, QString *demosFallback,
                               QString *sourceFallback);
    QList<ExampleItem> exampleItems;
    QStringList m_tags;
    bool m_updateOnQtVersionsChanged;
    bool m_initialized;
    bool m_helpInitialized;
};

class ExamplesListModelFilter : public QSortFilterProxyModel {
    Q_OBJECT
public:
    Q_PROPERTY(bool showTutorialsOnly READ showTutorialsOnly WRITE setShowTutorialsOnly NOTIFY showTutorialsOnlyChanged)
    Q_PROPERTY(QStringList filterTags READ filterTags WRITE setFilterTags NOTIFY filterTagsChanged)
    Q_PROPERTY(QStringList searchStrings READ searchStrings WRITE setSearchStrings NOTIFY searchStrings)

    explicit ExamplesListModelFilter(ExamplesListModel *sourceModel, QObject *parent);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    bool showTutorialsOnly() {return m_showTutorialsOnly;}
    QStringList filterTags() const { return m_filterTags; }
    QStringList searchStrings() const { return m_searchString; }

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

public slots:
    void setFilterTags(const QStringList& arg)
    {
        if (m_filterTags != arg) {
            m_filterTags = arg;
            emit filterTagsChanged(arg);
        }
    }
    void updateFilter();

    void setSearchStrings(const QStringList& arg)
    {
        if (m_searchString != arg) {
            m_searchString = arg;
            emit searchStrings(arg);
            delayedUpdateFilter();
        }
    }

    void parseSearchString(const QString& arg);
    void setShowTutorialsOnly(bool showTutorialsOnly);

signals:
    void showTutorialsOnlyChanged();

    void filterTagsChanged(const QStringList& arg);

    void searchStrings(const QStringList& arg);

protected:
    void timerEvent(QTimerEvent *event);

private:
    void delayedUpdateFilter();

    bool m_showTutorialsOnly;
    QStringList m_filterTags;
    QStringList m_searchString;
    ExamplesListModel *m_sourceModel;
    int m_timerId;
};

} // namespace Internal
} // namespace QtSupport

#endif // EXAMPLESLISTMODEL_H


