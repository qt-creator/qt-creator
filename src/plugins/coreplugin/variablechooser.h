/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef VARIABLECHOOSER_H
#define VARIABLECHOOSER_H

#include "core_global.h"

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTextEdit;
class QPlainTextEdit;
class QListWidgetItem;
QT_END_NAMESPACE

namespace Utils { class IconButton; }

namespace Core {

namespace Internal { namespace Ui { class VariableChooser; } }

class CORE_EXPORT VariableChooser : public QWidget
{
    Q_OBJECT

public:
    explicit VariableChooser(QWidget *parent = 0);
    ~VariableChooser();

    static const char kVariableSupportProperty[];
    static void addVariableSupport(QWidget *textcontrol);

protected:
    void keyPressEvent(QKeyEvent *ke);
    bool eventFilter(QObject *, QEvent *event);

private slots:
    void updateDescription(const QString &variable);
    void updateCurrentEditor(QWidget *old, QWidget *widget);
    void handleItemActivated(QListWidgetItem *item);
    void insertVariable(const QString &variable);
    void updatePositionAndShow();

private:
    void createIconButton();

    Internal::Ui::VariableChooser *ui;
    QString m_defaultDescription;
    QPointer<QLineEdit> m_lineEdit;
    QPointer<QTextEdit> m_textEdit;
    QPointer<QPlainTextEdit> m_plainTextEdit;
    QPointer<Utils::IconButton> m_iconButton;
};

} // namespace Core

#endif // VARIABLECHOOSER_H
