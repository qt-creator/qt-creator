// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR LGPL-3.0

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

#include "mimedatabase_p.h"
#include "mimeprovider_p.h"

namespace Utils {
namespace Internal {

class MimeTypeParserBase
{
    Q_DISABLE_COPY(MimeTypeParserBase)

public:
    MimeTypeParserBase() {}
    virtual ~MimeTypeParserBase() {}

    bool parse(const QByteArray &content, const QString &fileName, QString *errorMessage);

protected:
    virtual bool mimeTypeExists(const QString &mimeTypeName) = 0;
    virtual bool process(const MimeType &t, QString *errorMessage) = 0;
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
    inline bool mimeTypeExists(const QString &mimeTypeName) override
    { return m_provider.mimeTypeForName(mimeTypeName).isValid(); }

    inline bool process(const MimeType &t, QString *) override
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

} // Internal
} // Utils
