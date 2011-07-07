/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef EXAMPLESLISTMODEL_H
#define EXAMPLESLISTMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QtCore/QXmlStreamReader>
#include <QtGui/QSortFilterProxyModel>

namespace QtSupport {
namespace Internal {

enum ExampleRoles { Name=Qt::UserRole, ProjectPath, Description, ImageUrl,
                    DocUrl,  FilesToOpen, Tags, Difficulty, HasSourceCode, Type };

enum InstructionalType { Example=0, Demo, Tutorial };

struct ExampleItem {
    ExampleItem(): difficulty(0) {}
    InstructionalType type;
    QString name;
    QString projectPath;
    QString description;
    QString imageUrl;
    QString docUrl;
    QStringList filesToOpen;
    QStringList tags;
    int difficulty;
    bool hasSourceCode;
};

struct QMakePathCache {
    QString examplesPath;
    QString demosPath;
    QString sourcePath;
    QMakePathCache() {}
    QMakePathCache(const QString &_examplesPath, const QString &_demosPath, const QString &_sourcePath)
        : examplesPath(_examplesPath), demosPath(_demosPath), sourcePath(_sourcePath) {}
};

class ExamplesListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit ExamplesListModel(QObject *parent);
    void addItems(const QList<ExampleItem> &items);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QStringList tags() const { return m_tags; }


signals:
    void tagsUpdated();

public slots:
    void readNewsItems(const QString &examplesPath, const QString &demosPath, const QString &sourcePath);
    void cacheExamplesPath(const QString &examplesPath, const QString &demosPath, const QString &sourcePath);
    void helpInitialized();

private:
    QList<ExampleItem> parseExamples(QXmlStreamReader* reader, const QString& projectsOffset);
    QList<ExampleItem> parseDemos(QXmlStreamReader* reader, const QString& projectsOffset);
    QList<ExampleItem> parseTutorials(QXmlStreamReader* reader, const QString& projectsOffset);
    void clear();
    QStringList exampleSources() const;
    QList<ExampleItem> exampleItems;
    QStringList m_tags;
    QMakePathCache m_cache;

};

class ExamplesListModelFilter : public QSortFilterProxyModel {
    Q_OBJECT
public:
    Q_PROPERTY(bool showTutorialsOnly READ showTutorialsOnly WRITE setShowTutorialsOnly NOTIFY showTutorialsOnlyChanged)
    Q_PROPERTY(QString filterTag READ filterTag WRITE setFilterTag NOTIFY filterTagChanged)

    explicit ExamplesListModelFilter(QObject *parent);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool showTutorialsOnly() {return m_showTutorialsOnly;}

    QString filterTag() const
    {
        return m_filterTag;
    }

public slots:
    void setFilterTag(const QString& arg)
    {
        if (m_filterTag != arg) {
            m_filterTag = arg;
            emit filterTagChanged(arg);
        }
    }
    void updateFilter();

signals:
    void showTutorialsOnlyChanged();

    void filterTagChanged(const QString& arg);

private slots:
    void setShowTutorialsOnly(bool showTutorialsOnly);

private:
    bool m_showTutorialsOnly;
    QString m_filterTag;
};

} // namespace Internal
} // namespace QtSupport

#endif // EXAMPLESLISTMODEL_H


