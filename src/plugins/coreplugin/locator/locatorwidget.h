// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "locator.h"

#include <extensionsystem/iplugin.h>

#include <QPointer>
#include <QTimer>
#include <QWidget>

#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Core {
namespace Internal {

class LocatorModel;
class CompletionList;

class LocatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LocatorWidget(Locator *locator);
    ~LocatorWidget() override;

    void showText(const QString &text, int selectionStart = -1, int selectionLength = 0);
    QString currentText() const;
    QAbstractItemModel *model() const;

    void updatePlaceholderText(Command *command);
    void acceptEntry(int row);

signals:
    void showCurrentItemToolTip();
    void lostFocus();
    void hidePopup();
    void selectRow(int row);
    void handleKey(QKeyEvent *keyEvent); // only use with DirectConnection, event is deleted
    void parentChanged();
    void showPopup();

private:
    void showPopupNow();
    static void showConfigureDialog();
    void updateFilterList();
    bool isInMainWindow() const;

    void updatePreviousFocusWidget(QWidget *previous, QWidget *current);
    bool eventFilter(QObject *obj, QEvent *event) override;

    void runMatcher(const QString &text);
    static QList<ILocatorFilter*> filtersFor(const QString &text, QString &searchText);
    void setProgressIndicatorVisible(bool visible);

    LocatorModel *m_locatorModel = nullptr;
    QMenu *m_filterMenu = nullptr;
    QAction *m_centeredPopupAction = nullptr;
    QAction *m_refreshAction = nullptr;
    QAction *m_configureAction = nullptr;
    Utils::FancyLineEdit *m_fileLineEdit = nullptr;
    bool m_possibleToolTipRequest = false;
    QWidget *m_progressIndicator = nullptr;
    QTimer m_showProgressTimer;
    std::optional<int> m_rowRequestedForAccept;
    QPointer<QWidget> m_previousFocusWidget;
    std::unique_ptr<LocatorMatcher> m_locatorMatcher;
};

class LocatorPopup : public QWidget
{
public:
    LocatorPopup(LocatorWidget *locatorWidget, QWidget *parent = nullptr);

    CompletionList *completionList() const;
    LocatorWidget *inputWidget() const;

    void focusOutEvent (QFocusEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    QSize preferredSize();
    virtual void doUpdateGeometry();
    virtual void inputLostFocus();

    QPointer<QWidget> m_window;
    CompletionList *m_tree = nullptr;

private:
    void updateWindow();

    LocatorWidget *m_inputWidget = nullptr;
};

LocatorWidget *createStaticLocatorWidget(Locator *locator);
LocatorPopup *createLocatorPopup(Locator *locator, QWidget *parent);

} // namespace Internal
} // namespace Core
