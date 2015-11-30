/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef KEYWORDDIALOG_H
#define KEYWORDDIALOG_H

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

private slots:
    void colorSelected(const QColor &color);
    void acceptButtonClicked();

private:
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

#endif // KEYWORDDIALOG_H
