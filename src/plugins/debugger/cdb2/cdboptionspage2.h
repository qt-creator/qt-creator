/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CDBSETTINGSPAGE_H
#define CDBSETTINGSPAGE_H

#include "cdboptions2.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_cdboptionspagewidget2.h"

#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Debugger {
namespace Cdb {

class CdbOptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CdbOptionsPageWidget(QWidget *parent);

    void setOptions(CdbOptions &o);
    CdbOptions options() const;

    QString searchKeywords() const;

    virtual bool eventFilter(QObject *, QEvent *);

private slots:
    void autoDetect();
    void downLoadLinkActivated(const QString &);
    void hideReportLabel();

private:
    void setReport(const QString &, bool success);
    inline bool is64Bit() const;
    inline QString path() const;

    static bool checkInstallation(const QString &executable, bool is64Bit,
                                  QString *message);

    Ui::CdbOptionsPageWidget2 m_ui;
    QTimer *m_reportTimer;
};

class CdbOptionsPage : public Core::IOptionsPage
{
    Q_DISABLE_COPY(CdbOptionsPage)
    Q_OBJECT
public:
    explicit CdbOptionsPage();
    virtual ~CdbOptionsPage();

    static CdbOptionsPage *instance();

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
