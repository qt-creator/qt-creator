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

#include <qtsupport/baseqtversion.h>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStringList>
#include <QXmlStreamReader>

namespace QtSupport {
namespace Internal {

class ExamplesListModel;

class ExampleSetModel : public QStandardItemModel
{
    Q_OBJECT

public:
    ExampleSetModel();

    int selectedExampleSet() const { return m_selectedExampleSetIndex; }
    void selectExampleSet(int index);
    QStringList exampleSources(QString *examplesInstallPath, QString *demosInstallPath);

signals:
    void selectedExampleSetChanged(int);

private:
    struct ExtraExampleSet {
        QString displayName;
        QString manifestPath;
        QString examplesPath;
    };

    enum ExampleSetType {
        InvalidExampleSet,
        QtExampleSet,
        ExtraExampleSetType
    };

    void writeCurrentIdToSettings(int currentIndex) const;
    int readCurrentIndexFromSettings() const;

    QVariant getDisplayName(int index) const;
    QVariant getId(int index) const;
    ExampleSetType getType(int i) const;
    int getQtId(int index) const;
    int getExtraExampleSetIndex(int index) const;

    BaseQtVersion *findHighestQtVersion(const QList<BaseQtVersion *> &versions) const;

    int indexForQtVersion(BaseQtVersion *qtVersion) const;
    void recreateModel(const QList<BaseQtVersion *> &qtVersions);
    void updateQtVersionList();

    void qtVersionManagerLoaded();
    void helpManagerInitialized();
    void tryToInitialize();

    QList<ExtraExampleSet> m_extraExampleSets;
    QList<BaseQtVersion*> m_qtVersions;
    int m_selectedExampleSetIndex = -1;

    bool m_qtVersionManagerInitialized = false;
    bool m_helpManagerInitialized = false;
    bool m_initalized = false;
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
    explicit ExamplesListModel(QObject *parent);

    int rowCount(const QModelIndex &parent = QModelIndex()) const final;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;

    void updateExamples();

    QStringList exampleSets() const;
    ExampleSetModel *exampleSetModel() { return &m_exampleSetModel; }

signals:
    void selectedExampleSetChanged(int);

private:
    void updateSelectedQtVersion();

    void parseExamples(QXmlStreamReader *reader, const QString &projectsOffset,
                                     const QString &examplesInstallPath);
    void parseDemos(QXmlStreamReader *reader, const QString &projectsOffset,
                                  const QString &demosInstallPath);
    void parseTutorials(QXmlStreamReader *reader, const QString &projectsOffset);

    ExampleSetModel m_exampleSetModel;
    QList<ExampleItem> m_exampleItems;
};

class ExamplesListModelFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ExamplesListModelFilter(ExamplesListModel *sourceModel, bool showTutorialsOnly, QObject *parent);

    void setSearchString(const QString &arg);

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const final;
    void timerEvent(QTimerEvent *event) final;

    void delayedUpdateFilter();

    const bool m_showTutorialsOnly;
    QString m_searchString;
    QStringList m_filterTags;
    QStringList m_filterStrings;
    int m_timerId = 0;
};

} // namespace Internal
} // namespace QtSupport

Q_DECLARE_METATYPE(QtSupport::Internal::ExampleItem)
