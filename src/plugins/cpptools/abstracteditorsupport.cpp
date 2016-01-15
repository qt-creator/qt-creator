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

#include "abstracteditorsupport.h"

#include "cppfilesettingspage.h"
#include "cppmodelmanager.h"

#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/templateengine.h>

namespace CppTools {

AbstractEditorSupport::AbstractEditorSupport(CppModelManager *modelmanager, QObject *parent) :
    QObject(parent), m_modelmanager(modelmanager), m_revision(1)
{
    modelmanager->addExtraEditorSupport(this);
}

AbstractEditorSupport::~AbstractEditorSupport()
{
    m_modelmanager->removeExtraEditorSupport(this);
}

void AbstractEditorSupport::updateDocument()
{
    ++m_revision;
    m_modelmanager->updateSourceFiles(QSet<QString>() << fileName());
}

void AbstractEditorSupport::notifyAboutUpdatedContents() const
{
    m_modelmanager->emitAbstractEditorSupportContentsUpdated(fileName(), contents());
}

QString AbstractEditorSupport::licenseTemplate(const QString &file, const QString &className)
{
    const QString license = Internal::CppFileSettings::licenseTemplate();
    Utils::MacroExpander expander;
    expander.registerVariable("Cpp:License:FileName", tr("The file name."),
                              [file]() { return Utils::FileName::fromString(file).fileName(); });
    expander.registerVariable("Cpp:License:ClassName", tr("The class name."),
                              [className]() { return className; });

    return Utils::TemplateEngine::processText(&expander, license, 0);
}

} // namespace CppTools

