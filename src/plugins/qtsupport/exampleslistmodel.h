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

class ExamplesViewController;

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

class ExamplesViewController : public QObject
{
    Q_OBJECT
public:
    explicit ExamplesViewController(ExampleSetModel *exampleSetModel,
                                    Core::SectionedGridView *view,
                                    bool isExamples,
                                    QObject *parent);

    void updateExamples();

private:
    ExampleSetModel *m_exampleSetModel;
    Core::SectionedGridView *m_view;
    bool m_isExamples;
};

} // namespace Internal
} // namespace QtSupport
