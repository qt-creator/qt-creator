/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

protected:
    void keyPressEvent(QKeyEvent *ke);

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
