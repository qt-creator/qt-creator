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

#pragma once

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
    ExamplesListModel *examplesModel;
};

enum InstructionalType
{
    Example = 0, Demo, Tutorial
};

class ExampleItem
{
public:
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
    int difficulty = 0;
    bool hasSourceCode = false;
    bool isVideo = false;
    bool isHighlighted = false;
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

    int rowCount(const QModelIndex &parent = QModelIndex()) const final;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;

    void beginReset() { beginResetModel(); }
    void endReset() { endResetModel(); }

    void update();

    int selectedExampleSet() const;
    void selectExampleSet(int index);

    QList<BaseQtVersion*> qtVersions() const { return m_qtVersions; }
    QList<ExtraExampleSet> extraExampleSets() const { return m_extraExampleSets; }
    QStringList exampleSets() const;

signals:
    void selectedExampleSetChanged(int);

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
    int m_selectedExampleSetIndex = -1;
};

class ExamplesListModelFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ExamplesListModelFilter(ExamplesListModel *sourceModel, bool showTutorialsOnly, QObject *parent);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const final;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;

    void setSearchString(const QString &arg);
    void setFilterTags(const QStringList &arg);
    void updateFilter();

    void handleQtVersionsChanged();

signals:
    void filterTagsChanged(const QStringList &arg);
    void searchStringChanged(const QString &arg);

private:
    void setFilterStrings(const QStringList &arg);

    void qtVersionManagerLoaded();
    void helpManagerInitialized();

    void exampleDataRequested() const;
    void tryToInitialize();
    void timerEvent(QTimerEvent *event);
    void delayedUpdateFilter();

    const bool m_showTutorialsOnly;
    QString m_searchString;
    QStringList m_filterTags;
    QStringList m_filterStrings;
    ExamplesListModel *m_sourceModel;
    int m_timerId = 0;
    bool m_qtVersionManagerInitialized = false;
    bool m_helpManagerInitialized = false;
    bool m_initalized = false;
    bool m_exampleDataRequested = false;
};

} // namespace Internal
} // namespace QtSupport

Q_DECLARE_METATYPE(QtSupport::Internal::ExampleItem)
