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

#include <utils/optional.h>

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
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
    explicit LocatorWidget(Locator *locator);

    void showText(const QString &text, int selectionStart = -1, int selectionLength = 0);
    QString currentText() const;
    QAbstractItemModel *model() const;

    void updatePlaceholderText(Command *command);

    void scheduleAcceptEntry(const QModelIndex &index);

signals:
    void showCurrentItemToolTip();
    void lostFocus();
    void hidePopup();
    void selectRow(int row);
    void handleKey(QKeyEvent *keyEvent); // only use with DirectConnection, event is deleted
    void parentChanged();
    void showPopup();

private:
    void showPopupDelayed();
    void showPopupNow();
    void acceptEntry(int row);
    void showConfigureDialog();
    void addSearchResults(int firstIndex, int endIndex);
    void handleSearchFinished();
    void setFocusToCurrentMode();
    void updateFilterList();

    bool eventFilter(QObject *obj, QEvent *event);

    void updateCompletionList(const QString &text);
    QList<ILocatorFilter*> filtersFor(const QString &text, QString &searchText);
    void setProgressIndicatorVisible(bool visible);

    LocatorModel *m_locatorModel;

    QMenu *m_filterMenu;
    QAction *m_refreshAction;
    QAction *m_configureAction;
    Utils::FancyLineEdit *m_fileLineEdit;
    QTimer m_showPopupTimer;
    QFutureWatcher<LocatorFilterEntry> *m_entriesWatcher;
    QString m_requestedCompletionText;
    bool m_needsClearResult = true;
    bool m_updateRequested = false;
    bool m_possibleToolTipRequest = false;
    QWidget *m_progressIndicator;
    QTimer m_showProgressTimer;
    Utils::optional<int> m_rowRequestedForAccept;
};

class LocatorPopup : public QWidget
{
public:
    LocatorPopup(LocatorWidget *locatorWidget, QWidget *parent = 0);

    CompletionList *completionList() const;
    LocatorWidget *inputWidget() const;

    void focusOutEvent (QFocusEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    QSize preferredSize();
    virtual void updateGeometry();
    virtual void inputLostFocus();

    QPointer<QWidget> m_window;
    CompletionList *m_tree;

private:
    void updateWindow();

    LocatorWidget *m_inputWidget;
};

LocatorWidget *createStaticLocatorWidget(Locator *locator);
LocatorPopup *createLocatorPopup(Locator *locator, QWidget *parent);

} // namespace Internal
} // namespace Core
