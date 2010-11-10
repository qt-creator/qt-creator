#ifndef GLSLLEXER_H
#define GLSLLEXER_H

namespace GLSL {

class Token
{
public:
    int kind;
    int position;
    int length;
    int line; // ### remove

    union {
        int matchingBrace;
        void *ptr;
    };

    Token()
        : kind(0), position(0), length(0), line(0), ptr(0) {}

    bool is(int k) const { return k == kind; }
    bool isNot(int k) const { return k != kind; }
};

class Lexer
{
public:
    Lexer(const char *source, unsigned size);
    ~Lexer();

    enum
    {
        // Extra flag bits added to tokens by Lexer::classify() that
        // indicate which variant of GLSL the keyword belongs to.
        Variant_GLSL_120            = 0x00010000,   // 1.20 and higher
        Variant_GLSL_150            = 0x00020000,   // 1.50 and higher
        Variant_GLSL_400            = 0x00040000,   // 4.00 and higher
        Variant_GLSL_ES_100         = 0x00080000,   // ES 1.00 and higher
        Variant_GLSL_Qt             = 0x00100000,
        Variant_VertexShader        = 0x00200000,
        Variant_FragmentShader      = 0x00400000,
        Variant_Reserved            = 0x80000000,
        Variant_Mask                = 0xFFFF0000
    };

    int variant() const { return _variant; }
    void setVariant(int flags) { _variant = flags; }

    int yylex(Token *tk);

private:
    static int classify(const char *s, int len);

    void yyinp();
    int yylex_helper(const char **position, int *line);

private:
    const char *_source;
    const char *_it;
    const char *_end;
    int _size;
    int _yychar;
    int _lineno;
    int _variant;
};

} // end of namespace GLSL

#endif // GLSLLEXER_H
