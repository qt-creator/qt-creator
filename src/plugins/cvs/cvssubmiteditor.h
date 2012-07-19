/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CVSSUBMITEDITOR_H
#define CVSSUBMITEDITOR_H

#include <QPair>
#include <QStringList>

#include <vcsbase/vcsbasesubmiteditor.h>

namespace Cvs {
namespace Internal {

class CvsSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT

public:
    enum State { LocallyAdded, LocallyModified, LocallyRemoved };
    // A list of state indicators and file names.
    typedef QPair<State, QString> StateFilePair;
    typedef QList<StateFilePair> StateFilePairs;

    explicit CvsSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters,
                             QWidget *parentWidget = 0);

    void setStateList(const StateFilePairs &statusOutput);

private:
    QString stateName(State st) const;

    const QString m_msgAdded;
    const QString m_msgRemoved;
    const QString m_msgModified;
};

} // namespace Internal
} // namespace Cvs

#endif // CVSSUBMITEDITOR_H
