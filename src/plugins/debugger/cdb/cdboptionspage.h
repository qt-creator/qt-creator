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

#ifndef CDBSETTINGSPAGE_H
#define CDBSETTINGSPAGE_H

#include "cdboptions.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_cdboptionspagewidget.h"

#include <QWidget>
#include <QPointer>
#include <QSharedPointer>
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

class CdbPathDialog : public QDialog
{
    Q_OBJECT
public:
    enum Mode {
        SymbolPaths,
        SourcePaths
    };

    explicit CdbPathDialog(QWidget *parent, Mode mode);

    QStringList paths() const;
    void setPaths(const QStringList &paths);

private:
    Utils::PathListEditor *m_pathListEditor;
};

class CdbOptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CdbOptionsPageWidget(QWidget *parent);

    void setOptions(CdbOptions &o);
    CdbOptions options() const;

    QString searchKeywords() const;

private slots:
    void showSymbolPathDialog();
    void showSourcePathDialog();

private:
    void setSymbolPaths(const QStringList &);
    void setSourcePaths(const QStringList &);

    inline QString path() const;

    Ui::CdbOptionsPageWidget m_ui;
    CdbBreakEventWidget *m_breakEventWidget;
    QStringList m_symbolPaths;
    QStringList m_sourcePaths;
};

class CdbOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CdbOptionsPage();
    virtual ~CdbOptionsPage();

    static CdbOptionsPage *instance();

    // IOptionsPage
    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &) const;

    QSharedPointer<CdbOptions> options() const { return m_options; }

private:
    static CdbOptionsPage *m_instance;
    const QSharedPointer<CdbOptions> m_options;
    QPointer<CdbOptionsPageWidget> m_widget;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBSETTINGSPAGE_H
