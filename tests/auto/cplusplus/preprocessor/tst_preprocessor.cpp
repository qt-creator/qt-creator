#include <QtTest>
#include <pp.h>

using namespace CPlusPlus;

class tst_Preprocessor: public QObject
{
Q_OBJECT

private Q_SLOTS:
    void unfinished_function_like_macro_call();
    void nasty_macro_expansion();
};

void tst_Preprocessor::unfinished_function_like_macro_call()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(a,b) a + b"
                                         "\nfoo(1, 2\n"));

    QCOMPARE(preprocessed.trimmed(), QByteArray("foo"));
}

void tst_Preprocessor::nasty_macro_expansion()
{
    QByteArray input("\n"
                     "#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))\n"
                     "#define is_power_of_two(x)      ( !((x) & ((x)-1)) )\n"
                     "#define low_bit_mask(x)         ( ((x)-1) & ~(x) )\n"
                     "#define is_valid_mask(x)        is_power_of_two(1LU + (x) + low_bit_mask(x))\n"
                     "#define compile_ffs2(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0x1), 0, 1)\n"
                     "#define compile_ffs4(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0x3), \\\n"
                     "                              (compile_ffs2((__x))), \\\n"
                     "                              (compile_ffs2((__x) >> 2) + 2))\n"
                     "#define compile_ffs8(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0xf), \\\n"
                     "                              (compile_ffs4((__x))), \\\n"
                     "                              (compile_ffs4((__x) >> 4) + 4))\n"
                     "#define compile_ffs16(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0xff), \\\n"
                     "                              (compile_ffs8((__x))), \\\n"
                     "                              (compile_ffs8((__x) >> 8) + 8))\n"
                     "#define compile_ffs32(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0xffff), \\\n"
                     "                              (compile_ffs16((__x))), \\\n"
                     "                              (compile_ffs16((__x) >> 16) + 16))\n"
                     "#define FIELD_CHECK(__mask, __type)                     \\\n"
                     "        BUILD_BUG_ON(!(__mask) ||                       \\\n"
                     "                     !is_valid_mask(__mask) ||          \\\n"
                     "                     (__mask) != (__type)(__mask))      \\\n"
                     "\n"
                     "#define FIELD32(__mask)                         \\\n"
                     "({                                              \\\n"
                     "        FIELD_CHECK(__mask, u32);               \\\n"
                     "        (struct rt2x00_field32) {               \\\n"
                     "                compile_ffs32(__mask), (__mask) \\\n"
                     "        };                                      \\\n"
                     "})\n"
                     "#define BBPCSR                          0x00f0\n"
                     "#define BBPCSR_BUSY                     FIELD32(0x00008000)\n"
                     "#define WAIT_FOR_BBP(__dev, __reg)  \\\n"
                     "        rt2x00pci_regbusy_read((__dev), BBPCSR, BBPCSR_BUSY, (__reg))\n"
                     "if (WAIT_FOR_BBP(rt2x00dev, &reg)) {}\n"
                     );

    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"), input);

    QVERIFY(!preprocessed.contains("FIELD32"));
}

QTEST_APPLESS_MAIN(tst_Preprocessor)
#include "tst_preprocessor.moc"
