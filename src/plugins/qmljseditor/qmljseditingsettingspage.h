// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmllssettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>
#include <QWidget>

namespace QmlJSEditor {

class QmlJsEditingSettings
{
public:
    QmlJsEditingSettings() = default;

    static QmlJsEditingSettings get();
    void set();

    void fromSettings(Utils::QtcSettings *);
    void toSettings(Utils::QtcSettings *) const;

    bool equals(const QmlJsEditingSettings &other) const;

    bool enableContextPane() const;
    void setEnableContextPane(const bool enableContextPane);

    bool pinContextPane() const;
    void setPinContextPane(const bool pinContextPane);

    bool autoFormatOnSave() const;
    void setAutoFormatOnSave(const bool autoFormatOnSave);

    bool autoFormatOnlyCurrentProject() const;
    void setAutoFormatOnlyCurrentProject(const bool autoFormatOnlyCurrentProject);

    bool foldAuxData() const;
    void setFoldAuxData(const bool foldAuxData);

    QString defaultFormatCommand() const;
    QString formatCommand() const;
    void setFormatCommand(const QString &formatCommand);

    QString formatCommandOptions() const;
    void setFormatCommandOptions(const QString &formatCommandOptions);

    bool useCustomFormatCommand() const;
    void setUseCustomFormatCommand(bool customCommand);

    const QString uiQmlOpenMode() const;
    void setUiQmlOpenMode(const QString &mode);

    bool useCustomAnalyzer() const;
    void setUseCustomAnalyzer(bool customAnalyzer);

    QSet<int> disabledMessages() const;
    void setDisabledMessages(const QSet<int> &disabled);

    QSet<int> disabledMessagesForNonQuickUi() const;
    void setDisabledMessagesForNonQuickUi(const QSet<int> &disabled);

    bool useQmlls() const;
    void setUseQmlls(bool newUseQmlls);

    bool useLatestQmlls() const;
    void setUseLatestQmlls(bool newUseLatestQmlls);

    bool ignoreMinimumQmllsVersion() const;
    void setIgnoreMinimumQmllsVersion(bool newIgnoreMinimumQmllsVersion);

    bool disableBuiltinCodemodel() const;
    void setDisableBuiltinCodemodel(bool newDisableBuiltinCodemodel);

    bool generateQmllsIniFiles() const;
    void setGenerateQmllsIniFiles(bool newGenerateQmllsIniFiles);

    friend bool operator==(const QmlJsEditingSettings &s1, const QmlJsEditingSettings &s2)
    { return s1.equals(s2); }
    friend bool operator!=(const QmlJsEditingSettings &s1, const QmlJsEditingSettings &s2)
    { return !s1.equals(s2); }

private:
    bool m_enableContextPane = false;
    bool m_pinContextPane = false;
    bool m_autoFormatOnSave = false;
    bool m_autoFormatOnlyCurrentProject = false;
    bool m_foldAuxData = true;
    bool m_useCustomFormatCommand = false;
    bool m_useCustomAnalyzer = false;
    bool m_useQmlls = true;
    bool m_useLatestQmlls = false;
    bool m_ignoreMinimumQmllsVersion = false;
    bool m_disableBuiltinCodemodel = false;
    bool m_generateQmllsIniFiles = false;
    QString m_uiQmlOpenMode;
    QString m_formatCommand;
    QString m_formatCommandOptions;
    QSet<int> m_disabledMessages;
    QSet<int> m_disabledMessagesForNonQuickUi;
};

namespace Internal {

class QmlJsEditingSettingsPage : public Core::IOptionsPage
{
public:
    QmlJsEditingSettingsPage();
};

} // namespace Internal
} // namespace QmlDesigner
