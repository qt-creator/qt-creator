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

#include <coreplugin/welcomepagehelper.h>

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
    struct ExtraExampleSet
    {
        QString displayName;
        QString manifestPath;
        QString examplesPath;
    };
    static QVector<ExtraExampleSet> pluginRegisteredExampleSets();

    ExampleSetModel();

    int selectedExampleSet() const { return m_selectedExampleSetIndex; }
    void selectExampleSet(int index);
    QStringList exampleSources(QString *examplesInstallPath, QString *demosInstallPath);
    bool selectedQtSupports(const Utils::Id &target) const;

signals:
    void selectedExampleSetChanged(int);

private:

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

    QVector<ExtraExampleSet> m_extraExampleSets;
    int m_selectedExampleSetIndex = -1;
    QSet<Utils::Id> m_selectedQtTypes;

    bool m_qtVersionManagerInitialized = false;
    bool m_helpManagerInitialized = false;
    bool m_initalized = false;
};

enum InstructionalType
{
    Example = 0, Demo, Tutorial
};

class ExampleItem : public Core::ListItem
{
public:
    QString projectPath;
    QString docUrl;
    QStringList filesToOpen;
    QString mainFile; /* file to be visible after opening filesToOpen */
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

class ExamplesListModel : public Core::ListModel
{
    Q_OBJECT
public:
    explicit ExamplesListModel(QObject *parent);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;

    void updateExamples();

    QStringList exampleSets() const;
    ExampleSetModel *exampleSetModel() { return &m_exampleSetModel; }

    QPixmap fetchPixmapAndUpdatePixmapCache(const QString &url) const override;

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
};

class ExamplesListModelFilter : public Core::ListModelFilter
{
public:
    ExamplesListModelFilter(ExamplesListModel *sourceModel, bool showTutorialsOnly, QObject *parent);

protected:
    bool leaveFilterAcceptsRowBeforeFiltering(const Core::ListItem *item,
                                              bool *earlyExitResult) const override;
private:
    const bool m_showTutorialsOnly;
    ExamplesListModel *m_examplesListModel = nullptr;
};

} // namespace Internal
} // namespace QtSupport

Q_DECLARE_METATYPE(QtSupport::Internal::ExampleItem *)
