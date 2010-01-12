/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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


#include "cvssubmiteditor.h"

#include <utils/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace CVS::Internal;

CVSSubmitEditor::CVSSubmitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters,
                                               QWidget *parentWidget) :
    VCSBase::VCSBaseSubmitEditor(parameters, new Utils::SubmitEditorWidget(parentWidget)),
    m_msgAdded(tr("Added")),
    m_msgRemoved(tr("Removed")),
    m_msgModified(tr("Modified"))
{
}

QString CVSSubmitEditor::stateName(State st) const
{
    switch (st) {
    case LocallyAdded:
        return m_msgAdded;
    case LocallyModified:
        return m_msgModified;
    case LocallyRemoved:
        return m_msgRemoved;
    }
    return QString();
}

void CVSSubmitEditor::setStateList(const QList<StateFilePair> &statusOutput)
{
    typedef QList<StateFilePair>::const_iterator ConstIterator;
    VCSBase::SubmitFileModel *model = new VCSBase::SubmitFileModel(this);

    const ConstIterator cend = statusOutput.constEnd();
    for (ConstIterator it = statusOutput.constBegin(); it != cend; ++it)
        model->addFile(it->second, stateName(it->first), true);
    setFileModel(model);
}
