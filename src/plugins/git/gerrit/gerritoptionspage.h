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

#include <vcsbase/vcsbaseoptionspage.h>

#include <QWidget>
#include <QSharedPointer>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSpinBox;
class QCheckBox;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }
namespace Gerrit {
namespace Internal {

class GerritParameters;

class GerritOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GerritOptionsWidget(QWidget *parent = nullptr);

    GerritParameters parameters() const;
    void setParameters(const GerritParameters &);

private:
    QLineEdit *m_hostLineEdit;
    QLineEdit *m_userLineEdit;
    Utils::PathChooser *m_sshChooser;
    QSpinBox *m_portSpinBox;
    QCheckBox *m_httpsCheckBox;
};

class GerritOptionsPage : public VcsBase::VcsBaseOptionsPage
{
    Q_OBJECT

public:
    GerritOptionsPage(const QSharedPointer<GerritParameters> &p, QObject *parent = nullptr);
    ~GerritOptionsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    const QSharedPointer<GerritParameters> &m_parameters;
    QPointer<GerritOptionsWidget> m_widget;
};

} // namespace Internal
} // namespace Gerrit
