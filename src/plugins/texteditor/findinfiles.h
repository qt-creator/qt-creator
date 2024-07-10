// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "basefilefind.h"

#include <QPointer>

QT_BEGIN_NAMESPACE
class QComboBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
class QtcSettings;
} // Utils

namespace TextEditor {

class TEXTEDITOR_EXPORT FindInFiles : public BaseFileFind
{
    Q_OBJECT

public:
    FindInFiles();
    ~FindInFiles() override;

    QString id() const override;
    QString displayName() const override;
    QWidget *createConfigWidget() override;
    Utils::Store save() const override;
    void restore(const Utils::Store &s) override;
    bool isValid() const override;

    void setDirectory(const Utils::FilePath &directory);
    void setBaseDirectory(const Utils::FilePath &directory);
    static void findOnFileSystem(const QString &path);
    static FindInFiles *instance();

    // deprecated
    QByteArray settingsKey() const override;

protected:
    QString label() const override;
    QString toolTip() const override;
    void syncSearchEngineCombo(int selectedSearchEngineIndex) override;

private:
    FileContainerProvider fileContainerProvider() const override;
    void setValid(bool valid);
    void searchEnginesSelectionChanged(int index);
    void currentEditorChanged(Core::IEditor *editor);

    QPointer<QWidget> m_configWidget;
    QPointer<Utils::PathChooser> m_directory;
    QAbstractButton *m_currentDirectory;
    QStackedWidget *m_searchEngineWidget = nullptr;
    QComboBox *m_searchEngineCombo = nullptr;
    bool m_isValid = false;
};

namespace Internal { void setupFindInFiles(QObject *guard); }

} // namespace TextEditor
