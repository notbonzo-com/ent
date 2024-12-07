//
// Created by notbonzo on 11/23/24.
//
#include "AST.icc"
#include <format>

namespace ent::ast {
    base_node::~base_node() = default;
    void base_node::print(int indent) const {
        std::println("Base node (default print): Type = {}", static_cast<int>(m_type));
    }
}