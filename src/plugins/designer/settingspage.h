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

#include <QPointer>

QT_BEGIN_NAMESPACE
class QDesignerOptionsPageInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class SettingsPageWidget;

class SettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit SettingsPage(QDesignerOptionsPageInterface *designerPage);

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QDesignerOptionsPageInterface *m_designerPage;
    bool m_initialized = false;
    QPointer<QWidget> m_widget;
};

class SettingsPageProvider : public Core::IOptionsPageProvider
{
    Q_OBJECT

public:
    explicit SettingsPageProvider(QObject *parent = nullptr);

    QList<Core::IOptionsPage *> pages() const override;
    bool matches(const QString &searchKeyWord) const override;

private:
    mutable bool m_initialized = false;
    mutable QStringList m_keywords;
};

} // namespace Internal
} // namespace Designer
