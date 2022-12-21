// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "basefilefind.h"

#include <utils/fileutils.h>

#include <QPointer>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
class QComboBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

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
    void writeSettings(QSettings *settings) override;
    void readSettings(QSettings *settings) override;
    bool isValid() const override;

    void setDirectory(const Utils::FilePath &directory);
    void setBaseDirectory(const Utils::FilePath &directory);
    Utils::FilePath directory() const;
    static void findOnFileSystem(const QString &path);
    static FindInFiles *instance();

signals:
    void pathChanged(const Utils::FilePath &directory);

protected:
    Utils::FileIterator *files(const QStringList &nameFilters,
                               const QStringList &exclusionFilters,
                               const QVariant &additionalParameters) const override;
    QVariant additionalParameters() const override;
    QString label() const override;
    QString toolTip() const override;
    void syncSearchEngineCombo(int selectedSearchEngineIndex) override;

private:
    void setValid(bool valid);
    void searchEnginesSelectionChanged(int index);
    Utils::FilePath path() const;

    QPointer<QWidget> m_configWidget;
    QPointer<Utils::PathChooser> m_directory;
    QStackedWidget *m_searchEngineWidget = nullptr;
    QComboBox *m_searchEngineCombo = nullptr;
    bool m_isValid = false;
};

} // namespace TextEditor
