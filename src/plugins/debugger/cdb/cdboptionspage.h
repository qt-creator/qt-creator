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

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/savedaction.h>
#include "ui_cdboptionspagewidget.h"

#include <QPointer>
#include <QSharedPointer>
#include <QStringList>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Utils { class PathListEditor; }
namespace Debugger {
namespace Internal {

class CdbSymbolPathListEditor;
class CdbPathsPageWidget;

// Widget displaying a list of break events for the 'sxe' command
// with a checkbox to enable 'break' and optionally a QLineEdit for
// events with parameters (like 'out:Needle').
class CdbBreakEventWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CdbBreakEventWidget(QWidget *parent = 0);

    void setBreakEvents(const QStringList &l);
    QStringList breakEvents() const;

private:
    QString filterText(int i) const;
    void clear();

    QList<QCheckBox*> m_checkBoxes;
    QList<QLineEdit*> m_lineEdits;
};

class CdbOptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CdbOptionsPageWidget(QWidget *parent = 0);
    QStringList breakEvents() const;

    Utils::SavedActionSet group;

private:
    inline QString path() const;


    Ui::CdbOptionsPageWidget m_ui;
    CdbBreakEventWidget *m_breakEventWidget;
    CdbSymbolPathListEditor *m_symbolPathListEditor;
    Utils::PathListEditor *m_sourcePathListEditor;
};

class CdbOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CdbOptionsPage();
    virtual ~CdbOptionsPage();

    // IOptionsPage
    QWidget *widget();
    void apply();
    void finish();

    static const char *crtDbgReport;

private:
    Utils::SavedActionSet group;
    QPointer<CdbOptionsPageWidget> m_widget;
};

class CdbPathsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CdbPathsPage();
    virtual ~CdbPathsPage();

    static CdbPathsPage *instance();

    // IOptionsPage
    QWidget *widget();
    void apply();
    void finish();

private:
    QPointer<CdbPathsPageWidget> m_widget;
};

} // namespace Internal
} // namespace Debugger
