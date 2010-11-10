
#include "glslast.h"

namespace GLSL {

class Delete
{
    typedef void (Delete::*dispatch_func)(AST *);
    static dispatch_func dispatch[];

public:
    void accept(AST *ast)
    {
        if (! ast)
            return;
        else if (Operator *op = ast->asOperator())
            (this->*dispatch[op->ruleno])(ast);
        delete ast;
    }

    template <typename It>
    void accept(It first, It last)
    {
        for (; first != last; ++first)
            accept(*first);
    }

private:
    void on_variable_identifier_1(AST *);
    void on_primary_expression_2(AST *);
    void on_primary_expression_3(AST *);
    void on_primary_expression_4(AST *);
    void on_primary_expression_5(AST *);
    void on_primary_expression_6(AST *);
    void on_postfix_expression_7(AST *);
    void on_postfix_expression_8(AST *);
    void on_postfix_expression_9(AST *);
    void on_postfix_expression_10(AST *);
    void on_postfix_expression_11(AST *);
    void on_postfix_expression_12(AST *);
    void on_integer_expression_13(AST *);
    void on_function_call_14(AST *);
    void on_function_call_or_method_15(AST *);
    void on_function_call_or_method_16(AST *);
    void on_function_call_generic_17(AST *);
    void on_function_call_generic_18(AST *);
    void on_function_call_header_no_parameters_19(AST *);
    void on_function_call_header_no_parameters_20(AST *);
    void on_function_call_header_with_parameters_21(AST *);
    void on_function_call_header_with_parameters_22(AST *);
    void on_function_call_header_23(AST *);
    void on_function_identifier_24(AST *);
    void on_function_identifier_25(AST *);
    void on_unary_expression_26(AST *);
    void on_unary_expression_27(AST *);
    void on_unary_expression_28(AST *);
    void on_unary_expression_29(AST *);
    void on_unary_operator_30(AST *);
    void on_unary_operator_31(AST *);
    void on_unary_operator_32(AST *);
    void on_unary_operator_33(AST *);
    void on_multiplicative_expression_34(AST *);
    void on_multiplicative_expression_35(AST *);
    void on_multiplicative_expression_36(AST *);
    void on_multiplicative_expression_37(AST *);
    void on_additive_expression_38(AST *);
    void on_additive_expression_39(AST *);
    void on_additive_expression_40(AST *);
    void on_shift_expression_41(AST *);
    void on_shift_expression_42(AST *);
    void on_shift_expression_43(AST *);
    void on_relational_expression_44(AST *);
    void on_relational_expression_45(AST *);
    void on_relational_expression_46(AST *);
    void on_relational_expression_47(AST *);
    void on_relational_expression_48(AST *);
    void on_equality_expression_49(AST *);
    void on_equality_expression_50(AST *);
    void on_equality_expression_51(AST *);
    void on_and_expression_52(AST *);
    void on_and_expression_53(AST *);
    void on_exclusive_or_expression_54(AST *);
    void on_exclusive_or_expression_55(AST *);
    void on_inclusive_or_expression_56(AST *);
    void on_inclusive_or_expression_57(AST *);
    void on_logical_and_expression_58(AST *);
    void on_logical_and_expression_59(AST *);
    void on_logical_xor_expression_60(AST *);
    void on_logical_xor_expression_61(AST *);
    void on_logical_or_expression_62(AST *);
    void on_logical_or_expression_63(AST *);
    void on_conditional_expression_64(AST *);
    void on_conditional_expression_65(AST *);
    void on_assignment_expression_66(AST *);
    void on_assignment_expression_67(AST *);
    void on_assignment_operator_68(AST *);
    void on_assignment_operator_69(AST *);
    void on_assignment_operator_70(AST *);
    void on_assignment_operator_71(AST *);
    void on_assignment_operator_72(AST *);
    void on_assignment_operator_73(AST *);
    void on_assignment_operator_74(AST *);
    void on_assignment_operator_75(AST *);
    void on_assignment_operator_76(AST *);
    void on_assignment_operator_77(AST *);
    void on_assignment_operator_78(AST *);
    void on_expression_79(AST *);
    void on_expression_80(AST *);
    void on_constant_expression_81(AST *);
    void on_declaration_82(AST *);
    void on_declaration_83(AST *);
    void on_declaration_84(AST *);
    void on_declaration_85(AST *);
    void on_declaration_86(AST *);
    void on_declaration_87(AST *);
    void on_declaration_88(AST *);
    void on_declaration_89(AST *);
    void on_function_prototype_90(AST *);
    void on_function_declarator_91(AST *);
    void on_function_declarator_92(AST *);
    void on_function_header_with_parameters_93(AST *);
    void on_function_header_with_parameters_94(AST *);
    void on_function_header_95(AST *);
    void on_parameter_declarator_96(AST *);
    void on_parameter_declarator_97(AST *);
    void on_parameter_declaration_98(AST *);
    void on_parameter_declaration_99(AST *);
    void on_parameter_declaration_100(AST *);
    void on_parameter_declaration_101(AST *);
    void on_parameter_qualifier_102(AST *);
    void on_parameter_qualifier_103(AST *);
    void on_parameter_qualifier_104(AST *);
    void on_parameter_qualifier_105(AST *);
    void on_parameter_type_specifier_106(AST *);
    void on_init_declarator_list_107(AST *);
    void on_init_declarator_list_108(AST *);
    void on_init_declarator_list_109(AST *);
    void on_init_declarator_list_110(AST *);
    void on_init_declarator_list_111(AST *);
    void on_init_declarator_list_112(AST *);
    void on_init_declarator_list_113(AST *);
    void on_single_declaration_114(AST *);
    void on_single_declaration_115(AST *);
    void on_single_declaration_116(AST *);
    void on_single_declaration_117(AST *);
    void on_single_declaration_118(AST *);
    void on_single_declaration_119(AST *);
    void on_single_declaration_120(AST *);
    void on_single_declaration_121(AST *);
    void on_fully_specified_type_122(AST *);
    void on_fully_specified_type_123(AST *);
    void on_invariant_qualifier_124(AST *);
    void on_interpolation_qualifier_125(AST *);
    void on_interpolation_qualifier_126(AST *);
    void on_interpolation_qualifier_127(AST *);
    void on_layout_qualifier_128(AST *);
    void on_layout_qualifier_id_list_129(AST *);
    void on_layout_qualifier_id_list_130(AST *);
    void on_layout_qualifier_id_131(AST *);
    void on_layout_qualifier_id_132(AST *);
    void on_parameter_type_qualifier_133(AST *);
    void on_type_qualifier_134(AST *);
    void on_type_qualifier_135(AST *);
    void on_type_qualifier_136(AST *);
    void on_type_qualifier_137(AST *);
    void on_type_qualifier_138(AST *);
    void on_type_qualifier_139(AST *);
    void on_type_qualifier_140(AST *);
    void on_type_qualifier_141(AST *);
    void on_storage_qualifier_142(AST *);
    void on_storage_qualifier_143(AST *);
    void on_storage_qualifier_144(AST *);
    void on_storage_qualifier_145(AST *);
    void on_storage_qualifier_146(AST *);
    void on_storage_qualifier_147(AST *);
    void on_storage_qualifier_148(AST *);
    void on_storage_qualifier_149(AST *);
    void on_storage_qualifier_150(AST *);
    void on_storage_qualifier_151(AST *);
    void on_storage_qualifier_152(AST *);
    void on_storage_qualifier_153(AST *);
    void on_storage_qualifier_154(AST *);
    void on_type_specifier_155(AST *);
    void on_type_specifier_156(AST *);
    void on_type_specifier_no_prec_157(AST *);
    void on_type_specifier_no_prec_158(AST *);
    void on_type_specifier_no_prec_159(AST *);
    void on_type_specifier_nonarray_160(AST *);
    void on_type_specifier_nonarray_161(AST *);
    void on_type_specifier_nonarray_162(AST *);
    void on_type_specifier_nonarray_163(AST *);
    void on_type_specifier_nonarray_164(AST *);
    void on_type_specifier_nonarray_165(AST *);
    void on_type_specifier_nonarray_166(AST *);
    void on_type_specifier_nonarray_167(AST *);
    void on_type_specifier_nonarray_168(AST *);
    void on_type_specifier_nonarray_169(AST *);
    void on_type_specifier_nonarray_170(AST *);
    void on_type_specifier_nonarray_171(AST *);
    void on_type_specifier_nonarray_172(AST *);
    void on_type_specifier_nonarray_173(AST *);
    void on_type_specifier_nonarray_174(AST *);
    void on_type_specifier_nonarray_175(AST *);
    void on_type_specifier_nonarray_176(AST *);
    void on_type_specifier_nonarray_177(AST *);
    void on_type_specifier_nonarray_178(AST *);
    void on_type_specifier_nonarray_179(AST *);
    void on_type_specifier_nonarray_180(AST *);
    void on_type_specifier_nonarray_181(AST *);
    void on_type_specifier_nonarray_182(AST *);
    void on_type_specifier_nonarray_183(AST *);
    void on_type_specifier_nonarray_184(AST *);
    void on_type_specifier_nonarray_185(AST *);
    void on_type_specifier_nonarray_186(AST *);
    void on_type_specifier_nonarray_187(AST *);
    void on_type_specifier_nonarray_188(AST *);
    void on_type_specifier_nonarray_189(AST *);
    void on_type_specifier_nonarray_190(AST *);
    void on_type_specifier_nonarray_191(AST *);
    void on_type_specifier_nonarray_192(AST *);
    void on_type_specifier_nonarray_193(AST *);
    void on_type_specifier_nonarray_194(AST *);
    void on_type_specifier_nonarray_195(AST *);
    void on_type_specifier_nonarray_196(AST *);
    void on_type_specifier_nonarray_197(AST *);
    void on_type_specifier_nonarray_198(AST *);
    void on_type_specifier_nonarray_199(AST *);
    void on_type_specifier_nonarray_200(AST *);
    void on_type_specifier_nonarray_201(AST *);
    void on_type_specifier_nonarray_202(AST *);
    void on_type_specifier_nonarray_203(AST *);
    void on_type_specifier_nonarray_204(AST *);
    void on_type_specifier_nonarray_205(AST *);
    void on_type_specifier_nonarray_206(AST *);
    void on_type_specifier_nonarray_207(AST *);
    void on_type_specifier_nonarray_208(AST *);
    void on_type_specifier_nonarray_209(AST *);
    void on_type_specifier_nonarray_210(AST *);
    void on_type_specifier_nonarray_211(AST *);
    void on_type_specifier_nonarray_212(AST *);
    void on_type_specifier_nonarray_213(AST *);
    void on_type_specifier_nonarray_214(AST *);
    void on_type_specifier_nonarray_215(AST *);
    void on_type_specifier_nonarray_216(AST *);
    void on_type_specifier_nonarray_217(AST *);
    void on_type_specifier_nonarray_218(AST *);
    void on_type_specifier_nonarray_219(AST *);
    void on_type_specifier_nonarray_220(AST *);
    void on_type_specifier_nonarray_221(AST *);
    void on_type_specifier_nonarray_222(AST *);
    void on_type_specifier_nonarray_223(AST *);
    void on_type_specifier_nonarray_224(AST *);
    void on_type_specifier_nonarray_225(AST *);
    void on_type_specifier_nonarray_226(AST *);
    void on_type_specifier_nonarray_227(AST *);
    void on_type_specifier_nonarray_228(AST *);
    void on_type_specifier_nonarray_229(AST *);
    void on_type_specifier_nonarray_230(AST *);
    void on_type_specifier_nonarray_231(AST *);
    void on_type_specifier_nonarray_232(AST *);
    void on_type_specifier_nonarray_233(AST *);
    void on_type_specifier_nonarray_234(AST *);
    void on_type_specifier_nonarray_235(AST *);
    void on_type_specifier_nonarray_236(AST *);
    void on_type_specifier_nonarray_237(AST *);
    void on_type_specifier_nonarray_238(AST *);
    void on_type_specifier_nonarray_239(AST *);
    void on_type_specifier_nonarray_240(AST *);
    void on_type_specifier_nonarray_241(AST *);
    void on_type_specifier_nonarray_242(AST *);
    void on_type_specifier_nonarray_243(AST *);
    void on_type_specifier_nonarray_244(AST *);
    void on_type_specifier_nonarray_245(AST *);
    void on_type_specifier_nonarray_246(AST *);
    void on_precision_qualifier_247(AST *);
    void on_precision_qualifier_248(AST *);
    void on_precision_qualifier_249(AST *);
    void on_struct_specifier_250(AST *);
    void on_struct_specifier_251(AST *);
    void on_struct_declaration_list_252(AST *);
    void on_struct_declaration_list_253(AST *);
    void on_struct_declaration_254(AST *);
    void on_struct_declaration_255(AST *);
    void on_struct_declarator_list_256(AST *);
    void on_struct_declarator_list_257(AST *);
    void on_struct_declarator_258(AST *);
    void on_struct_declarator_259(AST *);
    void on_struct_declarator_260(AST *);
    void on_initializer_261(AST *);
    void on_declaration_statement_262(AST *);
    void on_statement_263(AST *);
    void on_statement_264(AST *);
    void on_simple_statement_265(AST *);
    void on_simple_statement_266(AST *);
    void on_simple_statement_267(AST *);
    void on_simple_statement_268(AST *);
    void on_simple_statement_269(AST *);
    void on_simple_statement_270(AST *);
    void on_simple_statement_271(AST *);
    void on_compound_statement_272(AST *);
    void on_compound_statement_273(AST *);
    void on_statement_no_new_scope_274(AST *);
    void on_statement_no_new_scope_275(AST *);
    void on_compound_statement_no_new_scope_276(AST *);
    void on_compound_statement_no_new_scope_277(AST *);
    void on_statement_list_278(AST *);
    void on_statement_list_279(AST *);
    void on_expression_statement_280(AST *);
    void on_expression_statement_281(AST *);
    void on_selection_statement_282(AST *);
    void on_selection_rest_statement_283(AST *);
    void on_selection_rest_statement_284(AST *);
    void on_condition_285(AST *);
    void on_condition_286(AST *);
    void on_switch_statement_287(AST *);
    void on_switch_statement_list_288(AST *);
    void on_switch_statement_list_289(AST *);
    void on_case_label_290(AST *);
    void on_case_label_291(AST *);
    void on_iteration_statement_292(AST *);
    void on_iteration_statement_293(AST *);
    void on_iteration_statement_294(AST *);
    void on_for_init_statement_295(AST *);
    void on_for_init_statement_296(AST *);
    void on_conditionopt_297(AST *);
    void on_conditionopt_298(AST *);
    void on_for_rest_statement_299(AST *);
    void on_for_rest_statement_300(AST *);
    void on_jump_statement_301(AST *);
    void on_jump_statement_302(AST *);
    void on_jump_statement_303(AST *);
    void on_jump_statement_304(AST *);
    void on_jump_statement_305(AST *);
    void on_translation_unit_306(AST *);
    void on_translation_unit_307(AST *);
    void on_external_declaration_308(AST *);
    void on_external_declaration_309(AST *);
    void on_external_declaration_310(AST *);
    void on_function_definition_311(AST *);
};

void deleteAST(AST *ast)
{
    Delete del;
    del.accept(ast);
}

} // end of namespace GLSL

