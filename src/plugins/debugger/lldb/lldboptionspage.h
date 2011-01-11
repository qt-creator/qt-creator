/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef LLDBSETTINGSPAGE_H
#define LLDBSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_lldboptionspagewidget.h"

#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QSettings>

namespace Debugger {
namespace Internal {

class LldbOptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LldbOptionsPageWidget(QWidget *parent, QSettings *s);

public slots:
    void save();
    void load();

private:
    Ui::LldbOptionsPageWidget m_ui;
    QSettings *s;
};

class LldbOptionsPage : public Core::IOptionsPage
{
    Q_DISABLE_COPY(LldbOptionsPage)
    Q_OBJECT
public:
    explicit LldbOptionsPage();
    virtual ~LldbOptionsPage();

    // IOptionsPage
    virtual QString id() const { return settingsId(); }
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();
    virtual bool matches(const QString &) const;

    static QString settingsId();

private:
    QPointer<LldbOptionsPageWidget> m_widget;
};

} // namespace Internal
} // namespace Debugger

#endif // LLDBSETTINGSPAGE_H
