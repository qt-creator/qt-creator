/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef FALLBACKSELECTORWIDGET_H
#define FALLBACKSELECTORWIDGET_H

#include "texteditor_global.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QComboBox;
class QLabel;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

namespace TextEditor {

class IFallbackPreferences;

class TEXTEDITOR_EXPORT FallbackSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FallbackSelectorWidget(QWidget *parent = 0);

    void setFallbackPreferences(TextEditor::IFallbackPreferences *fallbackPreferences);
    QString searchKeywords() const;

    void setFallbacksVisible(bool on);
    void setLabelText(const QString &text);

signals:

private slots:
    void slotComboBoxActivated(int index);
    void slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *);
    void slotRestoreValues(QObject *);

private:
    IFallbackPreferences *m_fallbackPreferences;

    QHBoxLayout *m_layout;

    QComboBox *m_comboBox;
    QLabel *m_comboBoxLabel;
    QPushButton *m_restoreButton;

    bool m_fallbackWidgetVisible;
    QString m_labelText;
};

} // namespace TextEditor

#endif // FALLBACKSELECTORWIDGET_H
