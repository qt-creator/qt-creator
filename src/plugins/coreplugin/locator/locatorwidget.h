/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "locator.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QLineEdit;
class QMenu;
class QTreeView;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Core {
namespace Internal {

class LocatorModel;
class CompletionList;

class LocatorWidget
  : public QWidget
{
    Q_OBJECT

public:
    explicit LocatorWidget(Locator *qop);

    void updateFilterList();

    void show(const QString &text, int selectionStart = -1, int selectionLength = 0);

    void setPlaceholderText(const QString &text);

private:
    void showPopup();
    void showPopupNow();
    void acceptCurrentEntry();
    void filterSelected();
    void showConfigureDialog();
    void addSearchResults(int firstIndex, int endIndex);
    void handleSearchFinished();
    void scheduleAcceptCurrentEntry();
    void setFocusToCurrentMode();

    bool eventFilter(QObject *obj, QEvent *event);

    void showCompletionList();
    void updateCompletionList(const QString &text);
    QList<ILocatorFilter*> filtersFor(const QString &text, QString &searchText);
    void setProgressIndicatorVisible(bool visible);

    Locator *m_locatorPlugin;
    LocatorModel *m_locatorModel;

    CompletionList *m_completionList;
    QMenu *m_filterMenu;
    QAction *m_refreshAction;
    QAction *m_configureAction;
    Utils::FancyLineEdit *m_fileLineEdit;
    QTimer m_showPopupTimer;
    QFutureWatcher<LocatorFilterEntry> *m_entriesWatcher;
    QMap<Id, QAction *> m_filterActionMap;
    QString m_requestedCompletionText;
    bool m_needsClearResult = true;
    bool m_updateRequested = false;
    bool m_acceptRequested = false;
    bool m_possibleToolTipRequest = false;
    QWidget *m_progressIndicator;
    QWidget *m_mainWindow;
    QTimer m_showProgressTimer;
};

} // namespace Internal
} // namespace Core
