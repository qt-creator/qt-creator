/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 AudioCodes Ltd.
**
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CLEARCASESUBMITEDITORWIDGET_H
#define CLEARCASESUBMITEDITORWIDGET_H

#include <utils/submiteditorwidget.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace ClearCase {
namespace Internal {

class ActivitySelector;

class ClearCaseSubmitEditorWidget : public Utils::SubmitEditorWidget
{
    Q_OBJECT

public:
    explicit ClearCaseSubmitEditorWidget(QWidget *parent = 0);
    QString activity() const;
    bool isIdentical() const;
    bool isPreserve() const;
    void setActivity(const QString &act);
    bool activityChanged() const;
    void addKeep();

protected:
    QString commitName() const;

private:
    ActivitySelector *m_actSelector;
    QCheckBox *m_chkIdentical;
    QCheckBox *m_chkPTime;
};

} // namespace Internal
} // namespace ClearCase

#endif // CLEARCASESUBMITEDITORWIDGET_H
