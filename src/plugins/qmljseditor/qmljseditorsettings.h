// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/filepath.h>

#include <QMutex>
#include <QObject>
#include <QVersionNumber>

namespace QmlJSEditor::Internal {

class QmlJsEditingSettings final : public Utils::AspectContainer
{
public:
    static const inline QVersionNumber mininumQmllsVersion = QVersionNumber(6, 8);

    QmlJsEditingSettings();

    QString defaultFormatCommand() const;

    Utils::BoolAspect enableContextPane{this};
    Utils::BoolAspect pinContextPane{this};
    Utils::BoolAspect autoFormatOnSave{this};
    Utils::BoolAspect autoFormatOnlyCurrentProject{this};
    Utils::BoolAspect foldAuxData{this};
    Utils::BoolAspect useCustomFormatCommand{this};
    Utils::BoolAspect useCustomAnalyzer{this};
    Utils::BoolAspect useQmlls{this};
    Utils::BoolAspect useLatestQmlls{this};
    Utils::BoolAspect ignoreMinimumQmllsVersion{this};
    Utils::BoolAspect disableBuiltinCodemodel{this};
    Utils::BoolAspect generateQmllsIniFiles{this};
    Utils::SelectionAspect uiQmlOpenMode{this};
    Utils::StringAspect formatCommand{this};
    Utils::StringAspect formatCommandOptions{this};
    Utils::IntegersAspect disabledMessages{this};
    Utils::IntegersAspect disabledMessagesForNonQuickUi{this};
};

class QmllsSettingsManager : public QObject
{
    Q_OBJECT

public:
    static QmllsSettingsManager *instance();

    Utils::FilePath latestQmlls();
    void setupAutoupdate();

    bool useQmlls() const;
    bool useLatestQmlls() const;

public slots:
    void checkForChanges();
signals:
    void settingsChanged();

private:
    QMutex m_mutex;
    bool m_useQmlls = true;
    bool m_useLatestQmlls = false;
    bool m_disableBuiltinCodemodel = false;
    bool m_generateQmllsIniFiles = false;
    Utils::FilePath m_latestQmlls;
};

QmlJsEditingSettings &settings();

class QmlJsEditingSettingsPage : public Core::IOptionsPage
{
public:
    QmlJsEditingSettingsPage();
};

} // QmlJSEditor::Internal
