// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

#ifdef USE_LEXEM_STORE
Symbol::LexemStore Symbol::lexemStore;
#endif

#ifdef Q_CC_MSVC
#define ErrorFormatString "%s(%d:%d): "
#else
#define ErrorFormatString "%s:%d:%d: "
#endif

void Parser::error(int rollback) {
    index -= rollback;
    error();
}
void Parser::error(const char *) { throw MocParseException(); }

void Parser::warning(const char *msg) {
    if (displayWarnings && msg)
        fprintf(stderr, ErrorFormatString "warning: %s\n",
                currentFilenames.top().constData(), qMax(0, index > 0 ? symbol().lineNum : 0), 1, msg);
}

void Parser::note(const char *msg) {
    if (displayNotes && msg)
        fprintf(stderr, ErrorFormatString "note: %s\n",
                currentFilenames.top().constData(), qMax(0, index > 0 ? symbol().lineNum : 0), 1, msg);
}

QT_END_NAMESPACE
