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

#ifndef SUBVERSIONSUBMITEDITOR_H
#define SUBVERSIONSUBMITEDITOR_H

#include <QPair>
#include <QStringList>

#include <vcsbase/vcsbasesubmiteditor.h>

namespace Subversion {
namespace Internal {

class SubversionSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT
public:
    explicit SubversionSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters,
                                    QWidget *parentWidget = 0);

    static QString fileFromStatusLine(const QString &statusLine);

    // A list of ( 'A','M','D') status indicators and file names.
    typedef QPair<QString, QString> StatusFilePair;

    void setStatusList(const QList<StatusFilePair> &statusOutput);
};

} // namespace Internal
} // namespace Subversion

#endif // SUBVERSIONSUBMITEDITOR_H