#include <iostream>

using namespace GLSL;

namespace {
bool debug = false;
}


Delete::dispatch_func Delete::dispatch[] = {
    &Delete::on_variable_identifier_1,
    &Delete::on_primary_expression_2,
    &Delete::on_primary_expression_3,
    &Delete::on_primary_expression_4,
    &Delete::on_primary_expression_5,
    &Delete::on_primary_expression_6,
    &Delete::on_postfix_expression_7,
    &Delete::on_postfix_expression_8,
    &Delete::on_postfix_expression_9,
    &Delete::on_postfix_expression_10,
    &Delete::on_postfix_expression_11,
    &Delete::on_postfix_expression_12,
    &Delete::on_integer_expression_13,
    &Delete::on_function_call_14,
    &Delete::on_function_call_or_method_15,
    &Delete::on_function_call_or_method_16,
    &Delete::on_function_call_generic_17,
    &Delete::on_function_call_generic_18,
    &Delete::on_function_call_header_no_parameters_19,
    &Delete::on_function_call_header_no_parameters_20,
    &Delete::on_function_call_header_with_parameters_21,
    &Delete::on_function_call_header_with_parameters_22,
    &Delete::on_function_call_header_23,
    &Delete::on_function_identifier_24,
    &Delete::on_function_identifier_25,
    &Delete::on_unary_expression_26,
    &Delete::on_unary_expression_27,
    &Delete::on_unary_expression_28,
    &Delete::on_unary_expression_29,
    &Delete::on_unary_operator_30,
    &Delete::on_unary_operator_31,
    &Delete::on_unary_operator_32,
    &Delete::on_unary_operator_33,
    &Delete::on_multiplicative_expression_34,
    &Delete::on_multiplicative_expression_35,
    &Delete::on_multiplicative_expression_36,
    &Delete::on_multiplicative_expression_37,
    &Delete::on_additive_expression_38,
    &Delete::on_additive_expression_39,
    &Delete::on_additive_expression_40,
    &Delete::on_shift_expression_41,
    &Delete::on_shift_expression_42,
    &Delete::on_shift_expression_43,
    &Delete::on_relational_expression_44,
    &Delete::on_relational_expression_45,
    &Delete::on_relational_expression_46,
    &Delete::on_relational_expression_47,
    &Delete::on_relational_expression_48,
    &Delete::on_equality_expression_49,
    &Delete::on_equality_expression_50,
    &Delete::on_equality_expression_51,
    &Delete::on_and_expression_52,
    &Delete::on_and_expression_53,
    &Delete::on_exclusive_or_expression_54,
    &Delete::on_exclusive_or_expression_55,
    &Delete::on_inclusive_or_expression_56,
    &Delete::on_inclusive_or_expression_57,
    &Delete::on_logical_and_expression_58,
    &Delete::on_logical_and_expression_59,
    &Delete::on_logical_xor_expression_60,
    &Delete::on_logical_xor_expression_61,
    &Delete::on_logical_or_expression_62,
    &Delete::on_logical_or_expression_63,
    &Delete::on_conditional_expression_64,
    &Delete::on_conditional_expression_65,
    &Delete::on_assignment_expression_66,
    &Delete::on_assignment_expression_67,
    &Delete::on_assignment_operator_68,
    &Delete::on_assignment_operator_69,
    &Delete::on_assignment_operator_70,
    &Delete::on_assignment_operator_71,
    &Delete::on_assignment_operator_72,
    &Delete::on_assignment_operator_73,
    &Delete::on_assignment_operator_74,
    &Delete::on_assignment_operator_75,
    &Delete::on_assignment_operator_76,
    &Delete::on_assignment_operator_77,
    &Delete::on_assignment_operator_78,
    &Delete::on_expression_79,
    &Delete::on_expression_80,
    &Delete::on_constant_expression_81,
    &Delete::on_declaration_82,
    &Delete::on_declaration_83,
    &Delete::on_declaration_84,
    &Delete::on_declaration_85,
    &Delete::on_declaration_86,
    &Delete::on_declaration_87,
    &Delete::on_declaration_88,
    &Delete::on_declaration_89,
    &Delete::on_function_prototype_90,
    &Delete::on_function_declarator_91,
    &Delete::on_function_declarator_92,
    &Delete::on_function_header_with_parameters_93,
    &Delete::on_function_header_with_parameters_94,
    &Delete::on_function_header_95,
    &Delete::on_parameter_declarator_96,
    &Delete::on_parameter_declarator_97,
    &Delete::on_parameter_declaration_98,
    &Delete::on_parameter_declaration_99,
    &Delete::on_parameter_declaration_100,
    &Delete::on_parameter_declaration_101,
    &Delete::on_parameter_qualifier_102,
    &Delete::on_parameter_qualifier_103,
    &Delete::on_parameter_qualifier_104,
    &Delete::on_parameter_qualifier_105,
    &Delete::on_parameter_type_specifier_106,
    &Delete::on_init_declarator_list_107,
    &Delete::on_init_declarator_list_108,
    &Delete::on_init_declarator_list_109,
    &Delete::on_init_declarator_list_110,
    &Delete::on_init_declarator_list_111,
    &Delete::on_init_declarator_list_112,
    &Delete::on_init_declarator_list_113,
    &Delete::on_single_declaration_114,
    &Delete::on_single_declaration_115,
    &Delete::on_single_declaration_116,
    &Delete::on_single_declaration_117,
    &Delete::on_single_declaration_118,
    &Delete::on_single_declaration_119,
    &Delete::on_single_declaration_120,
    &Delete::on_single_declaration_121,
    &Delete::on_fully_specified_type_122,
    &Delete::on_fully_specified_type_123,
    &Delete::on_invariant_qualifier_124,
    &Delete::on_interpolation_qualifier_125,
    &Delete::on_interpolation_qualifier_126,
    &Delete::on_interpolation_qualifier_127,
    &Delete::on_layout_qualifier_128,
    &Delete::on_layout_qualifier_id_list_129,
    &Delete::on_layout_qualifier_id_list_130,
    &Delete::on_layout_qualifier_id_131,
    &Delete::on_layout_qualifier_id_132,
    &Delete::on_parameter_type_qualifier_133,
    &Delete::on_type_qualifier_134,
    &Delete::on_type_qualifier_135,
    &Delete::on_type_qualifier_136,
    &Delete::on_type_qualifier_137,
    &Delete::on_type_qualifier_138,
    &Delete::on_type_qualifier_139,
    &Delete::on_type_qualifier_140,
    &Delete::on_type_qualifier_141,
    &Delete::on_storage_qualifier_142,
    &Delete::on_storage_qualifier_143,
    &Delete::on_storage_qualifier_144,
    &Delete::on_storage_qualifier_145,
    &Delete::on_storage_qualifier_146,
    &Delete::on_storage_qualifier_147,
    &Delete::on_storage_qualifier_148,
    &Delete::on_storage_qualifier_149,
    &Delete::on_storage_qualifier_150,
    &Delete::on_storage_qualifier_151,
    &Delete::on_storage_qualifier_152,
    &Delete::on_storage_qualifier_153,
    &Delete::on_storage_qualifier_154,
    &Delete::on_type_specifier_155,
    &Delete::on_type_specifier_156,
    &Delete::on_type_specifier_no_prec_157,
    &Delete::on_type_specifier_no_prec_158,
    &Delete::on_type_specifier_no_prec_159,
    &Delete::on_type_specifier_nonarray_160,
    &Delete::on_type_specifier_nonarray_161,
    &Delete::on_type_specifier_nonarray_162,
    &Delete::on_type_specifier_nonarray_163,
    &Delete::on_type_specifier_nonarray_164,
    &Delete::on_type_specifier_nonarray_165,
    &Delete::on_type_specifier_nonarray_166,
    &Delete::on_type_specifier_nonarray_167,
    &Delete::on_type_specifier_nonarray_168,
    &Delete::on_type_specifier_nonarray_169,
    &Delete::on_type_specifier_nonarray_170,
    &Delete::on_type_specifier_nonarray_171,
    &Delete::on_type_specifier_nonarray_172,
    &Delete::on_type_specifier_nonarray_173,
    &Delete::on_type_specifier_nonarray_174,
    &Delete::on_type_specifier_nonarray_175,
    &Delete::on_type_specifier_nonarray_176,
    &Delete::on_type_specifier_nonarray_177,
    &Delete::on_type_specifier_nonarray_178,
    &Delete::on_type_specifier_nonarray_179,
    &Delete::on_type_specifier_nonarray_180,
    &Delete::on_type_specifier_nonarray_181,
    &Delete::on_type_specifier_nonarray_182,
    &Delete::on_type_specifier_nonarray_183,
    &Delete::on_type_specifier_nonarray_184,
    &Delete::on_type_specifier_nonarray_185,
    &Delete::on_type_specifier_nonarray_186,
    &Delete::on_type_specifier_nonarray_187,
    &Delete::on_type_specifier_nonarray_188,
    &Delete::on_type_specifier_nonarray_189,
    &Delete::on_type_specifier_nonarray_190,
    &Delete::on_type_specifier_nonarray_191,
    &Delete::on_type_specifier_nonarray_192,
    &Delete::on_type_specifier_nonarray_193,
    &Delete::on_type_specifier_nonarray_194,
    &Delete::on_type_specifier_nonarray_195,
    &Delete::on_type_specifier_nonarray_196,
    &Delete::on_type_specifier_nonarray_197,
    &Delete::on_type_specifier_nonarray_198,
    &Delete::on_type_specifier_nonarray_199,
    &Delete::on_type_specifier_nonarray_200,
    &Delete::on_type_specifier_nonarray_201,
    &Delete::on_type_specifier_nonarray_202,
    &Delete::on_type_specifier_nonarray_203,
    &Delete::on_type_specifier_nonarray_204,
    &Delete::on_type_specifier_nonarray_205,
    &Delete::on_type_specifier_nonarray_206,
    &Delete::on_type_specifier_nonarray_207,
    &Delete::on_type_specifier_nonarray_208,
    &Delete::on_type_specifier_nonarray_209,
    &Delete::on_type_specifier_nonarray_210,
    &Delete::on_type_specifier_nonarray_211,
    &Delete::on_type_specifier_nonarray_212,
    &Delete::on_type_specifier_nonarray_213,
    &Delete::on_type_specifier_nonarray_214,
    &Delete::on_type_specifier_nonarray_215,
    &Delete::on_type_specifier_nonarray_216,
    &Delete::on_type_specifier_nonarray_217,
    &Delete::on_type_specifier_nonarray_218,
    &Delete::on_type_specifier_nonarray_219,
    &Delete::on_type_specifier_nonarray_220,
    &Delete::on_type_specifier_nonarray_221,
    &Delete::on_type_specifier_nonarray_222,
    &Delete::on_type_specifier_nonarray_223,
    &Delete::on_type_specifier_nonarray_224,
    &Delete::on_type_specifier_nonarray_225,
    &Delete::on_type_specifier_nonarray_226,
    &Delete::on_type_specifier_nonarray_227,
    &Delete::on_type_specifier_nonarray_228,
    &Delete::on_type_specifier_nonarray_229,
    &Delete::on_type_specifier_nonarray_230,
    &Delete::on_type_specifier_nonarray_231,
    &Delete::on_type_specifier_nonarray_232,
    &Delete::on_type_specifier_nonarray_233,
    &Delete::on_type_specifier_nonarray_234,
    &Delete::on_type_specifier_nonarray_235,
    &Delete::on_type_specifier_nonarray_236,
    &Delete::on_type_specifier_nonarray_237,
    &Delete::on_type_specifier_nonarray_238,
    &Delete::on_type_specifier_nonarray_239,
    &Delete::on_type_specifier_nonarray_240,
    &Delete::on_type_specifier_nonarray_241,
    &Delete::on_type_specifier_nonarray_242,
    &Delete::on_type_specifier_nonarray_243,
    &Delete::on_type_specifier_nonarray_244,
    &Delete::on_type_specifier_nonarray_245,
    &Delete::on_type_specifier_nonarray_246,
    &Delete::on_precision_qualifier_247,
    &Delete::on_precision_qualifier_248,
    &Delete::on_precision_qualifier_249,
    &Delete::on_struct_specifier_250,
    &Delete::on_struct_specifier_251,
    &Delete::on_struct_declaration_list_252,
    &Delete::on_struct_declaration_list_253,
    &Delete::on_struct_declaration_254,
    &Delete::on_struct_declaration_255,
    &Delete::on_struct_declarator_list_256,
    &Delete::on_struct_declarator_list_257,
    &Delete::on_struct_declarator_258,
    &Delete::on_struct_declarator_259,
    &Delete::on_struct_declarator_260,
    &Delete::on_initializer_261,
    &Delete::on_declaration_statement_262,
    &Delete::on_statement_263,
    &Delete::on_statement_264,
    &Delete::on_simple_statement_265,
    &Delete::on_simple_statement_266,
    &Delete::on_simple_statement_267,
    &Delete::on_simple_statement_268,
    &Delete::on_simple_statement_269,
    &Delete::on_simple_statement_270,
    &Delete::on_simple_statement_271,
    &Delete::on_compound_statement_272,
    &Delete::on_compound_statement_273,
    &Delete::on_statement_no_new_scope_274,
    &Delete::on_statement_no_new_scope_275,
    &Delete::on_compound_statement_no_new_scope_276,
    &Delete::on_compound_statement_no_new_scope_277,
    &Delete::on_statement_list_278,
    &Delete::on_statement_list_279,
    &Delete::on_expression_statement_280,
    &Delete::on_expression_statement_281,
    &Delete::on_selection_statement_282,
    &Delete::on_selection_rest_statement_283,
    &Delete::on_selection_rest_statement_284,
    &Delete::on_condition_285,
    &Delete::on_condition_286,
    &Delete::on_switch_statement_287,
    &Delete::on_switch_statement_list_288,
    &Delete::on_switch_statement_list_289,
    &Delete::on_case_label_290,
    &Delete::on_case_label_291,
    &Delete::on_iteration_statement_292,
    &Delete::on_iteration_statement_293,
    &Delete::on_iteration_statement_294,
    &Delete::on_for_init_statement_295,
    &Delete::on_for_init_statement_296,
    &Delete::on_conditionopt_297,
    &Delete::on_conditionopt_298,
    &Delete::on_for_rest_statement_299,
    &Delete::on_for_rest_statement_300,
    &Delete::on_jump_statement_301,
    &Delete::on_jump_statement_302,
    &Delete::on_jump_statement_303,
    &Delete::on_jump_statement_304,
    &Delete::on_jump_statement_305,
    &Delete::on_translation_unit_306,
    &Delete::on_translation_unit_307,
    &Delete::on_external_declaration_308,
    &Delete::on_external_declaration_309,
    &Delete::on_external_declaration_310,
    &Delete::on_function_definition_311,
0, };

