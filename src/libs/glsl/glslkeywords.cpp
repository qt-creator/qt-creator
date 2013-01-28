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

#include "glsllexer.h"
#include "glslparser.h"


using namespace GLSL;

static inline int classify2(const char *s) {
  if (s[0] == 'd') {
    if (s[1] == 'o') {
      return Parser::T_DO;
    }
  }
  else if (s[0] == 'i') {
    if (s[1] == 'f') {
      return Parser::T_IF;
    }
    else if (s[1] == 'n') {
      return Parser::T_IN;
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify3(const char *s) {
  if (s[0] == 'f') {
    if (s[1] == 'o') {
      if (s[2] == 'r') {
        return Parser::T_FOR;
      }
    }
  }
  else if (s[0] == 'i') {
    if (s[1] == 'n') {
      if (s[2] == 't') {
        return Parser::T_INT;
      }
    }
  }
  else if (s[0] == 'o') {
    if (s[1] == 'u') {
      if (s[2] == 't') {
        return Parser::T_OUT;
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify4(const char *s) {
  if (s[0] == 'b') {
    if (s[1] == 'o') {
      if (s[2] == 'o') {
        if (s[3] == 'l') {
          return Parser::T_BOOL;
        }
      }
    }
  }
  else if (s[0] == 'c') {
    if (s[1] == 'a') {
      if (s[2] == 's') {
        if (s[3] == 'e') {
          return Parser::T_CASE | Lexer::Variant_GLSL_150;
        }
      }
    }
  }
  else if (s[0] == 'e') {
    if (s[1] == 'l') {
      if (s[2] == 's') {
        if (s[3] == 'e') {
          return Parser::T_ELSE;
        }
      }
    }
  }
  else if (s[0] == 'f') {
    if (s[1] == 'l') {
      if (s[2] == 'a') {
        if (s[3] == 't') {
          return Parser::T_FLAT | Lexer::Variant_GLSL_150;
        }
      }
    }
  }
  else if (s[0] == 'l') {
    if (s[1] == 'o') {
      if (s[2] == 'w') {
        if (s[3] == 'p') {
          return Parser::T_LOWP | Lexer::Variant_GLSL_ES_100 | Lexer::Variant_GLSL_400;
        }
      }
    }
  }
  else if (s[0] == 'm') {
    if (s[1] == 'a') {
      if (s[2] == 't') {
        if (s[3] == '2') {
          return Parser::T_MAT2;
        }
        else if (s[3] == '3') {
          return Parser::T_MAT3;
        }
        else if (s[3] == '4') {
          return Parser::T_MAT4;
        }
      }
    }
  }
  else if (s[0] == 't') {
    if (s[1] == 'r') {
      if (s[2] == 'u') {
        if (s[3] == 'e') {
          return Parser::T_TRUE;
        }
      }
    }
  }
  else if (s[0] == 'u') {
    if (s[1] == 'i') {
      if (s[2] == 'n') {
        if (s[3] == 't') {
          return Parser::T_UINT | Lexer::Variant_GLSL_150;
        }
      }
    }
  }
  else if (s[0] == 'v') {
    if (s[1] == 'e') {
      if (s[2] == 'c') {
        if (s[3] == '2') {
          return Parser::T_VEC2;
        }
        else if (s[3] == '3') {
          return Parser::T_VEC3;
        }
        else if (s[3] == '4') {
          return Parser::T_VEC4;
        }
      }
    }
    else if (s[1] == 'o') {
      if (s[2] == 'i') {
        if (s[3] == 'd') {
          return Parser::T_VOID;
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify5(const char *s) {
  if (s[0] == 'b') {
    if (s[1] == 'r') {
      if (s[2] == 'e') {
        if (s[3] == 'a') {
          if (s[4] == 'k') {
            return Parser::T_BREAK;
          }
        }
      }
    }
    else if (s[1] == 'v') {
      if (s[2] == 'e') {
        if (s[3] == 'c') {
          if (s[4] == '2') {
            return Parser::T_BVEC2;
          }
          else if (s[4] == '3') {
            return Parser::T_BVEC3;
          }
          else if (s[4] == '4') {
            return Parser::T_BVEC4;
          }
        }
      }
    }
  }
  else if (s[0] == 'c') {
    if (s[1] == 'o') {
      if (s[2] == 'n') {
        if (s[3] == 's') {
          if (s[4] == 't') {
            return Parser::T_CONST;
          }
        }
      }
    }
  }
  else if (s[0] == 'd') {
    if (s[1] == 'm') {
      if (s[2] == 'a') {
        if (s[3] == 't') {
          if (s[4] == '2') {
            return Parser::T_DMAT2 | Lexer::Variant_GLSL_400;
          }
          else if (s[4] == '3') {
            return Parser::T_DMAT3 | Lexer::Variant_GLSL_400;
          }
          else if (s[4] == '4') {
            return Parser::T_DMAT4 | Lexer::Variant_GLSL_400;
          }
        }
      }
    }
    else if (s[1] == 'v') {
      if (s[2] == 'e') {
        if (s[3] == 'c') {
          if (s[4] == '2') {
            return Parser::T_DVEC2 | Lexer::Variant_GLSL_400;
          }
          else if (s[4] == '3') {
            return Parser::T_DVEC3 | Lexer::Variant_GLSL_400;
          }
          else if (s[4] == '4') {
            return Parser::T_DVEC4 | Lexer::Variant_GLSL_400;
          }
        }
      }
    }
  }
  else if (s[0] == 'f') {
    if (s[1] == 'a') {
      if (s[2] == 'l') {
        if (s[3] == 's') {
          if (s[4] == 'e') {
            return Parser::T_FALSE;
          }
        }
      }
    }
    else if (s[1] == 'l') {
      if (s[2] == 'o') {
        if (s[3] == 'a') {
          if (s[4] == 't') {
            return Parser::T_FLOAT;
          }
        }
      }
    }
  }
  else if (s[0] == 'h') {
    if (s[1] == 'i') {
      if (s[2] == 'g') {
        if (s[3] == 'h') {
          if (s[4] == 'p') {
            return Parser::T_HIGHP | Lexer::Variant_GLSL_ES_100 | Lexer::Variant_GLSL_400;
          }
        }
      }
    }
  }
  else if (s[0] == 'i') {
    if (s[1] == 'n') {
      if (s[2] == 'o') {
        if (s[3] == 'u') {
          if (s[4] == 't') {
            return Parser::T_INOUT;
          }
        }
      }
    }
    else if (s[1] == 'v') {
      if (s[2] == 'e') {
        if (s[3] == 'c') {
          if (s[4] == '2') {
            return Parser::T_IVEC2;
          }
          else if (s[4] == '3') {
            return Parser::T_IVEC3;
          }
          else if (s[4] == '4') {
            return Parser::T_IVEC4;
          }
        }
      }
    }
  }
  else if (s[0] == 'p') {
    if (s[1] == 'a') {
      if (s[2] == 't') {
        if (s[3] == 'c') {
          if (s[4] == 'h') {
            return Parser::T_PATCH | Lexer::Variant_GLSL_400;
          }
        }
      }
    }
  }
  else if (s[0] == 'u') {
    if (s[1] == 'v') {
      if (s[2] == 'e') {
        if (s[3] == 'c') {
          if (s[4] == '2') {
            return Parser::T_UVEC2 | Lexer::Variant_GLSL_150;
          }
          else if (s[4] == '3') {
            return Parser::T_UVEC3 | Lexer::Variant_GLSL_150;
          }
          else if (s[4] == '4') {
            return Parser::T_UVEC4 | Lexer::Variant_GLSL_150;
          }
        }
      }
    }
  }
  else if (s[0] == 'w') {
    if (s[1] == 'h') {
      if (s[2] == 'i') {
        if (s[3] == 'l') {
          if (s[4] == 'e') {
            return Parser::T_WHILE;
          }
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify6(const char *s) {
  if (s[0] == 'd') {
    if (s[1] == 'o') {
      if (s[2] == 'u') {
        if (s[3] == 'b') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              return Parser::T_DOUBLE | Lexer::Variant_GLSL_400;
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'l') {
    if (s[1] == 'a') {
      if (s[2] == 'y') {
        if (s[3] == 'o') {
          if (s[4] == 'u') {
            if (s[5] == 't') {
              return Parser::T_LAYOUT | Lexer::Variant_GLSL_150;
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'm') {
    if (s[1] == 'a') {
      if (s[2] == 't') {
        if (s[3] == '2') {
          if (s[4] == 'x') {
            if (s[5] == '2') {
              return Parser::T_MAT2X2 | Lexer::Variant_GLSL_120;
            }
            else if (s[5] == '3') {
              return Parser::T_MAT2X3 | Lexer::Variant_GLSL_120;
            }
            else if (s[5] == '4') {
              return Parser::T_MAT2X4 | Lexer::Variant_GLSL_120;
            }
          }
        }
        else if (s[3] == '3') {
          if (s[4] == 'x') {
            if (s[5] == '2') {
              return Parser::T_MAT3X2 | Lexer::Variant_GLSL_120;
            }
            else if (s[5] == '3') {
              return Parser::T_MAT3X3 | Lexer::Variant_GLSL_120;
            }
            else if (s[5] == '4') {
              return Parser::T_MAT3X4 | Lexer::Variant_GLSL_120;
            }
          }
        }
        else if (s[3] == '4') {
          if (s[4] == 'x') {
            if (s[5] == '2') {
              return Parser::T_MAT4X2 | Lexer::Variant_GLSL_120;
            }
            else if (s[5] == '3') {
              return Parser::T_MAT4X3 | Lexer::Variant_GLSL_120;
            }
            else if (s[5] == '4') {
              return Parser::T_MAT4X4 | Lexer::Variant_GLSL_120;
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'r') {
    if (s[1] == 'e') {
      if (s[2] == 't') {
        if (s[3] == 'u') {
          if (s[4] == 'r') {
            if (s[5] == 'n') {
              return Parser::T_RETURN;
            }
          }
        }
      }
    }
  }
  else if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              return Parser::T_SAMPLE | Lexer::Variant_Reserved;
            }
          }
        }
      }
    }
    else if (s[1] == 'm') {
      if (s[2] == 'o') {
        if (s[3] == 'o') {
          if (s[4] == 't') {
            if (s[5] == 'h') {
              return Parser::T_SMOOTH | Lexer::Variant_GLSL_150;
            }
          }
        }
      }
    }
    else if (s[1] == 't') {
      if (s[2] == 'r') {
        if (s[3] == 'u') {
          if (s[4] == 'c') {
            if (s[5] == 't') {
              return Parser::T_STRUCT;
            }
          }
        }
      }
    }
    else if (s[1] == 'w') {
      if (s[2] == 'i') {
        if (s[3] == 't') {
          if (s[4] == 'c') {
            if (s[5] == 'h') {
              return Parser::T_SWITCH | Lexer::Variant_GLSL_150;
            }
          }
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify7(const char *s) {
  if (s[0] == 'd') {
    if (s[1] == 'e') {
      if (s[2] == 'f') {
        if (s[3] == 'a') {
          if (s[4] == 'u') {
            if (s[5] == 'l') {
              if (s[6] == 't') {
                return Parser::T_DEFAULT | Lexer::Variant_GLSL_150;
              }
            }
          }
        }
      }
    }
    else if (s[1] == 'i') {
      if (s[2] == 's') {
        if (s[3] == 'c') {
          if (s[4] == 'a') {
            if (s[5] == 'r') {
              if (s[6] == 'd') {
                return Parser::T_DISCARD | Lexer::Variant_FragmentShader;
              }
            }
          }
        }
      }
    }
    else if (s[1] == 'm') {
      if (s[2] == 'a') {
        if (s[3] == 't') {
          if (s[4] == '2') {
            if (s[5] == 'x') {
              if (s[6] == '2') {
                return Parser::T_DMAT2X2 | Lexer::Variant_GLSL_400;
              }
              else if (s[6] == '3') {
                return Parser::T_DMAT2X3 | Lexer::Variant_GLSL_400;
              }
              else if (s[6] == '4') {
                return Parser::T_DMAT2X4 | Lexer::Variant_GLSL_400;
              }
            }
          }
          else if (s[4] == '3') {
            if (s[5] == 'x') {
              if (s[6] == '2') {
                return Parser::T_DMAT3X2 | Lexer::Variant_GLSL_400;
              }
              else if (s[6] == '3') {
                return Parser::T_DMAT3X3 | Lexer::Variant_GLSL_400;
              }
              else if (s[6] == '4') {
                return Parser::T_DMAT3X4 | Lexer::Variant_GLSL_400;
              }
            }
          }
          else if (s[4] == '4') {
            if (s[5] == 'x') {
              if (s[6] == '2') {
                return Parser::T_DMAT4X2 | Lexer::Variant_GLSL_400;
              }
              else if (s[6] == '3') {
                return Parser::T_DMAT4X3 | Lexer::Variant_GLSL_400;
              }
              else if (s[6] == '4') {
                return Parser::T_DMAT4X4 | Lexer::Variant_GLSL_400;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'm') {
    if (s[1] == 'e') {
      if (s[2] == 'd') {
        if (s[3] == 'i') {
          if (s[4] == 'u') {
            if (s[5] == 'm') {
              if (s[6] == 'p') {
                return Parser::T_MEDIUMP | Lexer::Variant_GLSL_ES_100 | Lexer::Variant_GLSL_400;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'u') {
    if (s[1] == 'n') {
      if (s[2] == 'i') {
        if (s[3] == 'f') {
          if (s[4] == 'o') {
            if (s[5] == 'r') {
              if (s[6] == 'm') {
                return Parser::T_UNIFORM;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'v') {
    if (s[1] == 'a') {
      if (s[2] == 'r') {
        if (s[3] == 'y') {
          if (s[4] == 'i') {
            if (s[5] == 'n') {
              if (s[6] == 'g') {
                return Parser::T_VARYING;
              }
            }
          }
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify8(const char *s) {
  if (s[0] == 'c') {
    if (s[1] == 'e') {
      if (s[2] == 'n') {
        if (s[3] == 't') {
          if (s[4] == 'r') {
            if (s[5] == 'o') {
              if (s[6] == 'i') {
                if (s[7] == 'd') {
                  return Parser::T_CENTROID | Lexer::Variant_GLSL_120;
                }
              }
            }
          }
        }
      }
    }
    else if (s[1] == 'o') {
      if (s[2] == 'n') {
        if (s[3] == 't') {
          if (s[4] == 'i') {
            if (s[5] == 'n') {
              if (s[6] == 'u') {
                if (s[7] == 'e') {
                  return Parser::T_CONTINUE;
                }
              }
            }
          }
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify9(const char *s) {
  if (s[0] == 'a') {
    if (s[1] == 't') {
      if (s[2] == 't') {
        if (s[3] == 'r') {
          if (s[4] == 'i') {
            if (s[5] == 'b') {
              if (s[6] == 'u') {
                if (s[7] == 't') {
                  if (s[8] == 'e') {
                    return Parser::T_ATTRIBUTE | Lexer::Variant_VertexShader;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'i') {
    if (s[1] == 'n') {
      if (s[2] == 'v') {
        if (s[3] == 'a') {
          if (s[4] == 'r') {
            if (s[5] == 'i') {
              if (s[6] == 'a') {
                if (s[7] == 'n') {
                  if (s[8] == 't') {
                    return Parser::T_INVARIANT;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'p') {
    if (s[1] == 'r') {
      if (s[2] == 'e') {
        if (s[3] == 'c') {
          if (s[4] == 'i') {
            if (s[5] == 's') {
              if (s[6] == 'i') {
                if (s[7] == 'o') {
                  if (s[8] == 'n') {
                    return Parser::T_PRECISION | Lexer::Variant_GLSL_ES_100;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == '1') {
                  if (s[8] == 'D') {
                    return Parser::T_SAMPLER1D | Lexer::Variant_GLSL_120;
                  }
                }
                else if (s[7] == '2') {
                  if (s[8] == 'D') {
                    return Parser::T_SAMPLER2D;
                  }
                }
                else if (s[7] == '3') {
                  if (s[8] == 'D') {
                    return Parser::T_SAMPLER3D | Lexer::Variant_GLSL_120;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify10(const char *s) {
  if (s[0] == 'i') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '1') {
                    if (s[9] == 'D') {
                      return Parser::T_ISAMPLER1D | Lexer::Variant_GLSL_150;
                    }
                  }
                  else if (s[8] == '2') {
                    if (s[9] == 'D') {
                      return Parser::T_ISAMPLER2D | Lexer::Variant_GLSL_150;
                    }
                  }
                  else if (s[8] == '3') {
                    if (s[9] == 'D') {
                      return Parser::T_ISAMPLER3D | Lexer::Variant_GLSL_150;
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
  else if (s[0] == 's') {
    if (s[1] == 'u') {
      if (s[2] == 'b') {
        if (s[3] == 'r') {
          if (s[4] == 'o') {
            if (s[5] == 'u') {
              if (s[6] == 't') {
                if (s[7] == 'i') {
                  if (s[8] == 'n') {
                    if (s[9] == 'e') {
                      return Parser::T_SUBROUTINE | Lexer::Variant_GLSL_400;
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
  else if (s[0] == 'u') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '1') {
                    if (s[9] == 'D') {
                      return Parser::T_USAMPLER1D | Lexer::Variant_GLSL_150;
                    }
                  }
                  else if (s[8] == '2') {
                    if (s[9] == 'D') {
                      return Parser::T_USAMPLER2D | Lexer::Variant_GLSL_150;
                    }
                  }
                  else if (s[8] == '3') {
                    if (s[9] == 'D') {
                      return Parser::T_USAMPLER3D | Lexer::Variant_GLSL_150;
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
  return Parser::T_IDENTIFIER;
}

static inline int classify11(const char *s) {
  if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == '2') {
                  if (s[8] == 'D') {
                    if (s[9] == 'M') {
                      if (s[10] == 'S') {
                        return Parser::T_SAMPLER2DMS | Lexer::Variant_GLSL_150;
                      }
                    }
                  }
                }
                else if (s[7] == 'C') {
                  if (s[8] == 'u') {
                    if (s[9] == 'b') {
                      if (s[10] == 'e') {
                        return Parser::T_SAMPLERCUBE;
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
  return Parser::T_IDENTIFIER;
}

static inline int classify12(const char *s) {
  if (s[0] == 'i') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '2') {
                    if (s[9] == 'D') {
                      if (s[10] == 'M') {
                        if (s[11] == 'S') {
                          return Parser::T_ISAMPLER2DMS | Lexer::Variant_GLSL_150;
                        }
                      }
                    }
                  }
                  else if (s[8] == 'C') {
                    if (s[9] == 'u') {
                      if (s[10] == 'b') {
                        if (s[11] == 'e') {
                          return Parser::T_ISAMPLERCUBE | Lexer::Variant_GLSL_150;
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
  else if (s[0] == 'u') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '2') {
                    if (s[9] == 'D') {
                      if (s[10] == 'M') {
                        if (s[11] == 'S') {
                          return Parser::T_USAMPLER2DMS | Lexer::Variant_GLSL_150;
                        }
                      }
                    }
                  }
                  else if (s[8] == 'C') {
                    if (s[9] == 'u') {
                      if (s[10] == 'b') {
                        if (s[11] == 'e') {
                          return Parser::T_USAMPLERCUBE | Lexer::Variant_GLSL_150;
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
  return Parser::T_IDENTIFIER;
}

static inline int classify13(const char *s) {
  if (s[0] == 'n') {
    if (s[1] == 'o') {
      if (s[2] == 'p') {
        if (s[3] == 'e') {
          if (s[4] == 'r') {
            if (s[5] == 's') {
              if (s[6] == 'p') {
                if (s[7] == 'e') {
                  if (s[8] == 'c') {
                    if (s[9] == 't') {
                      if (s[10] == 'i') {
                        if (s[11] == 'v') {
                          if (s[12] == 'e') {
                            return Parser::T_NOPERSPECTIVE | Lexer::Variant_GLSL_150;
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
  }
  else if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == '2') {
                  if (s[8] == 'D') {
                    if (s[9] == 'R') {
                      if (s[10] == 'e') {
                        if (s[11] == 'c') {
                          if (s[12] == 't') {
                            return Parser::T_SAMPLER2DRECT;
                          }
                        }
                      }
                    }
                  }
                }
                else if (s[7] == 'B') {
                  if (s[8] == 'u') {
                    if (s[9] == 'f') {
                      if (s[10] == 'f') {
                        if (s[11] == 'e') {
                          if (s[12] == 'r') {
                            return Parser::T_SAMPLERBUFFER | Lexer::Variant_GLSL_150;
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
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify14(const char *s) {
  if (s[0] == 'i') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '2') {
                    if (s[9] == 'D') {
                      if (s[10] == 'R') {
                        if (s[11] == 'e') {
                          if (s[12] == 'c') {
                            if (s[13] == 't') {
                              return Parser::T_ISAMPLER2DRECT | Lexer::Variant_GLSL_150;
                            }
                          }
                        }
                      }
                    }
                  }
                  else if (s[8] == 'B') {
                    if (s[9] == 'u') {
                      if (s[10] == 'f') {
                        if (s[11] == 'f') {
                          if (s[12] == 'e') {
                            if (s[13] == 'r') {
                              return Parser::T_ISAMPLERBUFFER | Lexer::Variant_GLSL_150;
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
    }
  }
  else if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == '1') {
                  if (s[8] == 'D') {
                    if (s[9] == 'A') {
                      if (s[10] == 'r') {
                        if (s[11] == 'r') {
                          if (s[12] == 'a') {
                            if (s[13] == 'y') {
                              return Parser::T_SAMPLER1DARRAY | Lexer::Variant_GLSL_150;
                            }
                          }
                        }
                      }
                    }
                  }
                }
                else if (s[7] == '2') {
                  if (s[8] == 'D') {
                    if (s[9] == 'A') {
                      if (s[10] == 'r') {
                        if (s[11] == 'r') {
                          if (s[12] == 'a') {
                            if (s[13] == 'y') {
                              return Parser::T_SAMPLER2DARRAY | Lexer::Variant_GLSL_150;
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
    }
  }
  else if (s[0] == 'u') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '2') {
                    if (s[9] == 'D') {
                      if (s[10] == 'R') {
                        if (s[11] == 'e') {
                          if (s[12] == 'c') {
                            if (s[13] == 't') {
                              return Parser::T_USAMPLER2DRECT | Lexer::Variant_GLSL_150;
                            }
                          }
                        }
                      }
                    }
                  }
                  else if (s[8] == 'B') {
                    if (s[9] == 'u') {
                      if (s[10] == 'f') {
                        if (s[11] == 'f') {
                          if (s[12] == 'e') {
                            if (s[13] == 'r') {
                              return Parser::T_USAMPLERBUFFER | Lexer::Variant_GLSL_150;
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
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify15(const char *s) {
  if (s[0] == 'i') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '1') {
                    if (s[9] == 'D') {
                      if (s[10] == 'A') {
                        if (s[11] == 'r') {
                          if (s[12] == 'r') {
                            if (s[13] == 'a') {
                              if (s[14] == 'y') {
                                return Parser::T_ISAMPLER1DARRAY | Lexer::Variant_GLSL_150;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                  else if (s[8] == '2') {
                    if (s[9] == 'D') {
                      if (s[10] == 'A') {
                        if (s[11] == 'r') {
                          if (s[12] == 'r') {
                            if (s[13] == 'a') {
                              if (s[14] == 'y') {
                                return Parser::T_ISAMPLER2DARRAY | Lexer::Variant_GLSL_150;
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
      }
    }
  }
  else if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == '1') {
                  if (s[8] == 'D') {
                    if (s[9] == 'S') {
                      if (s[10] == 'h') {
                        if (s[11] == 'a') {
                          if (s[12] == 'd') {
                            if (s[13] == 'o') {
                              if (s[14] == 'w') {
                                return Parser::T_SAMPLER1DSHADOW | Lexer::Variant_GLSL_120;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
                else if (s[7] == '2') {
                  if (s[8] == 'D') {
                    if (s[9] == 'S') {
                      if (s[10] == 'h') {
                        if (s[11] == 'a') {
                          if (s[12] == 'd') {
                            if (s[13] == 'o') {
                              if (s[14] == 'w') {
                                return Parser::T_SAMPLER2DSHADOW | Lexer::Variant_GLSL_120;
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
      }
    }
  }
  else if (s[0] == 'u') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '1') {
                    if (s[9] == 'D') {
                      if (s[10] == 'A') {
                        if (s[11] == 'r') {
                          if (s[12] == 'r') {
                            if (s[13] == 'a') {
                              if (s[14] == 'y') {
                                return Parser::T_USAMPLER1DARRAY | Lexer::Variant_GLSL_150;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                  else if (s[8] == '2') {
                    if (s[9] == 'D') {
                      if (s[10] == 'A') {
                        if (s[11] == 'r') {
                          if (s[12] == 'r') {
                            if (s[13] == 'a') {
                              if (s[14] == 'y') {
                                return Parser::T_USAMPLER2DARRAY | Lexer::Variant_GLSL_150;
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
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify16(const char *s) {
  if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == '2') {
                  if (s[8] == 'D') {
                    if (s[9] == 'M') {
                      if (s[10] == 'S') {
                        if (s[11] == 'A') {
                          if (s[12] == 'r') {
                            if (s[13] == 'r') {
                              if (s[14] == 'a') {
                                if (s[15] == 'y') {
                                  return Parser::T_SAMPLER2DMSARRAY | Lexer::Variant_GLSL_150;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
                else if (s[7] == 'C') {
                  if (s[8] == 'u') {
                    if (s[9] == 'b') {
                      if (s[10] == 'e') {
                        if (s[11] == 'A') {
                          if (s[12] == 'r') {
                            if (s[13] == 'r') {
                              if (s[14] == 'a') {
                                if (s[15] == 'y') {
                                  return Parser::T_SAMPLERCUBEARRAY | Lexer::Variant_GLSL_400;
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
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify17(const char *s) {
  if (s[0] == 'i') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '2') {
                    if (s[9] == 'D') {
                      if (s[10] == 'M') {
                        if (s[11] == 'S') {
                          if (s[12] == 'A') {
                            if (s[13] == 'r') {
                              if (s[14] == 'r') {
                                if (s[15] == 'a') {
                                  if (s[16] == 'y') {
                                    return Parser::T_ISAMPLER2DMSARRAY | Lexer::Variant_GLSL_150;
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                  else if (s[8] == 'C') {
                    if (s[9] == 'u') {
                      if (s[10] == 'b') {
                        if (s[11] == 'e') {
                          if (s[12] == 'A') {
                            if (s[13] == 'r') {
                              if (s[14] == 'r') {
                                if (s[15] == 'a') {
                                  if (s[16] == 'y') {
                                    return Parser::T_ISAMPLERCUBEARRAY | Lexer::Variant_GLSL_400;
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
          }
        }
      }
    }
  }
  else if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == 'C') {
                  if (s[8] == 'u') {
                    if (s[9] == 'b') {
                      if (s[10] == 'e') {
                        if (s[11] == 'S') {
                          if (s[12] == 'h') {
                            if (s[13] == 'a') {
                              if (s[14] == 'd') {
                                if (s[15] == 'o') {
                                  if (s[16] == 'w') {
                                    return Parser::T_SAMPLERCUBESHADOW | Lexer::Variant_GLSL_400;
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
          }
        }
      }
    }
  }
  else if (s[0] == 'u') {
    if (s[1] == 's') {
      if (s[2] == 'a') {
        if (s[3] == 'm') {
          if (s[4] == 'p') {
            if (s[5] == 'l') {
              if (s[6] == 'e') {
                if (s[7] == 'r') {
                  if (s[8] == '2') {
                    if (s[9] == 'D') {
                      if (s[10] == 'M') {
                        if (s[11] == 'S') {
                          if (s[12] == 'a') {
                            if (s[13] == 'r') {
                              if (s[14] == 'r') {
                                if (s[15] == 'a') {
                                  if (s[16] == 'y') {
                                    return Parser::T_USAMPLER2DMSARRAY | Lexer::Variant_GLSL_150;
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                  else if (s[8] == 'C') {
                    if (s[9] == 'u') {
                      if (s[10] == 'b') {
                        if (s[11] == 'e') {
                          if (s[12] == 'A') {
                            if (s[13] == 'r') {
                              if (s[14] == 'r') {
                                if (s[15] == 'a') {
                                  if (s[16] == 'y') {
                                    return Parser::T_USAMPLERCUBEARRAY | Lexer::Variant_GLSL_400;
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
          }
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify19(const char *s) {
  if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == '2') {
                  if (s[8] == 'D') {
                    if (s[9] == 'R') {
                      if (s[10] == 'e') {
                        if (s[11] == 'c') {
                          if (s[12] == 't') {
                            if (s[13] == 'S') {
                              if (s[14] == 'h') {
                                if (s[15] == 'a') {
                                  if (s[16] == 'd') {
                                    if (s[17] == 'o') {
                                      if (s[18] == 'w') {
                                        return Parser::T_SAMPLER2DRECTSHADOW;
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
              }
            }
          }
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify20(const char *s) {
  if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == '1') {
                  if (s[8] == 'D') {
                    if (s[9] == 'A') {
                      if (s[10] == 'r') {
                        if (s[11] == 'r') {
                          if (s[12] == 'a') {
                            if (s[13] == 'y') {
                              if (s[14] == 'S') {
                                if (s[15] == 'h') {
                                  if (s[16] == 'a') {
                                    if (s[17] == 'd') {
                                      if (s[18] == 'o') {
                                        if (s[19] == 'w') {
                                          return Parser::T_SAMPLER1DARRAYSHADOW | Lexer::Variant_GLSL_150;
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
                }
                else if (s[7] == '2') {
                  if (s[8] == 'D') {
                    if (s[9] == 'A') {
                      if (s[10] == 'r') {
                        if (s[11] == 'r') {
                          if (s[12] == 'a') {
                            if (s[13] == 'y') {
                              if (s[14] == 'S') {
                                if (s[15] == 'h') {
                                  if (s[16] == 'a') {
                                    if (s[17] == 'd') {
                                      if (s[18] == 'o') {
                                        if (s[19] == 'w') {
                                          return Parser::T_SAMPLER2DARRAYSHADOW | Lexer::Variant_GLSL_150;
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
                }
              }
            }
          }
        }
      }
    }
  }
  return Parser::T_IDENTIFIER;
}

static inline int classify22(const char *s) {
  if (s[0] == 's') {
    if (s[1] == 'a') {
      if (s[2] == 'm') {
        if (s[3] == 'p') {
          if (s[4] == 'l') {
            if (s[5] == 'e') {
              if (s[6] == 'r') {
                if (s[7] == 'C') {
                  if (s[8] == 'u') {
                    if (s[9] == 'b') {
                      if (s[10] == 'e') {
                        if (s[11] == 'A') {
                          if (s[12] == 'r') {
                            if (s[13] == 'r') {
                              if (s[14] == 'a') {
                                if (s[15] == 'y') {
                                  if (s[16] == 'S') {
                                    if (s[17] == 'h') {
                                      if (s[18] == 'a') {
                                        if (s[19] == 'd') {
                                          if (s[20] == 'o') {
                                            if (s[21] == 'w') {
                                              return Parser::T_SAMPLERCUBEARRAYSHADOW | Lexer::Variant_GLSL_400;
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
  return Parser::T_IDENTIFIER;
}

int Lexer::classify(const char *s, int n) {
  switch (n) {
    case 2: return classify2(s);
    case 3: return classify3(s);
    case 4: return classify4(s);
    case 5: return classify5(s);
    case 6: return classify6(s);
    case 7: return classify7(s);
    case 8: return classify8(s);
    case 9: return classify9(s);
    case 10: return classify10(s);
    case 11: return classify11(s);
    case 12: return classify12(s);
    case 13: return classify13(s);
    case 14: return classify14(s);
    case 15: return classify15(s);
    case 16: return classify16(s);
    case 17: return classify17(s);
    case 19: return classify19(s);
    case 20: return classify20(s);
    case 22: return classify22(s);
    default: return Parser::T_IDENTIFIER;
  } // switch
}

QStringList Lexer::keywords(int variant) {
  QStringList list;
  list += QLatin1String("do");
  list += QLatin1String("if");
  list += QLatin1String("in");
  list += QLatin1String("for");
  list += QLatin1String("int");
  list += QLatin1String("out");
  list += QLatin1String("bool");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("case");
  list += QLatin1String("else");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("flat");
  if (variant & (Lexer::Variant_GLSL_ES_100 | Lexer::Variant_GLSL_400))
    list += QLatin1String("lowp");
  list += QLatin1String("mat2");
  list += QLatin1String("mat3");
  list += QLatin1String("mat4");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("uint");
  list += QLatin1String("vec2");
  list += QLatin1String("vec3");
  list += QLatin1String("vec4");
  list += QLatin1String("void");
  list += QLatin1String("true");
  list += QLatin1String("break");
  list += QLatin1String("bvec2");
  list += QLatin1String("bvec3");
  list += QLatin1String("bvec4");
  list += QLatin1String("const");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat2");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat3");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat4");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dvec2");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dvec3");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dvec4");
  list += QLatin1String("float");
  if (variant & (Lexer::Variant_GLSL_ES_100 | Lexer::Variant_GLSL_400))
    list += QLatin1String("highp");
  list += QLatin1String("inout");
  list += QLatin1String("ivec2");
  list += QLatin1String("ivec3");
  list += QLatin1String("ivec4");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("patch");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("uvec2");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("uvec3");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("uvec4");
  list += QLatin1String("while");
  list += QLatin1String("false");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("double");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("layout");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat2x2");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat2x3");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat2x4");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat3x2");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat3x3");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat3x4");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat4x2");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat4x3");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("mat4x4");
  list += QLatin1String("return");
  if (variant & (Lexer::Variant_Reserved))
    list += QLatin1String("sample");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("smooth");
  list += QLatin1String("struct");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("switch");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("default");
  if (variant & (Lexer::Variant_FragmentShader))
    list += QLatin1String("discard");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat2x2");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat2x3");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat2x4");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat3x2");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat3x3");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat3x4");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat4x2");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat4x3");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("dmat4x4");
  if (variant & (Lexer::Variant_GLSL_ES_100 | Lexer::Variant_GLSL_400))
    list += QLatin1String("mediump");
  list += QLatin1String("uniform");
  list += QLatin1String("varying");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("centroid");
  list += QLatin1String("continue");
  if (variant & (Lexer::Variant_VertexShader))
    list += QLatin1String("attribute");
  list += QLatin1String("invariant");
  if (variant & (Lexer::Variant_GLSL_ES_100))
    list += QLatin1String("precision");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("sampler1D");
  list += QLatin1String("sampler2D");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("sampler3D");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isampler1D");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isampler2D");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isampler3D");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("subroutine");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usampler1D");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usampler2D");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usampler3D");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("sampler2DMS");
  list += QLatin1String("samplerCube");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isampler2DMS");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isamplerCube");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usampler2DMS");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usamplerCube");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("noperspective");
  list += QLatin1String("sampler2DRect");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("samplerBuffer");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isampler2DRect");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isamplerBuffer");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("sampler1DArray");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("sampler2DArray");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usampler2DRect");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usamplerBuffer");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isampler1DArray");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isampler2DArray");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("sampler1DShadow");
  if (variant & (Lexer::Variant_GLSL_120))
    list += QLatin1String("sampler2DShadow");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usampler1DArray");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usampler2DArray");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("sampler2DMSArray");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("samplerCubeArray");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("isampler2DMSArray");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("isamplerCubeArray");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("samplerCubeShadow");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("usampler2DMSarray");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("usamplerCubeArray");
  list += QLatin1String("sampler2DRectShadow");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("sampler1DArrayShadow");
  if (variant & (Lexer::Variant_GLSL_150))
    list += QLatin1String("sampler2DArrayShadow");
  if (variant & (Lexer::Variant_GLSL_400))
    list += QLatin1String("samplerCubeArrayShadow");
  return list;
}

