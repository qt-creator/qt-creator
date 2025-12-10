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

void GeneratedFileSupport::updateDocument()
{
    ++m_revision;
    CppModelManager::updateSourceFiles({filePath()});
}

void GeneratedFileSupport::notifyAboutUpdatedContents() const
{
    CppModelManager::emitGeneratedFileContentsUpdated(
                filePath(), sourceFilePath(), contents());
}

GeneratedFileSupport::GeneratedFileSupport(ExtraCompiler *generator,
                                                     const FilePath &generatedFile) :
    QObject(generator),
    m_generatedFilePath(generatedFile),
    m_generator(generator)
{
    CppModelManager::addGeneratedFileSupport(this);

    QLoggingCategory log("qtc.cppeditor.generatedcodemodelsupport", QtWarningMsg);
    qCDebug(log) << "ctor GeneratedFileSupport for" << m_generator->source()
                 << generatedFile;

    connect(m_generator, &ExtraCompiler::contentsChanged,
            this, &GeneratedFileSupport::onContentsChanged, Qt::QueuedConnection);
    onContentsChanged(generatedFile);
}

GeneratedFileSupport::~GeneratedFileSupport()
{
    CppModelManager::emitGeneratedFileSupportRemoved(m_generatedFilePath);
    QLoggingCategory log("qtc.cppeditor.generatedcodemodelsupport", QtWarningMsg);
    qCDebug(log) << "dtor ~generatedcodemodelsupport for" << m_generatedFilePath;

    CppModelManager::removeGeneratedFileSupport(this);
}

void GeneratedFileSupport::onContentsChanged(const FilePath &file)
{
    if (file == m_generatedFilePath) {
        notifyAboutUpdatedContents();
        updateDocument();
    }
}

QByteArray GeneratedFileSupport::contents() const
{
    return m_generator->content(m_generatedFilePath);
}

FilePath GeneratedFileSupport::filePath() const
{
    return m_generatedFilePath;
}

FilePath GeneratedFileSupport::sourceFilePath() const
{
    return m_generator->source();
}

void GeneratedFileSupport::update(const QList<ExtraCompiler *> &generators)
{
    static QObjectCache extraCompilerCache;

    for (ExtraCompiler *generator : generators) {
        if (extraCompilerCache.contains(generator))
            continue;

        extraCompilerCache.insert(generator);
        generator->forEachTarget([generator](const FilePath &generatedFile) {
            new GeneratedFileSupport(generator, generatedFile);
        });
    }
}

} // CppEditor
