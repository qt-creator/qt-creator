// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    QtVersion *findHighestQtVersion(const QtVersions &versions) const;

    int indexForQtVersion(QtVersion *qtVersion) const;
    void recreateModel(const QtVersions &qtVersions);
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

    void updateExamples();

    QStringList exampleSets() const;
    ExampleSetModel *exampleSetModel() { return &m_exampleSetModel; }

signals:
    void selectedExampleSetChanged(int);

private:
    void updateSelectedQtVersion();

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
