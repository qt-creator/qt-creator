/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef EXAMPLESLISTMODEL_H
#define EXAMPLESLISTMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStringList>
#include <QXmlStreamReader>
#include <qtsupport/baseqtversion.h>

namespace QtSupport {
namespace Internal {

class ExamplesListModel;

class ExampleSetModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum ExampleSetType {
        InvalidExampleSet,
        QtExampleSet,
        ExtraExampleSet
    };

    ExampleSetModel(ExamplesListModel *examplesModel, QObject *parent);

    void writeCurrentIdToSettings(int currentIndex) const;
    int readCurrentIndexFromSettings() const;

    int indexForQtVersion(BaseQtVersion *qtVersion) const;
    void update();

    QVariant getDisplayName(int index) const;
    QVariant getId(int index) const;
    ExampleSetType getType(int i) const;
    int getQtId(int index) const;
    int getExtraExampleSetIndex(int index) const;

private:
    QHash<int, QByteArray> roleNames() const;

    ExamplesListModel *examplesModel;
};

enum ExampleRoles
{
    Name = Qt::UserRole, ProjectPath, Description, ImageUrl,
    DocUrl, FilesToOpen, MainFile, Tags, Difficulty, HasSourceCode,
    Type, Dependencies, IsVideo, VideoUrl, VideoLength, Platforms,
    IsHighlighted
};

enum InstructionalType
{
    Example = 0, Demo, Tutorial
};

struct ExampleItem
{
    ExampleItem(): difficulty(0), isVideo(false), isHighlighted(false) {}
    QString name;
    QString projectPath;
    QString description;
    QString imageUrl;
    QString docUrl;
    QStringList filesToOpen;
    QString mainFile; /* file to be visible after opening filesToOpen */
    QStringList tags;
    QStringList dependencies;
    InstructionalType type;
    int difficulty;
    bool hasSourceCode;
    bool isVideo;
    bool isHighlighted;
    QString videoUrl;
    QString videoLength;
    QStringList platforms;
};

class ExamplesListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct ExtraExampleSet {
        QString displayName;
        QString manifestPath;
        QString examplesPath;
    };

    explicit ExamplesListModel(QObject *parent);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    void beginReset() { beginResetModel(); }
    void endReset() { endResetModel(); }

    void update();

    int selectedExampleSet() const;
    void selectExampleSet(int index);

    QList<BaseQtVersion*> qtVersions() const { return m_qtVersions; }
    QList<ExtraExampleSet> extraExampleSets() const { return m_extraExampleSets; }
    QAbstractItemModel* exampleSetModel() { return m_exampleSetModel; }

signals:
    void selectedExampleSetChanged();

private:
    void updateQtVersions();
    void updateExamples();

    void updateSelectedQtVersion();
    BaseQtVersion *findHighestQtVersion() const;

    void parseExamples(QXmlStreamReader *reader, const QString &projectsOffset,
                                     const QString &examplesInstallPath);
    void parseDemos(QXmlStreamReader *reader, const QString &projectsOffset,
                                  const QString &demosInstallPath);
    void parseTutorials(QXmlStreamReader *reader, const QString &projectsOffset);
    QStringList exampleSources(QString *examplesInstallPath, QString *demosInstallPath);

    ExampleSetModel* m_exampleSetModel;
    QList<BaseQtVersion*> m_qtVersions;
    QList<ExtraExampleSet> m_extraExampleSets;
    QList<ExampleItem> m_exampleItems;
    int m_selectedExampleSetIndex;
};

class ExamplesListModelFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    Q_PROPERTY(bool showTutorialsOnly READ showTutorialsOnly WRITE setShowTutorialsOnly NOTIFY showTutorialsOnlyChanged)
    Q_PROPERTY(QStringList filterTags READ filterTags WRITE setFilterTags NOTIFY filterTagsChanged)
    Q_PROPERTY(QStringList searchStrings READ searchStrings WRITE setSearchStrings NOTIFY searchStrings)

    Q_PROPERTY(int exampleSetIndex READ exampleSetIndex NOTIFY exampleSetIndexChanged)

    explicit ExamplesListModelFilter(ExamplesListModel *sourceModel, QObject *parent);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    bool showTutorialsOnly() { return m_showTutorialsOnly; }
    QStringList filterTags() const { return m_filterTags; }
    QStringList searchStrings() const { return m_searchString; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QAbstractItemModel* exampleSetModel();

    Q_INVOKABLE void filterForExampleSet(int index);

public slots:
    void setFilterTags(const QStringList &arg)
    {
        if (m_filterTags != arg) {
            m_filterTags = arg;
            emit filterTagsChanged(arg);
        }
    }
    void updateFilter();

    void setSearchStrings(const QStringList &arg)
    {
        if (m_searchString != arg) {
            m_searchString = arg;
            emit searchStrings(arg);
            delayedUpdateFilter();
        }
    }

    void parseSearchString(const QString &arg);
    void setShowTutorialsOnly(bool showTutorialsOnly);
    void handleQtVersionsChanged();

signals:
    void showTutorialsOnlyChanged();
    void filterTagsChanged(const QStringList &arg);
    void searchStrings(const QStringList &arg);
    void exampleSetIndexChanged();

private slots:
    void qtVersionManagerLoaded();
    void helpManagerInitialized();

private:
    void exampleDataRequested() const;
    void tryToInitialize();
    void timerEvent(QTimerEvent *event);
    void delayedUpdateFilter();
    int exampleSetIndex() const;

    bool m_showTutorialsOnly;
    QStringList m_filterTags;
    QStringList m_searchString;
    ExamplesListModel *m_sourceModel;
    int m_timerId;
    bool m_blockIndexUpdate;
    bool m_qtVersionManagerInitialized;
    bool m_helpManagerInitialized;
    bool m_initalized;
    bool m_exampleDataRequested;
};

} // namespace Internal
} // namespace QtSupport

#endif // EXAMPLESLISTMODEL_H


