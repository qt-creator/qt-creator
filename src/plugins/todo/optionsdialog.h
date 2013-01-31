/**************************************************************************
**
** Copyright (c) 2013 Dmitry Savchenko
** Copyright (c) 2013 Vasiliy Sorokin
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace Todo {
namespace Internal {

namespace Ui {
    class OptionsDialog;
}

class Settings;
class Keyword;

class OptionsDialog : public QWidget
{
    Q_OBJECT
public:
    explicit OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

    void setSettings(const Settings &settings);
    Settings settings();

private slots:
    void addButtonClicked();
    void editButtonClicked();
    void removeButtonClicked();
    void resetButtonClicked();
    void setButtonsEnabled();
    void itemDoubleClicked(QListWidgetItem *item);

private:
    void uiFromSettings(const Settings &settings);
    Settings settingsFromUi();
    void addToKeywordsList(const Keyword &keyword);
    void editItem(QListWidgetItem *item);
    QSet<QString> keywordNames();

    Ui::OptionsDialog *ui;
};

} // namespace Internal
} // namespace Todo

#endif // OPTIONSDIALOG_H
