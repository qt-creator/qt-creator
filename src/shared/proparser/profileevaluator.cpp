// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profileevaluator.h"

#include "qmakeglobals.h"
#include "qmakevfs.h"
#include "ioutils.h"

#include <QDir>

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE

void ProFileEvaluator::initialize()
{
    QMakeEvaluator::initStatics();
}

ProFileEvaluator::ProFileEvaluator(QMakeGlobals *option, QMakeParser *parser, QMakeVfs *vfs,
                                   QMakeHandler *handler)
  : d(new QMakeEvaluator(option, parser, vfs, handler)),
    m_vfs(vfs)
{
}

ProFileEvaluator::~ProFileEvaluator()
{
    delete d;
}

bool ProFileEvaluator::contains(const QString &variableName) const
{
    return d->m_valuemapStack.top().contains(ProKey(variableName));
}

QString ProFileEvaluator::value(const QString &variable) const
{
    const QStringList &vals = values(variable);
    if (!vals.isEmpty())
        return vals.first();

    return QString();
}

QStringList ProFileEvaluator::values(const QString &variableName) const
{
    const ProStringList &values = d->values(ProKey(variableName));
    QStringList ret;
    ret.reserve(values.size());
    for (const ProString &str : values) {
        const QString expanded = d->m_option->expandEnvVars(str.toQString());
        if (!expanded.isEmpty() || str.isEmpty())
            ret << expanded;
    }
    return ret;
}

QVector<ProFileEvaluator::SourceFile> ProFileEvaluator::fixifiedValues(
        const QString &variable, const QString &baseDirectory, const QString &buildDirectory,
        bool expandWildcards) const
{
    QVector<SourceFile> result;
    QString deviceRoot = d->deviceRoot();
    const ProStringList values = d->values(ProKey(variable));
    for (const ProString &str : values) {
        const QString &el = d->m_option->expandEnvVars(str.toQString());
        const QString fn = IoUtils::isAbsolutePath(deviceRoot, el)
                ? QDir::cleanPath(el) : QDir::cleanPath(baseDirectory + QLatin1Char('/') + el);
        if (IoUtils::exists(deviceRoot, fn)) {
            result << SourceFile{fn, str.sourceFile()};
            continue;
        }
        QStringView fileNamePattern;
        if (expandWildcards) {
            fileNamePattern = IoUtils::fileName(fn);
            expandWildcards = fileNamePattern.contains('*') || fileNamePattern.contains('?');
        }
        if (expandWildcards) {
            const QString patternBaseDir = IoUtils::pathName(fn).toString();
            const QDir::Filters filters = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot;
            for (const QString &fileName : QDir(patternBaseDir).entryList(
                     QStringList(fileNamePattern.toString()), filters)) {
                const QString fullFilePath
                        = QDir::cleanPath(patternBaseDir + '/' + fileName);
                result << SourceFile({fullFilePath, str.sourceFile()});
            }
        } else {
            if (IoUtils::isAbsolutePath(deviceRoot, el)) {
                result << SourceFile{fn, str.sourceFile()};
            } else {
                result << SourceFile{QDir::cleanPath(buildDirectory + QLatin1Char('/') + el),
                          str.sourceFile()};
            }
        }
    }
    return result;
}

QStringList ProFileEvaluator::sourcesToFiles(const QVector<ProFileEvaluator::SourceFile> &sources)
{
    QStringList result;
    result.reserve(sources.size());
    for (const auto &src : sources)
        result << src.fileName;
    return result;
}

QStringList ProFileEvaluator::featureRoots() const
{
    return d->m_featureRoots->paths;
}

// VFS note: all search paths are assumed to be real.
QStringList ProFileEvaluator::absolutePathValues(
        const QString &variable, const QString &baseDirectory) const
{
    QStringList result;
    const QStringList valueList = values(variable);
    for (const QString &el : valueList) {
        QString absEl = IoUtils::resolvePath(d->deviceRoot(), baseDirectory, el);
        if (IoUtils::fileType(d->deviceRoot(), absEl) == IoUtils::FileIsDir)
            result << absEl;
    }
    return result;
}

QVector<ProFileEvaluator::SourceFile> ProFileEvaluator::absoluteFileValues(
        const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
        QHash<ProString, bool> *handled, QSet<QString> &directoriesWithWildcards) const
{
    QMakeVfs::VfsFlags flags = (d->m_cumulative ? QMakeVfs::VfsCumulative : QMakeVfs::VfsExact);
    QVector<SourceFile> result;
    const ProStringList values = d->values(ProKey(variable));
    for (const ProString &str : values) {
        bool &seen = (*handled)[str];
        if (seen)
            continue;
        seen = true;
        const QString &el = d->m_option->expandEnvVars(str.toQString());
        QString absEl;
        if (IoUtils::isAbsolutePath(d->deviceRoot(), el)) {
            QString fn = QDir::cleanPath(el);
            if (m_vfs->exists(d->deviceRoot(), fn, flags)) {
                result << SourceFile{ fn, str.sourceFile() };
                goto next;
            }
            absEl = fn;
        } else {
            for (const QString &dir : searchDirs) {
                QString fn = QDir::cleanPath(dir + QLatin1Char('/') + el);
                if (m_vfs->exists(d->deviceRoot(), fn, flags)) {
                    result << SourceFile{fn, str.sourceFile()};
                    goto next;
                }
            }
            if (baseDirectory.isEmpty())
                goto next;
            absEl = QDir::cleanPath(baseDirectory + QLatin1Char('/') + el);
        }
        {
            int nameOff = absEl.lastIndexOf(QLatin1Char('/'));
            if (nameOff < 0) {
                // The entry is garbage (possibly after env var expansion)
                goto next;
            }
            QString absDir = d->m_tmp1.setRawData(absEl.constData(), nameOff);
            if (IoUtils::exists(d->deviceRoot(), absDir)) {
                QString wildcard = d->m_tmp2.setRawData(absEl.constData() + nameOff + 1,
                                                        absEl.length() - nameOff - 1);
                if (wildcard.contains(QLatin1Char('*')) || wildcard.contains(QLatin1Char('?'))) {
                    wildcard.detach(); // Keep m_tmp out of QRegExp's cache
                    QDir theDir(absDir);
                    theDir.setFilter(theDir.filter() & ~QDir::AllDirs);
                    const QStringList list = theDir.entryList(QStringList(wildcard));
                    for (const QString &fn : list)
                        if (fn != QLatin1String(".") && fn != QLatin1String(".."))
                            result << SourceFile{absDir + QLatin1Char('/') + fn, str.sourceFile()};
                    QString directoryWithWildcard(absDir);
                    directoryWithWildcard.detach();
                    directoriesWithWildcards.insert(directoryWithWildcard);
                } // else if (acceptMissing)
            }
        }
      next: ;
    }
    return result;
}

