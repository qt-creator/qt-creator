/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
