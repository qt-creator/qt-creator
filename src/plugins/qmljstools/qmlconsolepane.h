/**************************************************************************
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

#ifndef QMLCONSOLEPANE_H
#define QMLCONSOLEPANE_H

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QToolButton;
class QLabel;
QT_END_NAMESPACE

namespace Utils {
class SavedAction;
}

namespace QmlJSTools {

namespace Internal {

class QmlConsoleView;
class QmlConsoleItemDelegate;
class QmlConsoleProxyModel;
class QmlConsoleItemModel;

class QmlConsolePane : public Core::IOutputPane
{
    Q_OBJECT
public:
    QmlConsolePane(QObject *parent);
    ~QmlConsolePane();

    QWidget *outputWidget(QWidget *);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const { return tr("QML/JS Console"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    bool canFocus() const;
    bool hasFocus() const;
    void setFocus();

    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();
    bool canNavigate() const;

    void readSettings();
    void setContext(const QString &context);

public slots:
    void writeSettings() const;

private:
    QToolButton *m_showDebugButton;
    QToolButton *m_showWarningButton;
    QToolButton *m_showErrorButton;
    Utils::SavedAction *m_showDebugButtonAction;
    Utils::SavedAction *m_showWarningButtonAction;
    Utils::SavedAction *m_showErrorButtonAction;
    QWidget *m_spacer;
    QLabel *m_statusLabel;
    QmlConsoleView *m_consoleView;
    QmlConsoleItemDelegate *m_itemDelegate;
    QmlConsoleProxyModel *m_proxyModel;
    QWidget *m_consoleWidget;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLCONSOLEPANE_H
