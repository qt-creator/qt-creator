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

#include "core_global.h"

#include <utils/fancylineedit.h>
#include <utils/id.h>

#include <QObject>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QAction;
class QWidget;
QT_END_NAMESPACE

namespace Core {
class CommandButton;
class IContext;

class CORE_EXPORT IOutputPane : public QObject
{
    Q_OBJECT

public:
    IOutputPane(QObject *parent = nullptr);
    ~IOutputPane() override;

    virtual QWidget *outputWidget(QWidget *parent) = 0;
    virtual QList<QWidget *> toolBarWidgets() const;
    virtual QString displayName() const = 0;

    virtual int priorityInStatusBar() const = 0;

    virtual void clearContents() = 0;
    virtual void visibilityChanged(bool visible) = 0;

    virtual void setFocus() = 0;
    virtual bool hasFocus() const = 0;
    virtual bool canFocus() const = 0;

    virtual bool canNavigate() const = 0;
    virtual bool canNext() const = 0;
    virtual bool canPrevious() const = 0;
    virtual void goToNext() = 0;
    virtual void goToPrev() = 0;

    void setFont(const QFont &font);
    void setWheelZoomEnabled(bool enabled);

    enum Flag { NoModeSwitch = 0, ModeSwitch = 1, WithFocus = 2, EnsureSizeHint = 4};
    Q_DECLARE_FLAGS(Flags, Flag)

public slots:
    void popup(int flags) { emit showPage(flags); }

    void hide() { emit hidePage(); }
    void toggle(int flags) { emit togglePage(flags); }
    void navigateStateChanged() { emit navigateStateUpdate(); }
    void flash() { emit flashButton(); }
    void setIconBadgeNumber(int number) { emit setBadgeNumber(number); }

signals:
    void showPage(int flags);
    void hidePage();
    void togglePage(int flags);
    void navigateStateUpdate();
    void flashButton();
    void setBadgeNumber(int number);
    void zoomIn(int range);
    void zoomOut(int range);
    void resetZoom();
    void wheelZoomEnabledChanged(bool enabled);
    void fontChanged(const QFont &font);

protected:
    void setupFilterUi(const QString &historyKey);
    QString filterText() const;
    bool filterUsesRegexp() const { return m_filterRegexp; }
    bool filterIsInverted() const { return m_invertFilter; }
    Qt::CaseSensitivity filterCaseSensitivity() const { return m_filterCaseSensitivity; }
    void setFilteringEnabled(bool enable);
    QWidget *filterWidget() const { return m_filterOutputLineEdit; }
    void setupContext(const char *context, QWidget *widget);
    void setZoomButtonsEnabled(bool enabled);

private:
    virtual void updateFilter();

    void filterOutputButtonClicked();
    void setCaseSensitive(bool caseSensitive);
    void setRegularExpressions(bool regularExpressions);
    Utils::Id filterRegexpActionId() const;
    Utils::Id filterCaseSensitivityActionId() const;
    Utils::Id filterInvertedActionId() const;

    Core::CommandButton * const m_zoomInButton;
    Core::CommandButton * const m_zoomOutButton;
    QAction *m_filterActionRegexp = nullptr;
    QAction *m_filterActionCaseSensitive = nullptr;
    QAction *m_invertFilterAction = nullptr;
    Utils::FancyLineEdit *m_filterOutputLineEdit = nullptr;
    IContext *m_context = nullptr;
    bool m_filterRegexp = false;
    bool m_invertFilter = false;
    Qt::CaseSensitivity m_filterCaseSensitivity = Qt::CaseInsensitive;
};

} // namespace Core

 Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IOutputPane::Flags)
