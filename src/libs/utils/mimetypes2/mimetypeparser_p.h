// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qtconfigmacros.h>

#include "mimeprovider_p.h"

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace Utils {

class MimeTypeXMLData
{
public:
    void clear();

    void addGlobPattern(const QString &pattern);

    bool hasGlobDeleteAll = false; // true if the mimetype has a glob-deleteall tag
    QString name;
    MimeTypePrivate::LocaleHash localeComments;
    QString genericIconName; // TODO move to a struct that's specific to the XML provider
    QString iconName; // TODO move to a struct that's specific to the XML provider
    QStringList globPatterns;
};

class MimeTypeParserBase
{
    Q_DISABLE_COPY_MOVE(MimeTypeParserBase)

public:
    MimeTypeParserBase() {}
    virtual ~MimeTypeParserBase() {}

    bool parse(QIODevice *dev, const QString &fileName, QString *errorMessage);

    static bool parseNumber(QStringView n, int *target, QString *errorMessage);

protected:
    virtual bool process(const MimeTypeXMLData &t, QString *errorMessage) = 0;
    virtual bool process(const MimeGlobPattern &t, QString *errorMessage) = 0;
    virtual void processParent(const QString &child, const QString &parent) = 0;
    virtual void processAlias(const QString &alias, const QString &name) = 0;
    virtual void processMagicMatcher(const MimeMagicRuleMatcher &matcher) = 0;

private:
    enum ParseState {
        ParseBeginning,
        ParseMimeInfo,
        ParseMimeType,
        ParseComment,
        ParseGenericIcon,
        ParseIcon,
        ParseGlobPattern,
        ParseGlobDeleteAll,
        ParseSubClass,
        ParseAlias,
        ParseMagic,
        ParseMagicMatchRule,
        ParseOtherMimeTypeSubTag,
        ParseError
    };

    static ParseState nextState(ParseState currentState, QStringView startElement);
};


class MimeTypeParser : public MimeTypeParserBase
{
public:
    explicit MimeTypeParser(MimeXMLProvider &provider) : m_provider(provider) {}

protected:
    inline bool process(const MimeTypeXMLData &t, QString *) override
    { m_provider.addMimeType(t); return true; }

    inline bool process(const MimeGlobPattern &glob, QString *) override
    { m_provider.addGlobPattern(glob); return true; }

    inline void processParent(const QString &child, const QString &parent) override
    { m_provider.addParent(child, parent); }

    inline void processAlias(const QString &alias, const QString &name) override
    { m_provider.addAlias(alias, name); }

    inline void processMagicMatcher(const MimeMagicRuleMatcher &matcher) override
    { m_provider.addMagicMatcher(matcher); }

private:
    MimeXMLProvider &m_provider;
};

} // namespace Utils