ProFileEvaluator::TemplateType ProFileEvaluator::templateType() const
{
    static QString str_staticlib = QStringLiteral("staticlib");

    const ProStringList &templ = d->values(ProKey("TEMPLATE"));
    if (templ.count() >= 1) {
        const QString &t = templ.at(0).toQString();
        if (!t.compare(QLatin1String("app"), Qt::CaseInsensitive))
            return TT_Application;
        if (!t.compare(QLatin1String("lib"), Qt::CaseInsensitive))
            return d->isActiveConfig(QStringView(str_staticlib)) ? TT_StaticLibrary : TT_SharedLibrary;
        if (!t.compare(QLatin1String("script"), Qt::CaseInsensitive))
            return TT_Script;
        if (!t.compare(QLatin1String("aux"), Qt::CaseInsensitive))
            return TT_Aux;
        if (!t.compare(QLatin1String("subdirs"), Qt::CaseInsensitive))
            return TT_Subdirs;
    }
    return TT_Unknown;
}

bool ProFileEvaluator::loadNamedSpec(const QString &specDir, bool hostSpec)
{
    d->m_qmakespec = specDir;
    d->m_hostBuild = hostSpec;

    d->updateMkspecPaths();
    return d->loadSpecInternal();
}

bool ProFileEvaluator::accept(ProFile *pro, QMakeEvaluator::LoadFlags flags)
{
    static QString str_no_include_pwd = QStringLiteral("no_include_pwd");
    static QString str_plugin = QStringLiteral("plugin");
    static QString str_plugin_no_share_shlib_cflags = QStringLiteral("plugin_no_share_shlib_cflags");

    if (d->visitProFile(pro, QMakeHandler::EvalProjectFile, flags) != QMakeEvaluator::ReturnTrue)
        return false;

    if (flags & QMakeEvaluator::LoadPostFiles) {
        // This is postprocessing which is hard-coded inside qmake's generators.

        ProStringList &incpath = d->valuesRef(ProKey("INCLUDEPATH"));
        incpath += d->values(ProKey("QMAKE_INCDIR"));
        if (!d->isActiveConfig(QStringView(str_no_include_pwd))) {
            incpath.prepend(ProString(pro->directoryName()));
            // It's pretty stupid that this is appended - it should be the second entry.
            if (pro->directoryName() != d->m_outputDir)
                incpath << ProString(d->m_outputDir);
        }
        // The location of this is inconsistent among generators.
        incpath << ProString(d->m_qmakespec);

        // We ignore CFLAGS and LFLAGS, as they are not used higher up anyway.
        ProStringList &cxxflags = d->valuesRef(ProKey("QMAKE_CXXFLAGS"));
        switch (templateType()) {
        case TT_Application:
            cxxflags += d->values(ProKey("QMAKE_CXXFLAGS_APP"));
            break;
        case TT_SharedLibrary:
            {
                bool plugin = d->isActiveConfig(QStringView(str_plugin));
                if (!plugin || !d->isActiveConfig(QStringView(str_plugin_no_share_shlib_cflags)))
                    cxxflags += d->values(ProKey("QMAKE_CXXFLAGS_SHLIB"));
                if (plugin)
                    cxxflags += d->values(ProKey("QMAKE_CXXFLAGS_PLUGIN"));
            }
            break;
        default:
            break;
        }
    }

    return true;
}

QString ProFileEvaluator::propertyValue(const QString &name) const
{
    return d->m_option->propertyValue(ProKey(name)).toQString();
}

#ifdef PROEVALUATOR_CUMULATIVE
void ProFileEvaluator::setCumulative(bool on)
{
    d->m_cumulative = on;
}
#endif

void ProFileEvaluator::setExtraVars(const QHash<QString, QStringList> &extraVars)
{
    ProValueMap map;
    QHash<QString, QStringList>::const_iterator it = extraVars.constBegin();
    QHash<QString, QStringList>::const_iterator end = extraVars.constEnd();
    for ( ; it != end; ++it)
        map.insert(ProKey(it.key()), ProStringList(it.value()));
    d->setExtraVars(map);
}

void ProFileEvaluator::setExtraConfigs(const QStringList &extraConfigs)
{
     d->setExtraConfigs(ProStringList(extraConfigs));
}

void ProFileEvaluator::setOutputDir(const QString &dir)
{
    d->m_outputDir = dir;
}

QT_END_NAMESPACE
