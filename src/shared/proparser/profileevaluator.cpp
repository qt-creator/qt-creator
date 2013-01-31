/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "profileevaluator.h"

#include "qmakeglobals.h"
#include "ioutils.h"

#include <QDir>

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE

void ProFileEvaluator::initialize()
{
    QMakeEvaluator::initStatics();
}

ProFileEvaluator::ProFileEvaluator(ProFileGlobals *option, QMakeParser *parser,
                                   QMakeHandler *handler)
  : d(new QMakeEvaluator(option, parser, handler))
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
    foreach (const ProString &str, values)
        ret << d->m_option->expandEnvVars(str.toQString());
    return ret;
}

QStringList ProFileEvaluator::values(const QString &variableName, const ProFile *pro) const
{
    // It makes no sense to put any kind of magic into expanding these
    const ProStringList &values = d->m_valuemapStack.first().value(ProKey(variableName));
    QStringList ret;
    ret.reserve(values.size());
    foreach (const ProString &str, values)
        if (str.sourceFile() == pro)
            ret << d->m_option->expandEnvVars(str.toQString());
    return ret;
}

QString ProFileEvaluator::sysrootify(const QString &path, const QString &baseDir) const
{
    ProFileGlobals *option = static_cast<ProFileGlobals *>(d->m_option);
#ifdef Q_OS_WIN
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    const bool isHostSystemPath =
        option->sysroot.isEmpty() || path.startsWith(option->sysroot, cs)
        || path.startsWith(baseDir, cs) || path.startsWith(d->m_outputDir, cs);

    return isHostSystemPath ? path : option->sysroot + path;
}

QStringList ProFileEvaluator::absolutePathValues(
        const QString &variable, const QString &baseDirectory) const
{
    QStringList result;
    foreach (const QString &el, values(variable)) {
        QString absEl = IoUtils::isAbsolutePath(el)
            ? sysrootify(el, baseDirectory) : IoUtils::resolvePath(baseDirectory, el);
        if (IoUtils::fileType(absEl) == IoUtils::FileIsDir)
            result << QDir::cleanPath(absEl);
    }
    return result;
}

QStringList ProFileEvaluator::absoluteFileValues(
        const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
        const ProFile *pro) const
{
    QStringList result;
    foreach (const QString &el, pro ? values(variable, pro) : values(variable)) {
        QString absEl;
        if (IoUtils::isAbsolutePath(el)) {
            const QString elWithSysroot = sysrootify(el, baseDirectory);
            if (IoUtils::exists(elWithSysroot)) {
                result << QDir::cleanPath(elWithSysroot);
                goto next;
            }
            absEl = elWithSysroot;
        } else {
            foreach (const QString &dir, searchDirs) {
                QString fn = dir + QLatin1Char('/') + el;
                if (IoUtils::exists(fn)) {
                    result << QDir::cleanPath(fn);
                    goto next;
                }
            }
            if (baseDirectory.isEmpty())
                goto next;
            absEl = baseDirectory + QLatin1Char('/') + el;
        }
        {
            absEl = QDir::cleanPath(absEl);
            int nameOff = absEl.lastIndexOf(QLatin1Char('/'));
            QString absDir = d->m_tmp1.setRawData(absEl.constData(), nameOff);
            if (IoUtils::exists(absDir)) {
                QString wildcard = d->m_tmp2.setRawData(absEl.constData() + nameOff + 1,
                                                        absEl.length() - nameOff - 1);
                if (wildcard.contains(QLatin1Char('*')) || wildcard.contains(QLatin1Char('?'))) {
                    wildcard.detach(); // Keep m_tmp out of QRegExp's cache
                    QDir theDir(absDir);
                    foreach (const QString &fn, theDir.entryList(QStringList(wildcard)))
                        if (fn != QLatin1String(".") && fn != QLatin1String(".."))
                            result << absDir + QLatin1Char('/') + fn;
                } // else if (acceptMissing)
            }
        }
      next: ;
    }
    return result;
}

ProFileEvaluator::TemplateType ProFileEvaluator::templateType() const
{
    const ProStringList &templ = d->values(ProKey("TEMPLATE"));
    if (templ.count() >= 1) {
        const QString &t = templ.at(0).toQString();
        if (!t.compare(QLatin1String("app"), Qt::CaseInsensitive))
            return TT_Application;
        if (!t.compare(QLatin1String("lib"), Qt::CaseInsensitive))
            return TT_Library;
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
    return d->visitProFile(pro, QMakeHandler::EvalProjectFile, flags) == QMakeEvaluator::ReturnTrue;
}

QString ProFileEvaluator::propertyValue(const QString &name) const
{
    return d->m_option->propertyValue(ProKey(name)).toQString();
}

QString ProFileEvaluator::resolvedMkSpec() const
{
    return d->m_qmakespec;
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
