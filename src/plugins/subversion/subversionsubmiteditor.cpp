/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/


#include "subversionsubmiteditor.h"

#include <utils/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace Subversion::Internal;

SubversionSubmitEditor::SubversionSubmitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters,
                                               QWidget *parentWidget) :
    VCSBase::VCSBaseSubmitEditor(parameters, new Core::Utils::SubmitEditorWidget(parentWidget))
{
    setDisplayName(tr("Subversion Submit"));
}

void SubversionSubmitEditor::setStatusList(const QList<StatusFilePair> &statusOutput)
{
    typedef QList<StatusFilePair>::const_iterator ConstIterator;
    VCSBase::SubmitFileModel *model = new VCSBase::SubmitFileModel(this);

    const ConstIterator cend = statusOutput.constEnd();
    for (ConstIterator it = statusOutput.constBegin(); it != cend; ++it)
        model->addFile(it->second, it->first, true);
    setFileModel(model);

}

/*
QStringList SubversionSubmitEditor::vcsFileListToFileList(const QStringList &rl) const
{
    QStringList files;
    const QStringList::const_iterator cend = rl.constEnd();
    for (QStringList::const_iterator it = rl.constBegin(); it != cend; ++it)
        files.push_back(SubversionSubmitEditor::fileFromStatusLine(*it));
    return files;
}

QString SubversionSubmitEditor::fileFromStatusLine(const QString &statusLine)
{
    enum { filePos = 7 };
    return statusLine.mid(filePos, statusLine.size() - filePos);
}

*/