// variable_identifier ::= IDENTIFIER ;
void Delete::on_variable_identifier_1(AST *ast)
{
    if (debug)
        std::cout << "variable_identifier ::= IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// primary_expression ::= NUMBER ;
void Delete::on_primary_expression_2(AST *ast)
{
    if (debug)
        std::cout << "primary_expression ::= NUMBER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// primary_expression ::= TRUE ;
void Delete::on_primary_expression_3(AST *ast)
{
    if (debug)
        std::cout << "primary_expression ::= TRUE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// primary_expression ::= FALSE ;
void Delete::on_primary_expression_4(AST *ast)
{
    if (debug)
        std::cout << "primary_expression ::= FALSE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// primary_expression ::= variable_identifier ;
void Delete::on_primary_expression_5(AST *ast)
{
    if (debug)
        std::cout << "primary_expression ::= variable_identifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// primary_expression ::= LEFT_PAREN expression RIGHT_PAREN ;
void Delete::on_primary_expression_6(AST *ast)
{
    if (debug)
        std::cout << "primary_expression ::= LEFT_PAREN expression RIGHT_PAREN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// postfix_expression ::= primary_expression ;
void Delete::on_postfix_expression_7(AST *ast)
{
    if (debug)
        std::cout << "postfix_expression ::= primary_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// postfix_expression ::= postfix_expression LEFT_BRACKET integer_expression RIGHT_BRACKET ;
void Delete::on_postfix_expression_8(AST *ast)
{
    if (debug)
        std::cout << "postfix_expression ::= postfix_expression LEFT_BRACKET integer_expression RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// postfix_expression ::= function_call ;
void Delete::on_postfix_expression_9(AST *ast)
{
    if (debug)
        std::cout << "postfix_expression ::= function_call" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// postfix_expression ::= postfix_expression DOT IDENTIFIER ;
void Delete::on_postfix_expression_10(AST *ast)
{
    if (debug)
        std::cout << "postfix_expression ::= postfix_expression DOT IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// postfix_expression ::= postfix_expression INC_OP ;
void Delete::on_postfix_expression_11(AST *ast)
{
    if (debug)
        std::cout << "postfix_expression ::= postfix_expression INC_OP" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// postfix_expression ::= postfix_expression DEC_OP ;
void Delete::on_postfix_expression_12(AST *ast)
{
    if (debug)
        std::cout << "postfix_expression ::= postfix_expression DEC_OP" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// integer_expression ::= expression ;
void Delete::on_integer_expression_13(AST *ast)
{
    if (debug)
        std::cout << "integer_expression ::= expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call ::= function_call_or_method ;
void Delete::on_function_call_14(AST *ast)
{
    if (debug)
        std::cout << "function_call ::= function_call_or_method" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_or_method ::= function_call_generic ;
void Delete::on_function_call_or_method_15(AST *ast)
{
    if (debug)
        std::cout << "function_call_or_method ::= function_call_generic" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_or_method ::= postfix_expression DOT function_call_generic ;
void Delete::on_function_call_or_method_16(AST *ast)
{
    if (debug)
        std::cout << "function_call_or_method ::= postfix_expression DOT function_call_generic" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_generic ::= function_call_header_with_parameters RIGHT_PAREN ;
void Delete::on_function_call_generic_17(AST *ast)
{
    if (debug)
        std::cout << "function_call_generic ::= function_call_header_with_parameters RIGHT_PAREN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_generic ::= function_call_header_no_parameters RIGHT_PAREN ;
void Delete::on_function_call_generic_18(AST *ast)
{
    if (debug)
        std::cout << "function_call_generic ::= function_call_header_no_parameters RIGHT_PAREN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_header_no_parameters ::= function_call_header VOID ;
void Delete::on_function_call_header_no_parameters_19(AST *ast)
{
    if (debug)
        std::cout << "function_call_header_no_parameters ::= function_call_header VOID" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_header_no_parameters ::= function_call_header ;
void Delete::on_function_call_header_no_parameters_20(AST *ast)
{
    if (debug)
        std::cout << "function_call_header_no_parameters ::= function_call_header" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_header_with_parameters ::= function_call_header assignment_expression ;
void Delete::on_function_call_header_with_parameters_21(AST *ast)
{
    if (debug)
        std::cout << "function_call_header_with_parameters ::= function_call_header assignment_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_header_with_parameters ::= function_call_header_with_parameters COMMA assignment_expression ;
void Delete::on_function_call_header_with_parameters_22(AST *ast)
{
    if (debug)
        std::cout << "function_call_header_with_parameters ::= function_call_header_with_parameters COMMA assignment_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_call_header ::= function_identifier LEFT_PAREN ;
void Delete::on_function_call_header_23(AST *ast)
{
    if (debug)
        std::cout << "function_call_header ::= function_identifier LEFT_PAREN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_identifier ::= type_specifier ;
void Delete::on_function_identifier_24(AST *ast)
{
    if (debug)
        std::cout << "function_identifier ::= type_specifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_identifier ::= IDENTIFIER ;
void Delete::on_function_identifier_25(AST *ast)
{
    if (debug)
        std::cout << "function_identifier ::= IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// unary_expression ::= postfix_expression ;
void Delete::on_unary_expression_26(AST *ast)
{
    if (debug)
        std::cout << "unary_expression ::= postfix_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// unary_expression ::= INC_OP unary_expression ;
void Delete::on_unary_expression_27(AST *ast)
{
    if (debug)
        std::cout << "unary_expression ::= INC_OP unary_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// unary_expression ::= DEC_OP unary_expression ;
void Delete::on_unary_expression_28(AST *ast)
{
    if (debug)
        std::cout << "unary_expression ::= DEC_OP unary_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// unary_expression ::= unary_operator unary_expression ;
void Delete::on_unary_expression_29(AST *ast)
{
    if (debug)
        std::cout << "unary_expression ::= unary_operator unary_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// unary_operator ::= PLUS ;
void Delete::on_unary_operator_30(AST *ast)
{
    if (debug)
        std::cout << "unary_operator ::= PLUS" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// unary_operator ::= DASH ;
void Delete::on_unary_operator_31(AST *ast)
{
    if (debug)
        std::cout << "unary_operator ::= DASH" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// unary_operator ::= BANG ;
void Delete::on_unary_operator_32(AST *ast)
{
    if (debug)
        std::cout << "unary_operator ::= BANG" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// unary_operator ::= TILDE ;
void Delete::on_unary_operator_33(AST *ast)
{
    if (debug)
        std::cout << "unary_operator ::= TILDE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// multiplicative_expression ::= unary_expression ;
void Delete::on_multiplicative_expression_34(AST *ast)
{
    if (debug)
        std::cout << "multiplicative_expression ::= unary_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// multiplicative_expression ::= multiplicative_expression STAR unary_expression ;
void Delete::on_multiplicative_expression_35(AST *ast)
{
    if (debug)
        std::cout << "multiplicative_expression ::= multiplicative_expression STAR unary_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// multiplicative_expression ::= multiplicative_expression SLASH unary_expression ;
void Delete::on_multiplicative_expression_36(AST *ast)
{
    if (debug)
        std::cout << "multiplicative_expression ::= multiplicative_expression SLASH unary_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// multiplicative_expression ::= multiplicative_expression PERCENT unary_expression ;
void Delete::on_multiplicative_expression_37(AST *ast)
{
    if (debug)
        std::cout << "multiplicative_expression ::= multiplicative_expression PERCENT unary_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// additive_expression ::= multiplicative_expression ;
void Delete::on_additive_expression_38(AST *ast)
{
    if (debug)
        std::cout << "additive_expression ::= multiplicative_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// additive_expression ::= additive_expression PLUS multiplicative_expression ;
void Delete::on_additive_expression_39(AST *ast)
{
    if (debug)
        std::cout << "additive_expression ::= additive_expression PLUS multiplicative_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// additive_expression ::= additive_expression DASH multiplicative_expression ;
void Delete::on_additive_expression_40(AST *ast)
{
    if (debug)
        std::cout << "additive_expression ::= additive_expression DASH multiplicative_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// shift_expression ::= additive_expression ;
void Delete::on_shift_expression_41(AST *ast)
{
    if (debug)
        std::cout << "shift_expression ::= additive_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// shift_expression ::= shift_expression LEFT_OP additive_expression ;
void Delete::on_shift_expression_42(AST *ast)
{
    if (debug)
        std::cout << "shift_expression ::= shift_expression LEFT_OP additive_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// shift_expression ::= shift_expression RIGHT_OP additive_expression ;
void Delete::on_shift_expression_43(AST *ast)
{
    if (debug)
        std::cout << "shift_expression ::= shift_expression RIGHT_OP additive_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// relational_expression ::= shift_expression ;
void Delete::on_relational_expression_44(AST *ast)
{
    if (debug)
        std::cout << "relational_expression ::= shift_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// relational_expression ::= relational_expression LEFT_ANGLE shift_expression ;
void Delete::on_relational_expression_45(AST *ast)
{
    if (debug)
        std::cout << "relational_expression ::= relational_expression LEFT_ANGLE shift_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// relational_expression ::= relational_expression RIGHT_ANGLE shift_expression ;
void Delete::on_relational_expression_46(AST *ast)
{
    if (debug)
        std::cout << "relational_expression ::= relational_expression RIGHT_ANGLE shift_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// relational_expression ::= relational_expression LE_OP shift_expression ;
void Delete::on_relational_expression_47(AST *ast)
{
    if (debug)
        std::cout << "relational_expression ::= relational_expression LE_OP shift_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// relational_expression ::= relational_expression GE_OP shift_expression ;
void Delete::on_relational_expression_48(AST *ast)
{
    if (debug)
        std::cout << "relational_expression ::= relational_expression GE_OP shift_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// equality_expression ::= relational_expression ;
void Delete::on_equality_expression_49(AST *ast)
{
    if (debug)
        std::cout << "equality_expression ::= relational_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// equality_expression ::= equality_expression EQ_OP relational_expression ;
void Delete::on_equality_expression_50(AST *ast)
{
    if (debug)
        std::cout << "equality_expression ::= equality_expression EQ_OP relational_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// equality_expression ::= equality_expression NE_OP relational_expression ;
void Delete::on_equality_expression_51(AST *ast)
{
    if (debug)
        std::cout << "equality_expression ::= equality_expression NE_OP relational_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// and_expression ::= equality_expression ;
void Delete::on_and_expression_52(AST *ast)
{
    if (debug)
        std::cout << "and_expression ::= equality_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// and_expression ::= and_expression AMPERSAND equality_expression ;
void Delete::on_and_expression_53(AST *ast)
{
    if (debug)
        std::cout << "and_expression ::= and_expression AMPERSAND equality_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// exclusive_or_expression ::= and_expression ;
void Delete::on_exclusive_or_expression_54(AST *ast)
{
    if (debug)
        std::cout << "exclusive_or_expression ::= and_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// exclusive_or_expression ::= exclusive_or_expression CARET and_expression ;
void Delete::on_exclusive_or_expression_55(AST *ast)
{
    if (debug)
        std::cout << "exclusive_or_expression ::= exclusive_or_expression CARET and_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// inclusive_or_expression ::= exclusive_or_expression ;
void Delete::on_inclusive_or_expression_56(AST *ast)
{
    if (debug)
        std::cout << "inclusive_or_expression ::= exclusive_or_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// inclusive_or_expression ::= inclusive_or_expression VERTICAL_BAR exclusive_or_expression ;
void Delete::on_inclusive_or_expression_57(AST *ast)
{
    if (debug)
        std::cout << "inclusive_or_expression ::= inclusive_or_expression VERTICAL_BAR exclusive_or_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// logical_and_expression ::= inclusive_or_expression ;
void Delete::on_logical_and_expression_58(AST *ast)
{
    if (debug)
        std::cout << "logical_and_expression ::= inclusive_or_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// logical_and_expression ::= logical_and_expression AND_OP inclusive_or_expression ;
void Delete::on_logical_and_expression_59(AST *ast)
{
    if (debug)
        std::cout << "logical_and_expression ::= logical_and_expression AND_OP inclusive_or_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// logical_xor_expression ::= logical_and_expression ;
void Delete::on_logical_xor_expression_60(AST *ast)
{
    if (debug)
        std::cout << "logical_xor_expression ::= logical_and_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// logical_xor_expression ::= logical_xor_expression XOR_OP logical_and_expression ;
void Delete::on_logical_xor_expression_61(AST *ast)
{
    if (debug)
        std::cout << "logical_xor_expression ::= logical_xor_expression XOR_OP logical_and_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// logical_or_expression ::= logical_xor_expression ;
void Delete::on_logical_or_expression_62(AST *ast)
{
    if (debug)
        std::cout << "logical_or_expression ::= logical_xor_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// logical_or_expression ::= logical_or_expression OR_OP logical_xor_expression ;
void Delete::on_logical_or_expression_63(AST *ast)
{
    if (debug)
        std::cout << "logical_or_expression ::= logical_or_expression OR_OP logical_xor_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// conditional_expression ::= logical_or_expression ;
void Delete::on_conditional_expression_64(AST *ast)
{
    if (debug)
        std::cout << "conditional_expression ::= logical_or_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// conditional_expression ::= logical_or_expression QUESTION expression COLON assignment_expression ;
void Delete::on_conditional_expression_65(AST *ast)
{
    if (debug)
        std::cout << "conditional_expression ::= logical_or_expression QUESTION expression COLON assignment_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_expression ::= conditional_expression ;
void Delete::on_assignment_expression_66(AST *ast)
{
    if (debug)
        std::cout << "assignment_expression ::= conditional_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_expression ::= unary_expression assignment_operator assignment_expression ;
void Delete::on_assignment_expression_67(AST *ast)
{
    if (debug)
        std::cout << "assignment_expression ::= unary_expression assignment_operator assignment_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= EQUAL ;
void Delete::on_assignment_operator_68(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= EQUAL" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= MUL_ASSIGN ;
void Delete::on_assignment_operator_69(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= MUL_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= DIV_ASSIGN ;
void Delete::on_assignment_operator_70(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= DIV_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= MOD_ASSIGN ;
void Delete::on_assignment_operator_71(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= MOD_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= ADD_ASSIGN ;
void Delete::on_assignment_operator_72(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= ADD_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= SUB_ASSIGN ;
void Delete::on_assignment_operator_73(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= SUB_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= LEFT_ASSIGN ;
void Delete::on_assignment_operator_74(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= LEFT_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= RIGHT_ASSIGN ;
void Delete::on_assignment_operator_75(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= RIGHT_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= AND_ASSIGN ;
void Delete::on_assignment_operator_76(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= AND_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= XOR_ASSIGN ;
void Delete::on_assignment_operator_77(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= XOR_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// assignment_operator ::= OR_ASSIGN ;
void Delete::on_assignment_operator_78(AST *ast)
{
    if (debug)
        std::cout << "assignment_operator ::= OR_ASSIGN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// expression ::= assignment_expression ;
void Delete::on_expression_79(AST *ast)
{
    if (debug)
        std::cout << "expression ::= assignment_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// expression ::= expression COMMA assignment_expression ;
void Delete::on_expression_80(AST *ast)
{
    if (debug)
        std::cout << "expression ::= expression COMMA assignment_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// constant_expression ::= conditional_expression ;
void Delete::on_constant_expression_81(AST *ast)
{
    if (debug)
        std::cout << "constant_expression ::= conditional_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration ::= function_prototype SEMICOLON ;
void Delete::on_declaration_82(AST *ast)
{
    if (debug)
        std::cout << "declaration ::= function_prototype SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration ::= init_declarator_list SEMICOLON ;
void Delete::on_declaration_83(AST *ast)
{
    if (debug)
        std::cout << "declaration ::= init_declarator_list SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration ::= PRECISION precision_qualifier type_specifier_no_prec SEMICOLON ;
void Delete::on_declaration_84(AST *ast)
{
    if (debug)
        std::cout << "declaration ::= PRECISION precision_qualifier type_specifier_no_prec SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE SEMICOLON ;
void Delete::on_declaration_85(AST *ast)
{
    if (debug)
        std::cout << "declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER SEMICOLON ;
void Delete::on_declaration_86(AST *ast)
{
    if (debug)
        std::cout << "declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER LEFT_BRACKET RIGHT_BRACKET SEMICOLON ;
void Delete::on_declaration_87(AST *ast)
{
    if (debug)
        std::cout << "declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER LEFT_BRACKET RIGHT_BRACKET SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET SEMICOLON ;
void Delete::on_declaration_88(AST *ast)
{
    if (debug)
        std::cout << "declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration ::= type_qualifier SEMICOLON ;
void Delete::on_declaration_89(AST *ast)
{
    if (debug)
        std::cout << "declaration ::= type_qualifier SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_prototype ::= function_declarator RIGHT_PAREN ;
void Delete::on_function_prototype_90(AST *ast)
{
    if (debug)
        std::cout << "function_prototype ::= function_declarator RIGHT_PAREN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_declarator ::= function_header ;
void Delete::on_function_declarator_91(AST *ast)
{
    if (debug)
        std::cout << "function_declarator ::= function_header" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_declarator ::= function_header_with_parameters ;
void Delete::on_function_declarator_92(AST *ast)
{
    if (debug)
        std::cout << "function_declarator ::= function_header_with_parameters" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_header_with_parameters ::= function_header parameter_declaration ;
void Delete::on_function_header_with_parameters_93(AST *ast)
{
    if (debug)
        std::cout << "function_header_with_parameters ::= function_header parameter_declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_header_with_parameters ::= function_header_with_parameters COMMA parameter_declaration ;
void Delete::on_function_header_with_parameters_94(AST *ast)
{
    if (debug)
        std::cout << "function_header_with_parameters ::= function_header_with_parameters COMMA parameter_declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_header ::= fully_specified_type IDENTIFIER LEFT_PAREN ;
void Delete::on_function_header_95(AST *ast)
{
    if (debug)
        std::cout << "function_header ::= fully_specified_type IDENTIFIER LEFT_PAREN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_declarator ::= type_specifier IDENTIFIER ;
void Delete::on_parameter_declarator_96(AST *ast)
{
    if (debug)
        std::cout << "parameter_declarator ::= type_specifier IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_declarator ::= type_specifier IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
void Delete::on_parameter_declarator_97(AST *ast)
{
    if (debug)
        std::cout << "parameter_declarator ::= type_specifier IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_declaration ::= parameter_type_qualifier parameter_qualifier parameter_declarator ;
void Delete::on_parameter_declaration_98(AST *ast)
{
    if (debug)
        std::cout << "parameter_declaration ::= parameter_type_qualifier parameter_qualifier parameter_declarator" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_declaration ::= parameter_qualifier parameter_declarator ;
void Delete::on_parameter_declaration_99(AST *ast)
{
    if (debug)
        std::cout << "parameter_declaration ::= parameter_qualifier parameter_declarator" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_declaration ::= parameter_type_qualifier parameter_qualifier parameter_type_specifier ;
void Delete::on_parameter_declaration_100(AST *ast)
{
    if (debug)
        std::cout << "parameter_declaration ::= parameter_type_qualifier parameter_qualifier parameter_type_specifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_declaration ::= parameter_qualifier parameter_type_specifier ;
void Delete::on_parameter_declaration_101(AST *ast)
{
    if (debug)
        std::cout << "parameter_declaration ::= parameter_qualifier parameter_type_specifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_qualifier ::= ;
void Delete::on_parameter_qualifier_102(AST *ast)
{
    if (debug)
        std::cout << "parameter_qualifier ::=" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_qualifier ::= IN ;
void Delete::on_parameter_qualifier_103(AST *ast)
{
    if (debug)
        std::cout << "parameter_qualifier ::= IN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_qualifier ::= OUT ;
void Delete::on_parameter_qualifier_104(AST *ast)
{
    if (debug)
        std::cout << "parameter_qualifier ::= OUT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_qualifier ::= INOUT ;
void Delete::on_parameter_qualifier_105(AST *ast)
{
    if (debug)
        std::cout << "parameter_qualifier ::= INOUT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_type_specifier ::= type_specifier ;
void Delete::on_parameter_type_specifier_106(AST *ast)
{
    if (debug)
        std::cout << "parameter_type_specifier ::= type_specifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// init_declarator_list ::= single_declaration ;
void Delete::on_init_declarator_list_107(AST *ast)
{
    if (debug)
        std::cout << "init_declarator_list ::= single_declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// init_declarator_list ::= init_declarator_list COMMA IDENTIFIER ;
void Delete::on_init_declarator_list_108(AST *ast)
{
    if (debug)
        std::cout << "init_declarator_list ::= init_declarator_list COMMA IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
void Delete::on_init_declarator_list_109(AST *ast)
{
    if (debug)
        std::cout << "init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
void Delete::on_init_declarator_list_110(AST *ast)
{
    if (debug)
        std::cout << "init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET EQUAL initializer ;
void Delete::on_init_declarator_list_111(AST *ast)
{
    if (debug)
        std::cout << "init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET EQUAL initializer" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET EQUAL initializer ;
void Delete::on_init_declarator_list_112(AST *ast)
{
    if (debug)
        std::cout << "init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET EQUAL initializer" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// init_declarator_list ::= init_declarator_list COMMA IDENTIFIER EQUAL initializer ;
void Delete::on_init_declarator_list_113(AST *ast)
{
    if (debug)
        std::cout << "init_declarator_list ::= init_declarator_list COMMA IDENTIFIER EQUAL initializer" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// single_declaration ::= fully_specified_type ;
void Delete::on_single_declaration_114(AST *ast)
{
    if (debug)
        std::cout << "single_declaration ::= fully_specified_type" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// single_declaration ::= fully_specified_type IDENTIFIER ;
void Delete::on_single_declaration_115(AST *ast)
{
    if (debug)
        std::cout << "single_declaration ::= fully_specified_type IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
void Delete::on_single_declaration_116(AST *ast)
{
    if (debug)
        std::cout << "single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
void Delete::on_single_declaration_117(AST *ast)
{
    if (debug)
        std::cout << "single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET EQUAL initializer ;
void Delete::on_single_declaration_118(AST *ast)
{
    if (debug)
        std::cout << "single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET EQUAL initializer" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET EQUAL initializer ;
void Delete::on_single_declaration_119(AST *ast)
{
    if (debug)
        std::cout << "single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET EQUAL initializer" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// single_declaration ::= fully_specified_type IDENTIFIER EQUAL initializer ;
void Delete::on_single_declaration_120(AST *ast)
{
    if (debug)
        std::cout << "single_declaration ::= fully_specified_type IDENTIFIER EQUAL initializer" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// single_declaration ::= INVARIANT IDENTIFIER ;
void Delete::on_single_declaration_121(AST *ast)
{
    if (debug)
        std::cout << "single_declaration ::= INVARIANT IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// fully_specified_type ::= type_specifier ;
void Delete::on_fully_specified_type_122(AST *ast)
{
    if (debug)
        std::cout << "fully_specified_type ::= type_specifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// fully_specified_type ::= type_qualifier type_specifier ;
void Delete::on_fully_specified_type_123(AST *ast)
{
    if (debug)
        std::cout << "fully_specified_type ::= type_qualifier type_specifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// invariant_qualifier ::= INVARIANT ;
void Delete::on_invariant_qualifier_124(AST *ast)
{
    if (debug)
        std::cout << "invariant_qualifier ::= INVARIANT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// interpolation_qualifier ::= SMOOTH ;
void Delete::on_interpolation_qualifier_125(AST *ast)
{
    if (debug)
        std::cout << "interpolation_qualifier ::= SMOOTH" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// interpolation_qualifier ::= FLAT ;
void Delete::on_interpolation_qualifier_126(AST *ast)
{
    if (debug)
        std::cout << "interpolation_qualifier ::= FLAT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// interpolation_qualifier ::= NOPERSPECTIVE ;
void Delete::on_interpolation_qualifier_127(AST *ast)
{
    if (debug)
        std::cout << "interpolation_qualifier ::= NOPERSPECTIVE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// layout_qualifier ::= LAYOUT LEFT_PAREN layout_qualifier_id_list RIGHT_PAREN ;
void Delete::on_layout_qualifier_128(AST *ast)
{
    if (debug)
        std::cout << "layout_qualifier ::= LAYOUT LEFT_PAREN layout_qualifier_id_list RIGHT_PAREN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// layout_qualifier_id_list ::= layout_qualifier_id ;
void Delete::on_layout_qualifier_id_list_129(AST *ast)
{
    if (debug)
        std::cout << "layout_qualifier_id_list ::= layout_qualifier_id" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// layout_qualifier_id_list ::= layout_qualifier_id_list COMMA layout_qualifier_id ;
void Delete::on_layout_qualifier_id_list_130(AST *ast)
{
    if (debug)
        std::cout << "layout_qualifier_id_list ::= layout_qualifier_id_list COMMA layout_qualifier_id" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// layout_qualifier_id ::= IDENTIFIER ;
void Delete::on_layout_qualifier_id_131(AST *ast)
{
    if (debug)
        std::cout << "layout_qualifier_id ::= IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// layout_qualifier_id ::= IDENTIFIER EQUAL NUMBER ;
void Delete::on_layout_qualifier_id_132(AST *ast)
{
    if (debug)
        std::cout << "layout_qualifier_id ::= IDENTIFIER EQUAL NUMBER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// parameter_type_qualifier ::= CONST ;
void Delete::on_parameter_type_qualifier_133(AST *ast)
{
    if (debug)
        std::cout << "parameter_type_qualifier ::= CONST" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_qualifier ::= storage_qualifier ;
void Delete::on_type_qualifier_134(AST *ast)
{
    if (debug)
        std::cout << "type_qualifier ::= storage_qualifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_qualifier ::= layout_qualifier ;
void Delete::on_type_qualifier_135(AST *ast)
{
    if (debug)
        std::cout << "type_qualifier ::= layout_qualifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_qualifier ::= layout_qualifier storage_qualifier ;
void Delete::on_type_qualifier_136(AST *ast)
{
    if (debug)
        std::cout << "type_qualifier ::= layout_qualifier storage_qualifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_qualifier ::= interpolation_qualifier storage_qualifier ;
void Delete::on_type_qualifier_137(AST *ast)
{
    if (debug)
        std::cout << "type_qualifier ::= interpolation_qualifier storage_qualifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_qualifier ::= interpolation_qualifier ;
void Delete::on_type_qualifier_138(AST *ast)
{
    if (debug)
        std::cout << "type_qualifier ::= interpolation_qualifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_qualifier ::= invariant_qualifier storage_qualifier ;
void Delete::on_type_qualifier_139(AST *ast)
{
    if (debug)
        std::cout << "type_qualifier ::= invariant_qualifier storage_qualifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_qualifier ::= invariant_qualifier interpolation_qualifier storage_qualifier ;
void Delete::on_type_qualifier_140(AST *ast)
{
    if (debug)
        std::cout << "type_qualifier ::= invariant_qualifier interpolation_qualifier storage_qualifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_qualifier ::= INVARIANT ;
void Delete::on_type_qualifier_141(AST *ast)
{
    if (debug)
        std::cout << "type_qualifier ::= INVARIANT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= CONST ;
void Delete::on_storage_qualifier_142(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= CONST" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= ATTRIBUTE ;
void Delete::on_storage_qualifier_143(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= ATTRIBUTE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= VARYING ;
void Delete::on_storage_qualifier_144(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= VARYING" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= CENTROID VARYING ;
void Delete::on_storage_qualifier_145(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= CENTROID VARYING" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= IN ;
void Delete::on_storage_qualifier_146(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= IN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= OUT ;
void Delete::on_storage_qualifier_147(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= OUT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= CENTROID IN ;
void Delete::on_storage_qualifier_148(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= CENTROID IN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= CENTROID OUT ;
void Delete::on_storage_qualifier_149(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= CENTROID OUT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= PATCH IN ;
void Delete::on_storage_qualifier_150(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= PATCH IN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= PATCH OUT ;
void Delete::on_storage_qualifier_151(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= PATCH OUT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= SAMPLE IN ;
void Delete::on_storage_qualifier_152(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= SAMPLE IN" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= SAMPLE OUT ;
void Delete::on_storage_qualifier_153(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= SAMPLE OUT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// storage_qualifier ::= UNIFORM ;
void Delete::on_storage_qualifier_154(AST *ast)
{
    if (debug)
        std::cout << "storage_qualifier ::= UNIFORM" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier ::= type_specifier_no_prec ;
void Delete::on_type_specifier_155(AST *ast)
{
    if (debug)
        std::cout << "type_specifier ::= type_specifier_no_prec" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier ::= precision_qualifier type_specifier_no_prec ;
void Delete::on_type_specifier_156(AST *ast)
{
    if (debug)
        std::cout << "type_specifier ::= precision_qualifier type_specifier_no_prec" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_no_prec ::= type_specifier_nonarray ;
void Delete::on_type_specifier_no_prec_157(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_no_prec ::= type_specifier_nonarray" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_no_prec ::= type_specifier_nonarray LEFT_BRACKET RIGHT_BRACKET ;
void Delete::on_type_specifier_no_prec_158(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_no_prec ::= type_specifier_nonarray LEFT_BRACKET RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_no_prec ::= type_specifier_nonarray LEFT_BRACKET constant_expression RIGHT_BRACKET ;
void Delete::on_type_specifier_no_prec_159(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_no_prec ::= type_specifier_nonarray LEFT_BRACKET constant_expression RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= VOID ;
void Delete::on_type_specifier_nonarray_160(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= VOID" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= FLOAT ;
void Delete::on_type_specifier_nonarray_161(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= FLOAT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DOUBLE ;
void Delete::on_type_specifier_nonarray_162(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DOUBLE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= INT ;
void Delete::on_type_specifier_nonarray_163(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= INT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= UINT ;
void Delete::on_type_specifier_nonarray_164(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= UINT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= BOOL ;
void Delete::on_type_specifier_nonarray_165(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= BOOL" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= VEC2 ;
void Delete::on_type_specifier_nonarray_166(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= VEC2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= VEC3 ;
void Delete::on_type_specifier_nonarray_167(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= VEC3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= VEC4 ;
void Delete::on_type_specifier_nonarray_168(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= VEC4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DVEC2 ;
void Delete::on_type_specifier_nonarray_169(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DVEC2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DVEC3 ;
void Delete::on_type_specifier_nonarray_170(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DVEC3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DVEC4 ;
void Delete::on_type_specifier_nonarray_171(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DVEC4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= BVEC2 ;
void Delete::on_type_specifier_nonarray_172(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= BVEC2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= BVEC3 ;
void Delete::on_type_specifier_nonarray_173(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= BVEC3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= BVEC4 ;
void Delete::on_type_specifier_nonarray_174(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= BVEC4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= IVEC2 ;
void Delete::on_type_specifier_nonarray_175(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= IVEC2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= IVEC3 ;
void Delete::on_type_specifier_nonarray_176(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= IVEC3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= IVEC4 ;
void Delete::on_type_specifier_nonarray_177(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= IVEC4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= UVEC2 ;
void Delete::on_type_specifier_nonarray_178(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= UVEC2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= UVEC3 ;
void Delete::on_type_specifier_nonarray_179(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= UVEC3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= UVEC4 ;
void Delete::on_type_specifier_nonarray_180(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= UVEC4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT2 ;
void Delete::on_type_specifier_nonarray_181(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT3 ;
void Delete::on_type_specifier_nonarray_182(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT4 ;
void Delete::on_type_specifier_nonarray_183(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT2X2 ;
void Delete::on_type_specifier_nonarray_184(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT2X2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT2X3 ;
void Delete::on_type_specifier_nonarray_185(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT2X3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT2X4 ;
void Delete::on_type_specifier_nonarray_186(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT2X4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT3X2 ;
void Delete::on_type_specifier_nonarray_187(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT3X2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT3X3 ;
void Delete::on_type_specifier_nonarray_188(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT3X3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT3X4 ;
void Delete::on_type_specifier_nonarray_189(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT3X4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT4X2 ;
void Delete::on_type_specifier_nonarray_190(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT4X2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT4X3 ;
void Delete::on_type_specifier_nonarray_191(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT4X3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= MAT4X4 ;
void Delete::on_type_specifier_nonarray_192(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= MAT4X4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT2 ;
void Delete::on_type_specifier_nonarray_193(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT3 ;
void Delete::on_type_specifier_nonarray_194(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT4 ;
void Delete::on_type_specifier_nonarray_195(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT2X2 ;
void Delete::on_type_specifier_nonarray_196(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT2X2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT2X3 ;
void Delete::on_type_specifier_nonarray_197(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT2X3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT2X4 ;
void Delete::on_type_specifier_nonarray_198(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT2X4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT3X2 ;
void Delete::on_type_specifier_nonarray_199(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT3X2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT3X3 ;
void Delete::on_type_specifier_nonarray_200(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT3X3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT3X4 ;
void Delete::on_type_specifier_nonarray_201(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT3X4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT4X2 ;
void Delete::on_type_specifier_nonarray_202(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT4X2" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT4X3 ;
void Delete::on_type_specifier_nonarray_203(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT4X3" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= DMAT4X4 ;
void Delete::on_type_specifier_nonarray_204(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= DMAT4X4" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER1D ;
void Delete::on_type_specifier_nonarray_205(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER1D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER2D ;
void Delete::on_type_specifier_nonarray_206(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER2D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER3D ;
void Delete::on_type_specifier_nonarray_207(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER3D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLERCUBE ;
void Delete::on_type_specifier_nonarray_208(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLERCUBE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER1DSHADOW ;
void Delete::on_type_specifier_nonarray_209(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER1DSHADOW" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER2DSHADOW ;
void Delete::on_type_specifier_nonarray_210(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER2DSHADOW" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLERCUBESHADOW ;
void Delete::on_type_specifier_nonarray_211(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLERCUBESHADOW" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER1DARRAY ;
void Delete::on_type_specifier_nonarray_212(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER1DARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER2DARRAY ;
void Delete::on_type_specifier_nonarray_213(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER2DARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER1DARRAYSHADOW ;
void Delete::on_type_specifier_nonarray_214(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER1DARRAYSHADOW" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER2DARRAYSHADOW ;
void Delete::on_type_specifier_nonarray_215(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER2DARRAYSHADOW" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLERCUBEARRAY ;
void Delete::on_type_specifier_nonarray_216(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLERCUBEARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLERCUBEARRAYSHADOW ;
void Delete::on_type_specifier_nonarray_217(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLERCUBEARRAYSHADOW" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLER1D ;
void Delete::on_type_specifier_nonarray_218(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLER1D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLER2D ;
void Delete::on_type_specifier_nonarray_219(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLER2D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLER3D ;
void Delete::on_type_specifier_nonarray_220(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLER3D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLERCUBE ;
void Delete::on_type_specifier_nonarray_221(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLERCUBE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLER1DARRAY ;
void Delete::on_type_specifier_nonarray_222(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLER1DARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLER2DARRAY ;
void Delete::on_type_specifier_nonarray_223(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLER2DARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLERCUBEARRAY ;
void Delete::on_type_specifier_nonarray_224(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLERCUBEARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLER1D ;
void Delete::on_type_specifier_nonarray_225(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLER1D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLER2D ;
void Delete::on_type_specifier_nonarray_226(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLER2D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLER3D ;
void Delete::on_type_specifier_nonarray_227(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLER3D" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLERCUBE ;
void Delete::on_type_specifier_nonarray_228(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLERCUBE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLER1DARRAY ;
void Delete::on_type_specifier_nonarray_229(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLER1DARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLER2DARRAY ;
void Delete::on_type_specifier_nonarray_230(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLER2DARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLERCUBEARRAY ;
void Delete::on_type_specifier_nonarray_231(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLERCUBEARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER2DRECT ;
void Delete::on_type_specifier_nonarray_232(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER2DRECT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER2DRECTSHADOW ;
void Delete::on_type_specifier_nonarray_233(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER2DRECTSHADOW" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLER2DRECT ;
void Delete::on_type_specifier_nonarray_234(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLER2DRECT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLER2DRECT ;
void Delete::on_type_specifier_nonarray_235(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLER2DRECT" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLERBUFFER ;
void Delete::on_type_specifier_nonarray_236(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLERBUFFER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLERBUFFER ;
void Delete::on_type_specifier_nonarray_237(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLERBUFFER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLERBUFFER ;
void Delete::on_type_specifier_nonarray_238(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLERBUFFER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER2DMS ;
void Delete::on_type_specifier_nonarray_239(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER2DMS" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLER2DMS ;
void Delete::on_type_specifier_nonarray_240(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLER2DMS" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLER2DMS ;
void Delete::on_type_specifier_nonarray_241(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLER2DMS" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= SAMPLER2DMSARRAY ;
void Delete::on_type_specifier_nonarray_242(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= SAMPLER2DMSARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= ISAMPLER2DMSARRAY ;
void Delete::on_type_specifier_nonarray_243(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= ISAMPLER2DMSARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= USAMPLER2DMSARRAY ;
void Delete::on_type_specifier_nonarray_244(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= USAMPLER2DMSARRAY" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= struct_specifier ;
void Delete::on_type_specifier_nonarray_245(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= struct_specifier" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// type_specifier_nonarray ::= TYPE_NAME ;
void Delete::on_type_specifier_nonarray_246(AST *ast)
{
    if (debug)
        std::cout << "type_specifier_nonarray ::= TYPE_NAME" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// precision_qualifier ::= HIGHP ;
void Delete::on_precision_qualifier_247(AST *ast)
{
    if (debug)
        std::cout << "precision_qualifier ::= HIGHP" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// precision_qualifier ::= MEDIUMP ;
void Delete::on_precision_qualifier_248(AST *ast)
{
    if (debug)
        std::cout << "precision_qualifier ::= MEDIUMP" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// precision_qualifier ::= LOWP ;
void Delete::on_precision_qualifier_249(AST *ast)
{
    if (debug)
        std::cout << "precision_qualifier ::= LOWP" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_specifier ::= STRUCT IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE ;
void Delete::on_struct_specifier_250(AST *ast)
{
    if (debug)
        std::cout << "struct_specifier ::= STRUCT IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_specifier ::= STRUCT LEFT_BRACE struct_declaration_list RIGHT_BRACE ;
void Delete::on_struct_specifier_251(AST *ast)
{
    if (debug)
        std::cout << "struct_specifier ::= STRUCT LEFT_BRACE struct_declaration_list RIGHT_BRACE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declaration_list ::= struct_declaration ;
void Delete::on_struct_declaration_list_252(AST *ast)
{
    if (debug)
        std::cout << "struct_declaration_list ::= struct_declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declaration_list ::= struct_declaration_list struct_declaration ;
void Delete::on_struct_declaration_list_253(AST *ast)
{
    if (debug)
        std::cout << "struct_declaration_list ::= struct_declaration_list struct_declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declaration ::= type_specifier struct_declarator_list SEMICOLON ;
void Delete::on_struct_declaration_254(AST *ast)
{
    if (debug)
        std::cout << "struct_declaration ::= type_specifier struct_declarator_list SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declaration ::= type_qualifier type_specifier struct_declarator_list SEMICOLON ;
void Delete::on_struct_declaration_255(AST *ast)
{
    if (debug)
        std::cout << "struct_declaration ::= type_qualifier type_specifier struct_declarator_list SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declarator_list ::= struct_declarator ;
void Delete::on_struct_declarator_list_256(AST *ast)
{
    if (debug)
        std::cout << "struct_declarator_list ::= struct_declarator" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declarator_list ::= struct_declarator_list COMMA struct_declarator ;
void Delete::on_struct_declarator_list_257(AST *ast)
{
    if (debug)
        std::cout << "struct_declarator_list ::= struct_declarator_list COMMA struct_declarator" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declarator ::= IDENTIFIER ;
void Delete::on_struct_declarator_258(AST *ast)
{
    if (debug)
        std::cout << "struct_declarator ::= IDENTIFIER" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declarator ::= IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
void Delete::on_struct_declarator_259(AST *ast)
{
    if (debug)
        std::cout << "struct_declarator ::= IDENTIFIER LEFT_BRACKET RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// struct_declarator ::= IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
void Delete::on_struct_declarator_260(AST *ast)
{
    if (debug)
        std::cout << "struct_declarator ::= IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// initializer ::= assignment_expression ;
void Delete::on_initializer_261(AST *ast)
{
    if (debug)
        std::cout << "initializer ::= assignment_expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// declaration_statement ::= declaration ;
void Delete::on_declaration_statement_262(AST *ast)
{
    if (debug)
        std::cout << "declaration_statement ::= declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// statement ::= compound_statement ;
void Delete::on_statement_263(AST *ast)
{
    if (debug)
        std::cout << "statement ::= compound_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// statement ::= simple_statement ;
void Delete::on_statement_264(AST *ast)
{
    if (debug)
        std::cout << "statement ::= simple_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// simple_statement ::= declaration_statement ;
void Delete::on_simple_statement_265(AST *ast)
{
    if (debug)
        std::cout << "simple_statement ::= declaration_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// simple_statement ::= expression_statement ;
void Delete::on_simple_statement_266(AST *ast)
{
    if (debug)
        std::cout << "simple_statement ::= expression_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// simple_statement ::= selection_statement ;
void Delete::on_simple_statement_267(AST *ast)
{
    if (debug)
        std::cout << "simple_statement ::= selection_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// simple_statement ::= switch_statement ;
void Delete::on_simple_statement_268(AST *ast)
{
    if (debug)
        std::cout << "simple_statement ::= switch_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// simple_statement ::= case_label ;
void Delete::on_simple_statement_269(AST *ast)
{
    if (debug)
        std::cout << "simple_statement ::= case_label" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// simple_statement ::= iteration_statement ;
void Delete::on_simple_statement_270(AST *ast)
{
    if (debug)
        std::cout << "simple_statement ::= iteration_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// simple_statement ::= jump_statement ;
void Delete::on_simple_statement_271(AST *ast)
{
    if (debug)
        std::cout << "simple_statement ::= jump_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// compound_statement ::= LEFT_BRACE RIGHT_BRACE ;
void Delete::on_compound_statement_272(AST *ast)
{
    if (debug)
        std::cout << "compound_statement ::= LEFT_BRACE RIGHT_BRACE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// compound_statement ::= LEFT_BRACE statement_list RIGHT_BRACE ;
void Delete::on_compound_statement_273(AST *ast)
{
    if (debug)
        std::cout << "compound_statement ::= LEFT_BRACE statement_list RIGHT_BRACE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// statement_no_new_scope ::= compound_statement_no_new_scope ;
void Delete::on_statement_no_new_scope_274(AST *ast)
{
    if (debug)
        std::cout << "statement_no_new_scope ::= compound_statement_no_new_scope" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// statement_no_new_scope ::= simple_statement ;
void Delete::on_statement_no_new_scope_275(AST *ast)
{
    if (debug)
        std::cout << "statement_no_new_scope ::= simple_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// compound_statement_no_new_scope ::= LEFT_BRACE RIGHT_BRACE ;
void Delete::on_compound_statement_no_new_scope_276(AST *ast)
{
    if (debug)
        std::cout << "compound_statement_no_new_scope ::= LEFT_BRACE RIGHT_BRACE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// compound_statement_no_new_scope ::= LEFT_BRACE statement_list RIGHT_BRACE ;
void Delete::on_compound_statement_no_new_scope_277(AST *ast)
{
    if (debug)
        std::cout << "compound_statement_no_new_scope ::= LEFT_BRACE statement_list RIGHT_BRACE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// statement_list ::= statement ;
void Delete::on_statement_list_278(AST *ast)
{
    if (debug)
        std::cout << "statement_list ::= statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// statement_list ::= statement_list statement ;
void Delete::on_statement_list_279(AST *ast)
{
    if (debug)
        std::cout << "statement_list ::= statement_list statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// expression_statement ::= SEMICOLON ;
void Delete::on_expression_statement_280(AST *ast)
{
    if (debug)
        std::cout << "expression_statement ::= SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// expression_statement ::= expression SEMICOLON ;
void Delete::on_expression_statement_281(AST *ast)
{
    if (debug)
        std::cout << "expression_statement ::= expression SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// selection_statement ::= IF LEFT_PAREN expression RIGHT_PAREN selection_rest_statement ;
void Delete::on_selection_statement_282(AST *ast)
{
    if (debug)
        std::cout << "selection_statement ::= IF LEFT_PAREN expression RIGHT_PAREN selection_rest_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// selection_rest_statement ::= statement ELSE statement ;
void Delete::on_selection_rest_statement_283(AST *ast)
{
    if (debug)
        std::cout << "selection_rest_statement ::= statement ELSE statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// selection_rest_statement ::= statement ;
void Delete::on_selection_rest_statement_284(AST *ast)
{
    if (debug)
        std::cout << "selection_rest_statement ::= statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// condition ::= expression ;
void Delete::on_condition_285(AST *ast)
{
    if (debug)
        std::cout << "condition ::= expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// condition ::= fully_specified_type IDENTIFIER EQUAL initializer ;
void Delete::on_condition_286(AST *ast)
{
    if (debug)
        std::cout << "condition ::= fully_specified_type IDENTIFIER EQUAL initializer" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// switch_statement ::= SWITCH LEFT_PAREN expression RIGHT_PAREN LEFT_BRACE switch_statement_list RIGHT_BRACE ;
void Delete::on_switch_statement_287(AST *ast)
{
    if (debug)
        std::cout << "switch_statement ::= SWITCH LEFT_PAREN expression RIGHT_PAREN LEFT_BRACE switch_statement_list RIGHT_BRACE" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// switch_statement_list ::= ;
void Delete::on_switch_statement_list_288(AST *ast)
{
    if (debug)
        std::cout << "switch_statement_list ::=" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// switch_statement_list ::= statement_list ;
void Delete::on_switch_statement_list_289(AST *ast)
{
    if (debug)
        std::cout << "switch_statement_list ::= statement_list" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// case_label ::= CASE expression COLON ;
void Delete::on_case_label_290(AST *ast)
{
    if (debug)
        std::cout << "case_label ::= CASE expression COLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// case_label ::= DEFAULT COLON ;
void Delete::on_case_label_291(AST *ast)
{
    if (debug)
        std::cout << "case_label ::= DEFAULT COLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// iteration_statement ::= WHILE LEFT_PAREN condition RIGHT_PAREN statement_no_new_scope ;
void Delete::on_iteration_statement_292(AST *ast)
{
    if (debug)
        std::cout << "iteration_statement ::= WHILE LEFT_PAREN condition RIGHT_PAREN statement_no_new_scope" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// iteration_statement ::= DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON ;
void Delete::on_iteration_statement_293(AST *ast)
{
    if (debug)
        std::cout << "iteration_statement ::= DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// iteration_statement ::= FOR LEFT_PAREN for_init_statement for_rest_statement RIGHT_PAREN statement_no_new_scope ;
void Delete::on_iteration_statement_294(AST *ast)
{
    if (debug)
        std::cout << "iteration_statement ::= FOR LEFT_PAREN for_init_statement for_rest_statement RIGHT_PAREN statement_no_new_scope" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// for_init_statement ::= expression_statement ;
void Delete::on_for_init_statement_295(AST *ast)
{
    if (debug)
        std::cout << "for_init_statement ::= expression_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// for_init_statement ::= declaration_statement ;
void Delete::on_for_init_statement_296(AST *ast)
{
    if (debug)
        std::cout << "for_init_statement ::= declaration_statement" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// conditionopt ::= ;
void Delete::on_conditionopt_297(AST *ast)
{
    if (debug)
        std::cout << "conditionopt ::=" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// conditionopt ::= condition ;
void Delete::on_conditionopt_298(AST *ast)
{
    if (debug)
        std::cout << "conditionopt ::= condition" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// for_rest_statement ::= conditionopt SEMICOLON ;
void Delete::on_for_rest_statement_299(AST *ast)
{
    if (debug)
        std::cout << "for_rest_statement ::= conditionopt SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// for_rest_statement ::= conditionopt SEMICOLON expression ;
void Delete::on_for_rest_statement_300(AST *ast)
{
    if (debug)
        std::cout << "for_rest_statement ::= conditionopt SEMICOLON expression" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// jump_statement ::= CONTINUE SEMICOLON ;
void Delete::on_jump_statement_301(AST *ast)
{
    if (debug)
        std::cout << "jump_statement ::= CONTINUE SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// jump_statement ::= BREAK SEMICOLON ;
void Delete::on_jump_statement_302(AST *ast)
{
    if (debug)
        std::cout << "jump_statement ::= BREAK SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// jump_statement ::= RETURN SEMICOLON ;
void Delete::on_jump_statement_303(AST *ast)
{
    if (debug)
        std::cout << "jump_statement ::= RETURN SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// jump_statement ::= RETURN expression SEMICOLON ;
void Delete::on_jump_statement_304(AST *ast)
{
    if (debug)
        std::cout << "jump_statement ::= RETURN expression SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// jump_statement ::= DISCARD SEMICOLON ;
void Delete::on_jump_statement_305(AST *ast)
{
    if (debug)
        std::cout << "jump_statement ::= DISCARD SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// translation_unit ::= external_declaration ;
void Delete::on_translation_unit_306(AST *ast)
{
    if (debug)
        std::cout << "translation_unit ::= external_declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// translation_unit ::= translation_unit external_declaration ;
void Delete::on_translation_unit_307(AST *ast)
{
    if (debug)
        std::cout << "translation_unit ::= translation_unit external_declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// external_declaration ::= function_definition ;
void Delete::on_external_declaration_308(AST *ast)
{
    if (debug)
        std::cout << "external_declaration ::= function_definition" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// external_declaration ::= declaration ;
void Delete::on_external_declaration_309(AST *ast)
{
    if (debug)
        std::cout << "external_declaration ::= declaration" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// external_declaration ::= SEMICOLON ;
void Delete::on_external_declaration_310(AST *ast)
{
    if (debug)
        std::cout << "external_declaration ::= SEMICOLON" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

// function_definition ::= function_prototype compound_statement_no_new_scope ;
void Delete::on_function_definition_311(AST *ast)
{
    if (debug)
        std::cout << "function_definition ::= function_prototype compound_statement_no_new_scope" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}

