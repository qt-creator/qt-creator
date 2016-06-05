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

#include <QDialog>
#include <QSet>

namespace Todo {
namespace Internal {

namespace Ui { class KeywordDialog; }

class Keyword;
enum class IconType;

class KeywordDialog : public QDialog
{
    Q_OBJECT
public:
    KeywordDialog(const Keyword &keyword, const QSet<QString> &alreadyUsedKeywordNames,
                  QWidget *parent = 0);
    ~KeywordDialog();

    Keyword keyword();

private:
    void colorSelected(const QColor &color);
    void acceptButtonClicked();
    void setupListWidget(IconType selectedIcon);
    void setupColorWidgets(const QColor &color);
    bool canAccept();
    bool isKeywordNameCorrect();
    bool isKeywordNameAlreadyUsed();
    void showError(const QString &text);
    QString keywordName();

    Ui::KeywordDialog *ui;
    QSet<QString> m_alreadyUsedKeywordNames;
};

} // namespace Internal
} // namespace Todo
