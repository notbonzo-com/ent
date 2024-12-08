

#include "Codegen.hh"
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <ranges>

namespace ent {
    codegen::codegen(const std::string_view module_name)
        : m_context(std::make_unique<llvm::LLVMContext>()),
          m_module(std::make_unique<llvm::Module>(std::string(module_name), *m_context)),
          m_builder(std::make_unique<llvm::IRBuilder<>>(*m_context)) {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmPrinters();
        llvm::InitializeAllAsmParsers();

        const std::string target_triple = m_target_triple.empty() ? "x86_64-pc-linux-gnu" : m_target_triple;
        std::string error;
        const auto target_ptr = llvm::TargetRegistry::lookupTarget(target_triple, error);
        if (!target_ptr) {
            throw std::runtime_error("Failed to lookup target: " + error);
        }
        m_target = std::make_unique<llvm::Target>(*target_ptr);

        const llvm::TargetOptions opt;
        auto RM = llvm::Reloc::Model::PIC_;
        m_target_machine = std::unique_ptr<llvm::TargetMachine>(
            m_target->createTargetMachine(target_triple, "generic", "", opt, RM)
        );
        if (!m_target_machine) {
            throw std::runtime_error("Failed to create target machine");
        }

        m_module->setDataLayout(m_target_machine->createDataLayout());
        m_module->setTargetTriple(target_triple);

    }

    llvm::Module* codegen::get_module() const {
        return m_module.get();
    }

    void codegen::set_target_triple(const std::string_view triple) {
        m_target_triple = std::string(triple);
        m_module->setTargetTriple(m_target_triple);
    }

    void codegen::set_data_layout(const std::string_view layout) const {
        m_module->setDataLayout(layout.data());
    }

    bool codegen::write_ir_to_file(const std::string_view filename) const {
        std::error_code EC;
        llvm::raw_fd_ostream OS(filename.data(), EC, llvm::sys::fs::OF_None);
        if (EC) {
            return false;
        }
        m_module->print(OS, nullptr);
        return true;
    }

    bool codegen::write_ir_to_stream(std::ostream &os) const {
        std::string buffer;
        llvm::raw_string_ostream llvm_stream(buffer);
        m_module->print(llvm_stream, nullptr);
        os << llvm_stream.str();
        return true;
    }

    llvm::Function* codegen::get_named_function(const std::string_view name) const {
        return m_module->getFunction(name.data());
    }

    std::string codegen::mangle_name(const std::string_view base_name, const std::vector<variable_type>& args) {
        std::ostringstream mangled;
        mangled << "_E" << base_name.length() << base_name;

        for (const auto& arg : args) {
            mangled << mangle_type(arg);
        }
        return mangled.str();
    }

    std::string codegen::mangle_type(const variable_type& vtype) {
        if (vtype.pointer != 0) {
            auto new_vtype = vtype;
            --new_vtype.pointer;
            return "P" + mangle_type(new_vtype);
        }
        if (vtype.is_struct) {
            return "S" + std::to_string(vtype.base_type.length()) + vtype.base_type;
        }

        if (vtype.base_type == "void") return "v";
        if (vtype.base_type == "byte") return "b";
        if (vtype.base_type == "word") return "w";
        if (vtype.base_type == "dword") return "d";
        if (vtype.base_type == "qword") return "q";
        if (vtype.base_type == "sbyte") return "B";
        if (vtype.base_type == "sword") return "W";
        if (vtype.base_type == "sdword") return "D";
        if (vtype.base_type == "sqword") return "Q";

        return std::to_string(vtype.base_type.length()) + vtype.base_type; // Should be unreachable
    }

    llvm::Type* codegen::get_llvm_primitive_type(const std::string_view base_type) const {
        if (base_type == "byte" || base_type == "sbyte") {
            return llvm::Type::getInt8Ty(*m_context);
        }
        if (base_type == "word" || base_type == "sword") {
            return llvm::Type::getInt16Ty(*m_context);
        }
        if (base_type == "dword" || base_type == "sdword") {
            return llvm::Type::getInt32Ty(*m_context);
        }
        if (base_type == "qword" || base_type == "qdword") {
            return llvm::Type::getInt64Ty(*m_context);
        }
        return nullptr;
    }

    llvm::Type* codegen::get_llvm_type(const variable_type &vtype) {
        if (vtype.pointer != 0) {
            llvm::Type* base_type = get_llvm_primitive_type(vtype.base_type);
            if (!base_type) {
                base_type = llvm::StructType::getTypeByName(*m_context, vtype.base_type);
            }
            if (base_type) {
                return llvm::PointerType::getUnqual(base_type);
            }
        }

        if (vtype.is_struct) {
            const auto struct_type = llvm::StructType::create(*m_context, vtype.base_type);
            std::vector<llvm::Type*> struct_elements;

            for (const auto &val: vtype.struct_values | std::views::values) {
                llvm::Type* member_type = get_llvm_type(val);
                if (!member_type) {
                    return nullptr;
                }
                struct_elements.push_back(member_type);
            }

            struct_type->setBody(struct_elements, /*packed=*/false);
            return struct_type;
        }

        llvm::Type* type = get_llvm_primitive_type(vtype.base_type);
        if (!type) {
            llvm::errs() << "Unknown type: " << vtype.base_type << "\n";
        }
        return type;
    }

    void codegen::push_scope() {
        m_symbol_stack.emplace_back();
    }

    void codegen::pop_scope() {
        if (!m_symbol_stack.empty()) {
            m_symbol_stack.pop_back();
        }
    }

    llvm::Value* codegen::get_variable_value(const std::string_view name) {
        for (auto & it : std::ranges::reverse_view(m_symbol_stack)) {
            if (auto found = it.find(std::string(name)); found != it.end()) {
                return found->second;
            }
        }
        return nullptr;
    }

    bool codegen::set_variable_value(const std::string_view name, llvm::Value* value) {
        if (m_symbol_stack.empty()) {
            return false;
        }
        m_symbol_stack.back()[std::string(name)] = value;
        return true;
    }

    bool codegen::compile_to_object(const std::string_view filename) {
        std::error_code EC;
        llvm::raw_fd_ostream dest(filename.data(), EC, llvm::sys::fs::OF_None);
        if (EC) {
            llvm::errs() << "Could not open file: " << EC.message() << "\n";
            return false;
        }

        llvm::legacy::PassManager pass_manager;
        if (m_target_machine->addPassesToEmitFile(
            pass_manager, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
            llvm::errs() << "Target machine can't emit a file of this type\n";
            return false;
        }

        pass_manager.run(*m_module);
        dest.flush();

        llvm::outs() << "Object file written to " << filename << "\n";
        return true;
    }

}