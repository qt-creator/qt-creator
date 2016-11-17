/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cpptools_global.h"

#include <cpptools/abstracteditorsupport.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/extracompiler.h>

#include <QDateTime>
#include <QHash>
#include <QProcess>
#include <QSet>

namespace Core { class IEditor; }
namespace ProjectExplorer { class Project; }

namespace CppTools {

class CPPTOOLS_EXPORT GeneratedCodeModelSupport : public AbstractEditorSupport
{
    Q_OBJECT

public:
    GeneratedCodeModelSupport(CppModelManager *modelmanager,
                              ProjectExplorer::ExtraCompiler *generator,
                              const Utils::FileName &generatedFile);
    ~GeneratedCodeModelSupport();

    /// \returns the contents encoded in UTF-8.
    QByteArray contents() const;
    QString fileName() const; // The generated file

    static void update(const QList<ProjectExplorer::ExtraCompiler *> &generators);

private:
    void onContentsChanged(const Utils::FileName &file);
    Utils::FileName m_generatedFileName;
    ProjectExplorer::ExtraCompiler *m_generator;
};

} // CppTools
