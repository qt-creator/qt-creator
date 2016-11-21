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

#include "basefilefind.h"

#include <utils/fileutils.h>

#include <QPointer>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
class QLabel;
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

    void setDirectory(const Utils::FileName &directory);
    Utils::FileName directory() const;
    static void findOnFileSystem(const QString &path);
    static FindInFiles *instance();

signals:
    void pathChanged(const QString &directory);

protected:
    Utils::FileIterator *files(const QStringList &nameFilters,
                               const QVariant &additionalParameters) const override;
    QVariant additionalParameters() const override;
    QString label() const override;
    QString toolTip() const override;
    void syncSearchEngineCombo(int selectedSearchEngineIndex) override;

private:
    void setValid(bool valid);
    void searchEnginesSelectionChanged(int index);
    Utils::FileName path() const;

    QPointer<QWidget> m_configWidget;
    QPointer<Utils::PathChooser> m_directory;
    QStackedWidget *m_searchEngineWidget = nullptr;
    QComboBox *m_searchEngineCombo = nullptr;
    bool m_isValid = false;
};

} // namespace TextEditor
