/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GERRIT_INTERNAL_GERRITOPTIONSPAGE_H
#define GERRIT_INTERNAL_GERRITOPTIONSPAGE_H

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
    explicit GerritOptionsWidget(QWidget *parent = 0);

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
    GerritOptionsPage(const QSharedPointer<GerritParameters> &p,
                      QObject *parent = 0);
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

#endif // GERRIT_INTERNAL_GERRITOPTIONSPAGE_H
