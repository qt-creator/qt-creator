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

#ifndef KEYWORDDIALOG_H
#define KEYWORDDIALOG_H

#include <QDialog>
#include <QSet>

namespace Todo {
namespace Internal {

namespace Ui {
    class KeywordDialog;
}

class Keyword;

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
    void setupListWidget(const QString &selectedIcon);
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
