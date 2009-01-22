/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef TEXTFILEWIZARD_H
#define TEXTFILEWIZARD_H

#include "texteditor_global.h"

#include <coreplugin/basefilewizard.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT TextFileWizard : public Core::StandardFileWizard
{
    Q_OBJECT

public:
    typedef Core::BaseFileWizardParameters BaseFileWizardParameters;
    TextFileWizard(const QString &mimeType,
                   const QString &editorKind,
                   const QString &suggestedFileName,
                   const BaseFileWizardParameters &parameters,
                   QObject *parent = 0);

protected:
    virtual Core::GeneratedFiles
        generateFilesFromPath(const QString &path, const QString &name,
                              QString *errorMessage) const;
private:
    const QString m_mimeType;
    const QString m_editorKind;
    const QString m_suggestedFileName;
};

} // namespace TextEditor

#endif // TEXTFILEWIZARD_H
