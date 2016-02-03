/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include <vcsbase/submiteditorwidget.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QVBoxLayout;
QT_END_NAMESPACE

namespace ClearCase {
namespace Internal {

class ActivitySelector;

class ClearCaseSubmitEditorWidget : public VcsBase::SubmitEditorWidget
{
    Q_OBJECT

public:
    ClearCaseSubmitEditorWidget();

    QString activity() const;
    bool isIdentical() const;
    bool isPreserve() const;
    void setActivity(const QString &act);
    bool activityChanged() const;
    void addKeep();
    void addActivitySelector(bool isUcm);

protected:
    QString commitName() const;

private:
    ActivitySelector *m_actSelector = nullptr;
    QCheckBox *m_chkIdentical;
    QCheckBox *m_chkPTime;
    QVBoxLayout *m_verticalLayout;
};

} // namespace Internal
} // namespace ClearCase
