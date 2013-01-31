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

#include "cgi.h"

#include <QByteArray>

const char cgi_chars[] = "0123456789abcdef"; // RFC 1738 suggests lower-case to be optimal

QString CGI::encodeURL(const QString &rawText)
{
    QByteArray utf = rawText.toUtf8();
    QString enc;
    enc.reserve(utf.length()); // Make sure we at least have space for a normal US-ASCII URL

    QByteArray::const_iterator it = utf.constBegin();
    while (it != utf.constEnd()) {
        const char ch = *it;
        if (('A' <= ch && ch <= 'Z')
            || ('a' <= ch && ch <= 'z')
            || ('0' <= ch && ch <= '9'))
            enc.append(QLatin1Char(ch));
        else if (ch == ' ')
            enc.append(QLatin1Char('+'));
        else {
            switch (ch) {
            case '-': case '_':
            case '(': case ')':
            case '.': case '!':
            case '~': case '*':
            case '\'':
                enc.append(QLatin1Char(ch));
                break;
            default:
                ushort c1 = (*it & 0xF0) >> 4;
                ushort c2 = (*it & 0x0F);
                enc.append(QLatin1Char('%'));
                enc.append(QLatin1Char(*(cgi_chars + c1)));
                enc.append(QLatin1Char(*(cgi_chars + c2)));
                break;
            }
        }
        ++it;
    }
    return enc;
}

QString CGI::decodeURL(const QString &urlText)
{
    QByteArray dec;
    QString::const_iterator it = urlText.constBegin();
    while (it != urlText.constEnd()) {
        ushort ch = (*it).unicode();
        switch (ch) {
        case '%':
            {
                char c1 = char(0x00ff & (*(++it)).unicode());
                char c2 = char(0x00ff & (*(++it)).unicode());
                ushort v = 0;
                if ('A' <= c1 && c1 <= 'Z')
                    v = c1 - 'A' + 10;
                else if ('a' <= c1 && c1 <= 'z')
                    v = c1 - 'a' + 10;
                else if ('0' <= c1 && c1 <= '9')
                    v = c1 - '0';
                else
                    continue; // Malformed URL!
                v <<= 4; // c1 was MSB half
                if ('A' <= c2 && c2 <= 'Z')
                    v |= c2 - 'A' + 10;
                else if ('a' <= c2 && c2 <= 'z')
                    v |= c2 - 'a' + 10;
                else if ('0' <= c2 && c2 <= '9')
                    v |= c2 - '0';
                else
                    continue; // Malformed URL!
                dec.append((char)v);
            }
            break;
        case '+':
            dec.append(' ');
            break;
        default:
            if (ch < 256) {
                dec.append(ch);
            } else {
                // should not happen with proper URLs but stay on the safe side
                dec.append(QString(*it).toUtf8());
            }
            break;
        }
        ++it;
    }
    return QString::fromUtf8(dec.constData(), dec.length());
}

