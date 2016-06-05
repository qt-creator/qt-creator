/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
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

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace Todo {
namespace Internal {

namespace Ui { class OptionsDialog; }

class Settings;
class Keyword;

class OptionsDialog : public QWidget
{
public:
    explicit OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

    void setSettings(const Settings &settings);
    Settings settings();

private:
    void addKeywordButtonClicked();
    void editKeywordButtonClicked();
    void removeKeywordButtonClicked();
    void resetKeywordsButtonClicked();
    void setKeywordsButtonsEnabled();
    void keywordDoubleClicked(QListWidgetItem *item);
    void uiFromSettings(const Settings &settings);
    Settings settingsFromUi();
    void addToKeywordsList(const Keyword &keyword);
    void editKeyword(QListWidgetItem *item);
    QSet<QString> keywordNames();

    Ui::OptionsDialog *ui;
};

} // namespace Internal
} // namespace Todo
