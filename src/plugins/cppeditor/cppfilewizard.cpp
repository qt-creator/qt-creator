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

#include "cppfilewizard.h"
#include "cppeditor.h"
#include "cppeditorconstants.h"

#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace CppEditor;
using namespace CppEditor::Internal;

enum { debugWizard = 0 };

CppFileWizard::CppFileWizard(const BaseFileWizardParameters &parameters,
                             FileType type,
                             QObject *parent) :
    Core::StandardFileWizard(parameters, parent),
    m_type(type)
{
}

QString CppFileWizard::toAlphaNum(const QString &s)
{
    QString rc;
    const int len = s.size();
    const QChar underscore =  QLatin1Char('_');

    for (int i = 0; i < len; i++) {
        const QChar c = s.at(i);
        if (c == underscore || c.isLetterOrNumber())
            rc += c;
    }
    return rc;
}

Core::GeneratedFiles CppFileWizard::generateFilesFromPath(const QString &path,
                                                          const QString &name,
                                                          QString * /*errorMessage*/) const

{
    const QString mimeType = m_type == Source ? QLatin1String(Constants::CPP_SOURCE_MIMETYPE) : QLatin1String(Constants::CPP_HEADER_MIMETYPE);
    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, preferredSuffix(mimeType));
    Core::GeneratedFile file(fileName);
    file.setEditorKind(QLatin1String(Constants::C_CPPEDITOR));
    const QString cleanName = toAlphaNum(QFileInfo(name).baseName());

    file.setContents(fileContents(m_type, fileName));
    return Core::GeneratedFiles() << file;
}

QString CppFileWizard::fileContents(FileType type, const QString &fileName) const
{
    const QString baseName = QFileInfo(fileName).baseName();
    QString contents;
    QTextStream str(&contents);
    switch (type) {
    case Header: {
            QString guard = toAlphaNum(baseName).toUpper();
            guard += QLatin1String("_H");
            str << QLatin1String("#ifndef ") << guard
                << QLatin1String("\n#define ") <<  guard <<  QLatin1String("\n\n#endif // ")
                << guard << QLatin1String("\n");
        }
        break;
    case Source:
        str << QLatin1String("#include \"") <<  baseName << '.' << preferredSuffix(QLatin1String(Constants::CPP_HEADER_MIMETYPE)) << QLatin1String("\"\n\n");
        break;
    }
    return contents;
}