// -------------------------------------------------------------------------------------------------
inline const char *unicodeToHTML(ushort unicode_char)
{
    switch (unicode_char) {
    // Latin -------------------------------
    case 0x0022: return "quot";    // (34  ) quotation mark = APL quote
    case 0x0026: return "amp";     // (38  ) ampersand
    case 0x003C: return "lt";      // (60  ) less-than sign
    case 0x003E: return "gt";      // (62  ) greater-than sign
    case 0x00A0: return "nbsp";    // (160 ) no-break space = non-breaking space
    case 0x00A1: return "iexcl";   // (161 ) inverted exclamation mark
    case 0x00A2: return "cent";    // (162 ) cent sign
    case 0x00A3: return "pound";   // (163 ) pound sign
    case 0x00A4: return "curren";  // (164 ) currency sign
    case 0x00A5: return "yen";     // (165 ) yen sign = yuan sign
    case 0x00A6: return "brvbar";  // (166 ) broken bar = broken vertical bar
    case 0x00A7: return "sect";    // (167 ) section sign
    case 0x00A8: return "uml";     // (168 ) diaeresis = spacing diaeresis
    case 0x00A9: return "copy";    // (169 ) copyright sign
    case 0x00AA: return "ordf";    // (170 ) feminine ordinal indicator
    case 0x00AB: return "laquo";   // (171 ) left-pointing double angle quotation mark = left pointing guillemet
    case 0x00AC: return "not";     // (172 ) not sign
    case 0x00AD: return "shy";     // (173 ) soft hyphen = discretionary hyphen
    case 0x00AE: return "reg";     // (174 ) registered sign = registered trade mark sign
    case 0x00AF: return "macr";    // (175 ) macron = spacing macron = overline = APL overbar
    case 0x00B0: return "deg";     // (176 ) degree sign
    case 0x00B1: return "plusmn";  // (177 ) plus-minus sign = plus-or-minus sign
    case 0x00B2: return "sup2";    // (178 ) superscript two = superscript digit two = squared
    case 0x00B3: return "sup3";    // (179 ) superscript three = superscript digit three = cubed
    case 0x00B4: return "acute";   // (180 ) acute accent = spacing acute
    case 0x00B5: return "micro";   // (181 ) micro sign
    case 0x00B6: return "para";    // (182 ) pilcrow sign = paragraph sign
    case 0x00B7: return "middot";  // (183 ) middle dot = Georgian comma = Greek middle dot
    case 0x00B8: return "cedil";   // (184 ) cedilla = spacing cedilla
    case 0x00B9: return "sup1";    // (185 ) superscript one = superscript digit one
    case 0x00BA: return "ordm";    // (186 ) masculine ordinal indicator
    case 0x00BB: return "raquo";   // (187 ) right-pointing double angle quotation mark = right pointing guillemet
    case 0x00BC: return "frac14";  // (188 ) vulgar fraction one quarter = fraction one quarter
    case 0x00BD: return "frac12";  // (189 ) vulgar fraction one half = fraction one half
    case 0x00BE: return "frac34";  // (190 ) vulgar fraction three quarters = fraction three quarters
    case 0x00BF: return "iquest";  // (191 ) inverted question mark = turned question mark
    case 0x00C0: return "Agrave";  // (192 ) capital letter A with grave = capital letter
    case 0x00C1: return "Aacute";  // (193 ) capital letter A with acute
    case 0x00C2: return "Acirc";   // (194 ) capital letter A with circumflex
    case 0x00C3: return "Atilde";  // (195 ) capital letter A with tilde
    case 0x00C4: return "Auml";    // (196 ) capital letter A with diaeresis
    case 0x00C5: return "Aring";   // (197 ) capital letter A with ring above = capital letter
    case 0x00C6: return "AElig";   // (198 ) capital letter AE =  capital ligature
    case 0x00C7: return "Ccedil";  // (199 ) capital letter C with cedilla
    case 0x00C8: return "Egrave";  // (200 ) capital letter E with grave
    case 0x00C9: return "Eacute";  // (201 ) capital letter E with acute
    case 0x00CA: return "Ecirc";   // (202 ) capital letter E with circumflex
    case 0x00CB: return "Euml";    // (203 ) capital letter E with diaeresis
    case 0x00CC: return "Igrave";  // (204 ) capital letter I with grave
    case 0x00CD: return "Iacute";  // (205 ) capital letter I with acute
    case 0x00CE: return "Icirc";   // (206 ) capital letter I with circumflex
    case 0x00CF: return "Iuml";    // (207 ) capital letter I with diaeresis
    case 0x00D0: return "ETH";     // (208 ) capital letter ETH
    case 0x00D1: return "Ntilde";  // (209 ) capital letter N with tilde
    case 0x00D2: return "Ograve";  // (210 ) capital letter O with grave
    case 0x00D3: return "Oacute";  // (211 ) capital letter O with acute
    case 0x00D4: return "Ocirc";   // (212 ) capital letter O with circumflex
    case 0x00D5: return "Otilde";  // (213 ) capital letter O with tilde
    case 0x00D6: return "Ouml";    // (214 ) capital letter O with diaeresis
    case 0x00D7: return "times";   // (215 ) multiplication sign
    case 0x00D8: return "Oslash";  // (216 ) capital letter O with stroke = capital letter
    case 0x00D9: return "Ugrave";  // (217 ) capital letter U with grave
    case 0x00DA: return "Uacute";  // (218 ) capital letter U with acute
    case 0x00DB: return "Ucirc";   // (219 ) capital letter U with circumflex
    case 0x00DC: return "Uuml";    // (220 ) capital letter U with diaeresis
    case 0x00DD: return "Yacute";  // (221 ) capital letter Y with acute
    case 0x00DE: return "THORN";   // (222 ) capital letter THORN
    case 0x00DF: return "szlig";   // (223 ) small letter sharp s = ess-zed
    case 0x00E0: return "agrave";  // (224 ) small letter a with grave = small letter
    case 0x00E1: return "aacute";  // (225 ) small letter a with acute
    case 0x00E2: return "acirc";   // (226 ) small letter a with circumflex
    case 0x00E3: return "atilde";  // (227 ) small letter a with tilde
    case 0x00E4: return "auml";    // (228 ) small letter a with diaeresis
    case 0x00E5: return "aring";   // (229 ) small letter a with ring above = small letter
    case 0x00E6: return "aelig";   // (230 ) small letter ae = small letter
    case 0x00E7: return "ccedil";  // (231 ) small letter c with cedilla
    case 0x00E8: return "egrave";  // (232 ) small letter e with grave
    case 0x00E9: return "eacute";  // (233 ) small letter e with acute
    case 0x00EA: return "ecirc";   // (234 ) small letter e with circumflex
    case 0x00EB: return "euml";    // (235 ) small letter e with diaeresis
    case 0x00EC: return "igrave";  // (236 ) small letter i with grave
    case 0x00ED: return "iacute";  // (237 ) small letter i with acute
    case 0x00EE: return "icirc";   // (238 ) small letter i with circumflex
    case 0x00EF: return "iuml";    // (239 ) small letter i with diaeresis
    case 0x00F0: return "eth";     // (240 ) small letter eth
    case 0x00F1: return "ntilde";  // (241 ) small letter n with tilde
    case 0x00F2: return "ograve";  // (242 ) small letter o with grave
    case 0x00F3: return "oacute";  // (243 ) small letter o with acute
    case 0x00F4: return "ocirc";   // (244 ) small letter o with circumflex
    case 0x00F5: return "otilde";  // (245 ) small letter o with tilde
    case 0x00F6: return "ouml";    // (246 ) small letter o with diaeresis
    case 0x00F7: return "divide";  // (247 ) division sign
    case 0x00F8: return "oslash";  // (248 ) small letter o with stroke = small letter
    case 0x00F9: return "ugrave";  // (249 ) small letter u with grave
    case 0x00FA: return "uacute";  // (250 ) small letter u with acute
    case 0x00FB: return "ucirc";   // (251 ) small letter u with circumflex
    case 0x00FC: return "uuml";    // (252 ) small letter u with diaeresis
    case 0x00FD: return "yacute";  // (253 ) small letter y with acute
    case 0x00FE: return "thorn";   // (254 ) small letter thorn
    case 0x00FF: return "yuml";    // (255 ) small letter y with diaeresis
    case 0x0152: return "OElig";   // (338 ) capital ligature OE
    case 0x0153: return "oelig";   // (339 ) small ligature oe
    case 0x0160: return "Scaron";  // (352 ) capital letter S with caron
    case 0x0161: return "scaron";  // (353 ) small letter s with caron
    case 0x0178: return "Yuml";    // (376 ) capital letter Y with diaeresis
    case 0x0192: return "fnof";    // (402 ) small f with hook = function = florin
    case 0x02C6: return "circ";    // (710 ) modifier letter circumflex accent
    case 0x02DC: return "tilde";   // (732 ) small tilde
    // Greek -------------------------------
    case 0x0391: return "Alpha";   // (913 ) capital letter alpha
    case 0x0392: return "Beta";    // (914 ) capital letter beta
    case 0x0393: return "Gamma";   // (915 ) capital letter gamma
    case 0x0394: return "Delta";   // (916 ) capital letter delta
    case 0x0395: return "Epsilon"; // (917 ) capital letter epsilon
    case 0x0396: return "Zeta";    // (918 ) capital letter zeta
    case 0x0397: return "Eta";     // (919 ) capital letter eta
    case 0x0398: return "Theta";   // (920 ) capital letter theta
    case 0x0399: return "Iota";    // (921 ) capital letter iota
    case 0x039A: return "Kappa";   // (922 ) capital letter kappa
    case 0x039B: return "Lambda";  // (923 ) capital letter lambda
    case 0x039C: return "Mu";      // (924 ) capital letter mu
    case 0x039D: return "Nu";      // (925 ) capital letter nu
    case 0x039E: return "Xi";      // (926 ) capital letter xi
    case 0x039F: return "Omicron"; // (927 ) capital letter omicron
    case 0x03A0: return "Pi";      // (928 ) capital letter pi
    case 0x03A1: return "Rho";     // (929 ) capital letter rho
    case 0x03A3: return "Sigma";   // (931 ) capital letter sigma
    case 0x03A4: return "Tau";     // (932 ) capital letter tau
    case 0x03A5: return "Upsilon"; // (933 ) capital letter upsilon
    case 0x03A6: return "Phi";     // (934 ) capital letter phi
    case 0x03A7: return "Chi";     // (935 ) capital letter chi
    case 0x03A8: return "Psi";     // (936 ) capital letter psi
    case 0x03A9: return "Omega";   // (937 ) capital letter omega
    case 0x03B1: return "alpha";   // (945 ) small letter alpha
    case 0x03B2: return "beta";    // (946 ) small letter beta
    case 0x03B3: return "gamma";   // (947 ) small letter gamma
    case 0x03B4: return "delta";   // (948 ) small letter delta
    case 0x03B5: return "epsilon"; // (949 ) small letter epsilon
    case 0x03B6: return "zeta";    // (950 ) small letter zeta
    case 0x03B7: return "eta";     // (951 ) small letter eta
    case 0x03B8: return "theta";   // (952 ) small letter theta
    case 0x03B9: return "iota";    // (953 ) small letter iota
    case 0x03BA: return "kappa";   // (954 ) small letter kappa
    case 0x03BB: return "lambda";  // (955 ) small letter lambda
    case 0x03BC: return "mu";      // (956 ) small letter mu
    case 0x03BD: return "nu";      // (957 ) small letter nu
    case 0x03BE: return "xi";      // (958 ) small letter xi
    case 0x03BF: return "omicron"; // (959 ) small letter omicron
    case 0x03C0: return "pi";      // (960 ) small letter pi
    case 0x03C1: return "rho";     // (961 ) small letter rho
    case 0x03C2: return "sigmaf";  // (962 ) small letter final sigma
    case 0x03C3: return "sigma";   // (963 ) small letter sigma
    case 0x03C4: return "tau";     // (964 ) small letter tau
    case 0x03C5: return "upsilon"; // (965 ) small letter upsilon
    case 0x03C6: return "phi";     // (966 ) small letter phi
    case 0x03C7: return "chi";     // (967 ) small letter chi
    case 0x03C8: return "psi";     // (968 ) small letter psi
    case 0x03C9: return "omega";   // (969 ) small letter omega
    case 0x03D1: return "thetasym";// (977 ) small letter theta symbol
    case 0x03D2: return "upsih";   // (978 ) upsilon with hook symbol
    case 0x03D6: return "piv";     // (982 ) pi symbol
    // General Punctuation -----------------
    case 0x2002: return "ensp";    // (8194) en space
    case 0x2003: return "emsp";    // (8195) em space
    case 0x2009: return "thinsp";  // (8201) thin space
    case 0x200C: return "zwnj";    // (8204) zero width non-joiner
    case 0x200D: return "zwj";     // (8205) zero width joiner
    case 0x200E: return "lrm";     // (8206) left-to-right mark
    case 0x200F: return "rlm";     // (8207) right-to-left mark
    case 0x2013: return "ndash";   // (8211) en dash
    case 0x2014: return "mdash";   // (8212) em dash
    case 0x2018: return "lsquo";   // (8216) left single quotation mark
    case 0x2019: return "rsquo";   // (8217) right single quotation mark
    case 0x201A: return "sbquo";   // (8218) single low-9 quotation mark
    case 0x201C: return "ldquo";   // (8220) left double quotation mark
    case 0x201D: return "rdquo";   // (8221) right double quotation mark
    case 0x201E: return "bdquo";   // (8222) double low-9 quotation mark
    case 0x2020: return "dagger";  // (8224) dagger
    case 0x2021: return "Dagger";  // (8225) double dagger
    case 0x2022: return "bull";    // (8226) bullet = black small circle
    case 0x2026: return "hellip";  // (8230) horizontal ellipsis = three dot leader
    case 0x2030: return "permil";  // (8240) per mille sign
    case 0x2032: return "prime";   // (8242) prime = minutes = feet
    case 0x2033: return "Prime";   // (8243) double prime = seconds = inches
    case 0x2039: return "lsaquo";  // (8249) single left-pointing angle quotation mark
    case 0x203A: return "rsaquo";  // (8250) single right-pointing angle quotation mark
    case 0x203E: return "oline";   // (8254) overline = spacing overscore
    case 0x2044: return "frasl";   // (8260) fraction slash
    // Currency Symbols --------------------
    case 0x20AC: return "euro";    // (8364) euro sign
    // Letterlike Symbols ------------------
    case 0x2111: return "image";   // (8465) blackletter capital I = imaginary part
    case 0x2118: return "weierp";  // (8472) script capital P = power set = Weierstrass p
    case 0x211C: return "real";    // (8476) blackletter capital R = real part symbol
    case 0x2122: return "trade";   // (8482) trade mark sign
    case 0x2135: return "alefsym"; // (8501) alef symbol = first transfinite cardinal
    // Arrows ------------------------------
    case 0x2190: return "larr";    // (8592) leftwards arrow
    case 0x2191: return "uarr";    // (8593) upwards arrow
    case 0x2192: return "rarr";    // (8594) rightwards arrow
    case 0x2193: return "darr";    // (8595) downwards arrow
    case 0x2194: return "harr";    // (8596) left right arrow
    case 0x21B5: return "crarr";   // (8629) downwards arrow with corner leftwards = carriage return
    case 0x21D0: return "lArr";    // (8656) leftwards double arrow
    case 0x21D1: return "uArr";    // (8657) upwards double arrow
    case 0x21D2: return "rArr";    // (8658) rightwards double arrow
    case 0x21D3: return "dArr";    // (8659) downwards double arrow
    case 0x21D4: return "hArr";    // (8660) left right double arrow
    // Mathematical Operators --------------
    case 0x2200: return "forall";  // (8704) for all
    case 0x2202: return "part";    // (8706) partial differential
    case 0x2203: return "exist";   // (8707) there exists
    case 0x2205: return "empty";   // (8709) empty set = null set = diameter
    case 0x2207: return "nabla";   // (8711) nabla = backward difference
    case 0x2208: return "isin";    // (8712) element of
    case 0x2209: return "notin";   // (8713) not an element of
    case 0x220B: return "ni";      // (8715) contains as member
    case 0x220F: return "prod";    // (8719) n-ary product = product sign
    case 0x2211: return "sum";     // (8721) n-ary sumation
    case 0x2212: return "minus";   // (8722) minus sign
    case 0x2217: return "lowast";  // (8727) asterisk operator
    case 0x221A: return "radic";   // (8730) square root = radical sign
    case 0x221D: return "prop";    // (8733) proportional to
    case 0x221E: return "infin";   // (8734) infinity
    case 0x2220: return "ang";     // (8736) angle
    case 0x2227: return "and";     // (8743) logical and = wedge
    case 0x2228: return "or";      // (8744) logical or = vee
    case 0x2229: return "cap";     // (8745) intersection = cap
    case 0x222A: return "cup";     // (8746) union = cup
    case 0x222B: return "int";     // (8747) integral
    case 0x2234: return "there4";  // (8756) therefore
    case 0x223C: return "sim";     // (8764) tilde operator = varies with = similar to
    case 0x2245: return "cong";    // (8773) approximately equal to
    case 0x2248: return "asymp";   // (8776) almost equal to = asymptotic to
    case 0x2260: return "ne";      // (8800) not equal to
    case 0x2261: return "equiv";   // (8801) identical to
    case 0x2264: return "le";      // (8804) less-than or equal to
    case 0x2265: return "ge";      // (8805) greater-than or equal to
    case 0x2282: return "sub";     // (8834) subset of
    case 0x2283: return "sup";     // (8835) superset of
    case 0x2284: return "nsub";    // (8836) not a subset of
    case 0x2286: return "sube";    // (8838) subset of or equal to
    case 0x2287: return "supe";    // (8839) superset of or equal to
    case 0x2295: return "oplus";   // (8853) circled plus = direct sum
    case 0x2297: return "otimes";  // (8855) circled times = vector product
    case 0x22A5: return "perp";    // (8869) up tack = orthogonal to = perpendicular
    case 0x22C5: return "sdot";    // (8901) dot operator
    // Miscellaneous Technical -------------
    case 0x2308: return "lceil";   // (8968) left ceiling = apl upstile
    case 0x2309: return "rceil";   // (8969) right ceiling
    case 0x230A: return "lfloor";  // (8970) left floor = apl downstile
    case 0x230B: return "rfloor";  // (8971) right floor
    case 0x2329: return "lang";    // (9001) left-pointing angle bracket = bra
    case 0x232A: return "rang";    // (9002) right-pointing angle bracket = ket
    // Geometric Shapes --------------------
    case 0x25CA: return "loz";     // (9674) lozenge
    // Miscellaneous Symbols ---------------
    case 0x2660: return "spades";  // (9824) black spade suit
    case 0x2663: return "clubs";   // (9827) black club suit = shamrock
    case 0x2665: return "hearts";  // (9829) black heart suit = valentine
    case 0x2666: return "diams";   // (9830) black diamond suit
    default: break;
    }
    return 0;
}

QString CGI::encodeHTML(const QString &rawText, int conversionFlags)
{
    QString enc;
    enc.reserve(rawText.length()); // at least

    QString::const_iterator it = rawText.constBegin();
    while (it != rawText.constEnd()) {
        const char *html = unicodeToHTML((*it).unicode());
        if (html) {
            enc.append(QLatin1Char('&'));
            enc.append(QLatin1String(html));
            enc.append(QLatin1Char(';'));
        } else if ((conversionFlags & CGI::LineBreaks)
                   && ((*it).toLatin1() == '\n')) {
                enc.append(QLatin1String("<BR>\n"));
        } else if ((conversionFlags & CGI::Spaces)
                   && ((*it).toLatin1() == ' ')) {
                enc.append(QLatin1String("&nbsp;"));
        } else if ((conversionFlags & CGI::Tabs)
                   && ((*it).toLatin1() == '\t')) {
                enc.append(QLatin1String("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"));
        } else if ((*it).unicode() > 0x00FF) {
            enc.append(QLatin1String("&#"));
            enc.append(QString::number((*it).unicode()));
            enc.append(QLatin1Char(';'));
        } else {
            enc.append(*it);
        }
        ++it;
    }

    return enc;
}

