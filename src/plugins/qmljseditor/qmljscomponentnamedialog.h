/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLJSCOMPONENTNAMEDIALOG_H
#define QMLJSCOMPONENTNAMEDIALOG_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
    class ComponentNameDialog;
}
QT_END_NAMESPACE

namespace QmlJSEditor {
namespace Internal {

class ComponentNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ComponentNameDialog(QWidget *parent = 0);
    ~ComponentNameDialog();

    static void go(QString *proposedName, QString *proposedPath, QWidget *parent = 0);

public slots:
    void choosePath();
    void validate();

protected:
    QString isValid() const;

private:
    Ui::ComponentNameDialog *ui;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSCOMPONENTNAMEDIALOG_H
