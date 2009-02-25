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

#ifndef CPPFILEWIZARD_H
#define CPPFILEWIZARD_H

#include "cppeditorenums.h"

#include <coreplugin/basefilewizard.h>

namespace CppEditor {
namespace Internal {

class CppFileWizard : public Core::StandardFileWizard
{
    Q_OBJECT

public:
    typedef Core::BaseFileWizardParameters BaseFileWizardParameters;

    CppFileWizard(const BaseFileWizardParameters &parameters,
                  FileType type,
                  QObject *parent = 0);

protected:
    static QString toAlphaNum(const QString &s);
    QString fileContents(FileType type, const QString &baseName) const;

protected:
    Core::GeneratedFiles generateFilesFromPath(const QString &path,
                                               const QString &fileName,
                                               QString *errorMessage) const;
private:
     const FileType m_type;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPFILEWIZARD_H
