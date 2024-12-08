//
// Created by notbonzo on 12/8/24.
//

#ifndef CODEGEN_HH
#define CODEGEN_HH

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "AST.icc"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/MC/TargetRegistry.h>

namespace ent {

    class codegen {
    public:
        explicit codegen(std::string_view module_name);
        ~codegen() = default;

        codegen(const codegen&) = delete;
        codegen& operator=(const codegen&) = delete;
        codegen(codegen&&) = delete;
        codegen& operator=(codegen&&) = delete;

        bool generate_code(const std::shared_ptr<ast::program_node> &root);

        [[nodiscard]] bool write_ir_to_file(std::string_view filename) const;
        bool write_ir_to_stream(std::ostream &os) const;
        void set_target_triple(std::string_view triple);
        void set_data_layout(std::string_view layout) const;
        bool compile_to_object(std::string_view filename);

        [[nodiscard]] llvm::Module* get_module() const;

    private:
        static std::string mangle_name(std::string_view base_name, const std::vector<variable_type>& args);
        static std::string mangle_type(const variable_type& vtype);

        llvm::Value* emit_node(const std::shared_ptr<ast::base_node> &node);
        llvm::Value* emit_expression_node(const std::shared_ptr<ast::expression_node> &expr);
        llvm::Value* emit_binary_node(const std::shared_ptr<ast::binary_node> &bin);
        llvm::Value* emit_unary_node(const std::shared_ptr<ast::unary_node> &un);
        llvm::Value* emit_literal_node(const std::shared_ptr<ast::literal_node> &lit);
        llvm::Value* emit_string_literal_node(const std::shared_ptr<ast::string_literal_node> &str);
        llvm::Value* emit_variable_node(const std::shared_ptr<ast::variable_node> &var);
        llvm::Value* emit_variable_declaration_node(const std::shared_ptr<ast::variable_declaration_node> &decl);
        llvm::Value* emit_variable_declaration_assign_node(const std::shared_ptr<ast::variable_declaration_assign_node> &decl_assign);
        llvm::Value* emit_assignment_node(const std::shared_ptr<ast::assignment_node> &assign);
        llvm::Value* emit_parameter_node(const std::shared_ptr<ast::parameter_node> &param);
        llvm::Value* emit_function_call_node(const std::shared_ptr<ast::function_call_node> &call);
        llvm::Value* emit_element_call_node(const std::shared_ptr<ast::element_call_node> &call);
        llvm::Value* emit_member_invoke_node(const std::shared_ptr<ast::member_invoke_node> &member);
        llvm::Value* emit_index_access_node(const std::shared_ptr<ast::index_access_node> &idx);
        llvm::Value* emit_index_assignment_node(const std::shared_ptr<ast::index_assignment_node> &idx_assign);
        llvm::Value* emit_if_node(const std::shared_ptr<ast::if_node> &ifstmt);
        llvm::Value* emit_while_node(const std::shared_ptr<ast::while_node> &whilestmt);
        llvm::Value* emit_switch_node(const std::shared_ptr<ast::switch_node> &sw);
        llvm::Value* emit_case_node(const std::shared_ptr<ast::case_node> &case_stmt);
        llvm::Value* emit_return_node(const std::shared_ptr<ast::return_node> &ret);
        llvm::Value* emit_break_node(const std::shared_ptr<ast::break_node> &brk);
        llvm::Value* emit_continue_node(const std::shared_ptr<ast::continue_node> &cont);
        llvm::Value* emit_increment_node(const std::shared_ptr<ast::increment_node> &inc);
        llvm::Value* emit_decrement_node(const std::shared_ptr<ast::decrement_node> &dec);
        llvm::Value* emit_body_node(const std::shared_ptr<ast::body_node> &body);
        llvm::Function* emit_function_node(const std::shared_ptr<ast::function_node> &func);
        llvm::Function* emit_function_prototype_node(const std::shared_ptr<ast::function_prototype_node> &proto);
        llvm::Value* emit_extern_node(const std::shared_ptr<ast::extern_node> &ext);

        llvm::Type* get_llvm_type(const variable_type &vtype);
        [[nodiscard]] llvm::Type* get_llvm_primitive_type(std::string_view base_type) const;
        [[nodiscard]] llvm::Function* get_named_function(std::string_view name) const;

        void push_scope();
        void pop_scope();
        llvm::Value* get_variable_value(std::string_view name);
        bool set_variable_value(std::string_view name, llvm::Value* value);

        std::unique_ptr<llvm::LLVMContext> m_context;
        std::unique_ptr<llvm::Module> m_module;
        std::unique_ptr<llvm::IRBuilder<>> m_builder;
        std::unique_ptr<llvm::Target> m_target;
        std::unique_ptr<llvm::TargetMachine> m_target_machine;

        std::vector<std::unordered_map<std::string, llvm::Value*>> m_symbol_stack;

        std::string m_target_triple;
    };

} // ent

#endif //CODEGEN_HH
