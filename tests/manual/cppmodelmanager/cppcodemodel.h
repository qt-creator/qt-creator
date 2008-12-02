/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#ifndef CPPCODEMODEL_H
#define CPPCODEMODEL_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QStringList>

#include "cppcodemodelitems.h"

class pp_macro;
class CppCodeModelPart;
class CppPartParser;
class CppPartPP;

class CppCodeModel : public QObject,
    public CodeModel
{
    Q_OBJECT

public:
    CppCodeModel(const QByteArray &configuration, QObject *parent = 0);
    ~CppCodeModel();

    void addIncludePath(const QString &path);
    void removeIncludePath(const QString &path);

    void update(const QStringList &files);

protected:
    // returns the macros for this part
    QStringList needsParsing();
    QHash<QString, pp_macro*> *macros();
    QByteArray *contents(QString &file, const QString &local = QString());
    void store();

    void resolvePart(const QString &abspath, CppCodeModelPart **part) const;
    void resolveGlobalPath(QString &file, CppCodeModelPart **part) const;
    void resolveLocalPath(QString &file, const QString &local, CppCodeModelPart **part) const;

    // used by the parser
    CppFileModelItem *fileItem(const QString &name);
    bool hasScope(const QString &name) const;

private:
    QMap<QString, CppCodeModelPart *> m_partcache;
    QMap<QString, CppFileModelItem *> m_fileitems;
    QMap<QString, CppCodeModelPart *> m_includedirs;

    QByteArray m_configuration;
    QSet<QString> m_parsefiles;
    QHash<QString, pp_macro*> m_macros;

    friend class Binder;
    friend class CodeModelFinder;
    friend class CppPartParser;
    friend class CppPartPP;
};
DECLARE_LANGUAGE_CODEMODEL(Cpp)

#endif // CPPCODEMODELPART_H
