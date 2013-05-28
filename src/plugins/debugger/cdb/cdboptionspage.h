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

#ifndef CDBOPTIONSPAGE_H
#define CDBOPTIONSPAGE_H

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

namespace Utils {
    class PathListEditor;
}
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
    explicit CdbOptionsPageWidget(QWidget *parent);
    QStringList breakEvents() const;
    QString searchKeywords() const;

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
    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &) const;

    static const char *crtDbgReport;

private:
    Utils::SavedActionSet group;
    QPointer<CdbOptionsPageWidget> m_widget;
    QString m_searchKeywords;
};

class CdbPathsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CdbPathsPage();
    virtual ~CdbPathsPage();

    static CdbPathsPage *instance();

    // IOptionsPage
    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &searchKeyWord) const;

private:
    QPointer<CdbPathsPageWidget> m_widget;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBOPTIONSPAGE_H
