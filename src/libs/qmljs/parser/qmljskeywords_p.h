/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QMLJSKEYWORDS_P_H
#define QMLJSKEYWORDS_P_H

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

static inline int classify2(const QChar *s, bool qmlMode) {
  if (s[0].unicode() == 'a') {
    if (s[1].unicode() == 's') {
      return qmlMode ? Lexer::T_AS : Lexer::T_RESERVED_WORD;
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'o') {
      return Lexer::T_DO;
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'f') {
      return Lexer::T_IF;
    }
    else if (s[1].unicode() == 'n') {
      return Lexer::T_IN;
    }
  }
  else if (qmlMode && s[0].unicode() == 'o') {
    if (s[1].unicode() == 'n') {
      return Lexer::T_ON;
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify3(const QChar *s, bool /*qmlMode*/) {
  if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'r') {
        return Lexer::T_FOR;
      }
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 't') {
        return Lexer::T_INT;
      }
    }
  }
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'w') {
        return Lexer::T_NEW;
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'y') {
        return Lexer::T_TRY;
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        return Lexer::T_VAR;
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify4(const QChar *s, bool /*qmlMode*/) {
  if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'y') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          return Lexer::T_BYTE;
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 'e') {
          return Lexer::T_CASE;
        }
      }
    }
    else if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'r') {
          return Lexer::T_CHAR;
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 'e') {
          return Lexer::T_ELSE;
        }
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'u') {
        if (s[3].unicode() == 'm') {
          return Lexer::T_ENUM;
        }
      }
    }
  }
  else if (s[0].unicode() == 'g') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'o') {
          return Lexer::T_GOTO;
        }
      }
    }
  }
  else if (s[0].unicode() == 'l') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'g') {
          return Lexer::T_LONG;
        }
      }
    }
  }
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'l') {
          return Lexer::T_NULL;
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 's') {
          return Lexer::T_THIS;
        }
      }
    }
    else if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'u') {
        if (s[3].unicode() == 'e') {
          return Lexer::T_TRUE;
        }
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'd') {
          return Lexer::T_VOID;
        }
      }
    }
  }
  else if (s[0].unicode() == 'w') {
    if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'h') {
          return Lexer::T_WITH;
        }
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify5(const QChar *s, bool /*qmlMode*/) {
  if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'k') {
            return Lexer::T_BREAK;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'h') {
            return Lexer::T_CATCH;
          }
        }
      }
    }
    else if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 's') {
            return Lexer::T_CLASS;
          }
        }
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 't') {
            return Lexer::T_CONST;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 'e') {
            return Lexer::T_FALSE;
          }
        }
      }
    }
    else if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'l') {
            return Lexer::T_FINAL;
          }
        }
      }
    }
    else if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 't') {
            return Lexer::T_FLOAT;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 't') {
            return Lexer::T_SHORT;
          }
        }
      }
    }
    else if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            return Lexer::T_SUPER;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'w') {
            return Lexer::T_THROW;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'w') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'e') {
            return Lexer::T_WHILE;
          }
        }
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify6(const QChar *s, bool qmlMode) {
  if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'e') {
              return Lexer::T_DELETE;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'u') {
        if (s[3].unicode() == 'b') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'e') {
              return Lexer::T_DOUBLE;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'x') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 't') {
              return Lexer::T_EXPORT;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'm') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 't') {
              return qmlMode ? Lexer::T_IMPORT : Lexer::T_RESERVED_WORD;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'v') {
            if (s[5].unicode() == 'e') {
              return Lexer::T_NATIVE;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'b') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'c') {
              return qmlMode ? Lexer::T_PUBLIC : Lexer::T_RESERVED_WORD;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'r') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'n') {
              return Lexer::T_RETURN;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (qmlMode && s[1].unicode() == 'i') {
      if (s[2].unicode() == 'g') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'l') {
              return Lexer::T_SIGNAL;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 't') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'c') {
              return Lexer::T_STATIC;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'w') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'c') {
            if (s[5].unicode() == 'h') {
              return Lexer::T_SWITCH;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'w') {
            if (s[5].unicode() == 's') {
              return Lexer::T_THROWS;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'y') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'f') {
              return Lexer::T_TYPEOF;
            }
          }
        }
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify7(const QChar *s, bool /*qmlMode*/) {
  if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'n') {
                return Lexer::T_BOOLEAN;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'f') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'u') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 't') {
                return Lexer::T_DEFAULT;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'x') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 's') {
                return Lexer::T_EXTENDS;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'y') {
                return Lexer::T_FINALLY;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'c') {
        if (s[3].unicode() == 'k') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'g') {
              if (s[6].unicode() == 'e') {
                return Lexer::T_PACKAGE;
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'v') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'e') {
                return Lexer::T_PRIVATE;
              }
            }
          }
        }
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify8(const QChar *s, bool qmlMode) {
  if (s[0].unicode() == 'a') {
    if (s[1].unicode() == 'b') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'c') {
                if (s[7].unicode() == 't') {
                  return Lexer::T_ABSTRACT;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'u') {
                if (s[7].unicode() == 'e') {
                  return Lexer::T_CONTINUE;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'b') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'g') {
            if (s[5].unicode() == 'g') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'r') {
                  return Lexer::T_DEBUGGER;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 'o') {
                if (s[7].unicode() == 'n') {
                  return Lexer::T_FUNCTION;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (qmlMode && s[0].unicode() == 'p') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'y') {
                  return Lexer::T_PROPERTY;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (qmlMode && s[0].unicode() == 'r') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'd') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'y') {
                  return Lexer::T_READONLY;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'e') {
                  return Lexer::T_VOLATILE;
                }
              }
            }
          }
        }
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify9(const QChar *s, bool /*qmlMode*/) {
  if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'f') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'c') {
                  if (s[8].unicode() == 'e') {
                    return Lexer::T_INTERFACE;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'c') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'e') {
                  if (s[8].unicode() == 'd') {
                    return Lexer::T_PROTECTED;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 's') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 't') {
                    return Lexer::T_TRANSIENT;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify10(const QChar *s, bool /*qmlMode*/) {
  if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'm') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'm') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 't') {
                    if (s[9].unicode() == 's') {
                      return Lexer::T_IMPLEMENTS;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'c') {
                if (s[7].unicode() == 'e') {
                  if (s[8].unicode() == 'o') {
                    if (s[9].unicode() == 'f') {
                      return Lexer::T_INSTANCEOF;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

static inline int classify12(const QChar *s, bool /*qmlMode*/) {
  if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'y') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'h') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'o') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'i') {
                    if (s[9].unicode() == 'z') {
                      if (s[10].unicode() == 'e') {
                        if (s[11].unicode() == 'd') {
                          return Lexer::T_SYNCHRONIZED;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return Lexer::T_IDENTIFIER;
}

int Lexer::classify(const QChar *s, int n, bool qmlMode) {
  switch (n) {
    case 2: return classify2(s, qmlMode);
    case 3: return classify3(s, qmlMode);
    case 4: return classify4(s, qmlMode);
    case 5: return classify5(s, qmlMode);
    case 6: return classify6(s, qmlMode);
    case 7: return classify7(s, qmlMode);
    case 8: return classify8(s, qmlMode);
    case 9: return classify9(s, qmlMode);
    case 10: return classify10(s, qmlMode);
    case 12: return classify12(s, qmlMode);
    default: return Lexer::T_IDENTIFIER;
  } // switch
}

#endif // QMLJSKEYWORDS_P_H
