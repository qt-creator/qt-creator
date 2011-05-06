/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/


#include "subversionsubmiteditor.h"

#include <utils/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace Subversion::Internal;

SubversionSubmitEditor::SubversionSubmitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters,
                                               QWidget *parentWidget) :
    VCSBase::VCSBaseSubmitEditor(parameters, new Utils::SubmitEditorWidget(parentWidget))
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
