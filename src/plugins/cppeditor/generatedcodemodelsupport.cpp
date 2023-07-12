// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generatedcodemodelsupport.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/extracompiler.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor {

class QObjectCache
{
public:
    bool contains(QObject *object) const
    {
        return m_cache.contains(object);
    }

    void insert(QObject *object)
    {
        QObject::connect(object, &QObject::destroyed, [this](QObject *dead) {
            m_cache.remove(dead);
        });
        m_cache.insert(object);
    }

private:
    QSet<QObject *> m_cache;
};

GeneratedCodeModelSupport::GeneratedCodeModelSupport(ExtraCompiler *generator,
                                                     const FilePath &generatedFile) :
    AbstractEditorSupport(generator),
    m_generatedFilePath(generatedFile),
    m_generator(generator)
{
    QLoggingCategory log("qtc.cppeditor.generatedcodemodelsupport", QtWarningMsg);
    qCDebug(log) << "ctor GeneratedCodeModelSupport for" << m_generator->source()
                 << generatedFile;

    connect(m_generator, &ExtraCompiler::contentsChanged,
            this, &GeneratedCodeModelSupport::onContentsChanged, Qt::QueuedConnection);
    onContentsChanged(generatedFile);
}

GeneratedCodeModelSupport::~GeneratedCodeModelSupport()
{
    CppModelManager::emitAbstractEditorSupportRemoved(m_generatedFilePath.toString());
    QLoggingCategory log("qtc.cppeditor.generatedcodemodelsupport", QtWarningMsg);
    qCDebug(log) << "dtor ~generatedcodemodelsupport for" << m_generatedFilePath;
}

void GeneratedCodeModelSupport::onContentsChanged(const FilePath &file)
{
    if (file == m_generatedFilePath) {
        notifyAboutUpdatedContents();
        updateDocument();
    }
}

QByteArray GeneratedCodeModelSupport::contents() const
{
    return m_generator->content(m_generatedFilePath);
}

FilePath GeneratedCodeModelSupport::filePath() const
{
    return m_generatedFilePath;
}

FilePath GeneratedCodeModelSupport::sourceFilePath() const
{
    return m_generator->source();
}

void GeneratedCodeModelSupport::update(const QList<ExtraCompiler *> &generators)
{
    static QObjectCache extraCompilerCache;

    for (ExtraCompiler *generator : generators) {
        if (extraCompilerCache.contains(generator))
            continue;

        extraCompilerCache.insert(generator);
        generator->forEachTarget([generator](const FilePath &generatedFile) {
            new GeneratedCodeModelSupport(generator, generatedFile);
        });
    }
}

} // CppEditor
