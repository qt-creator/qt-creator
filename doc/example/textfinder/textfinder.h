/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef TEXTFINDER_H
#define TEXTFINDER_H

#include "ui_textfinder.h"

#include <QtGui/QWidget>

class QPushButton;
class QTextEdit;
class QLineEdit;

class TextFinder : public QWidget
{
    Q_OBJECT

public:
    TextFinder(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~TextFinder();

private slots:
    void on_findButton_clicked();

private:
    Ui::Form ui;
    void loadTextFile();

    QPushButton *ui_findButton;
    QTextEdit *ui_textEdit;
    QLineEdit *ui_lineEdit;
    bool isFirstTime;
};

#endif // TEXTFINDER_H
