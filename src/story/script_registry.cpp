#include <story/script_registry.hpp>
#include <debug/logs.hpp>
#include <entities/npc.hpp>

namespace ScriptRegistry {
std::unordered_map<std::string, Script> scripts;
}

namespace FunctionRegistry {
    std::unordered_map<std::string, Function> functions;
    void addFunction(const std::string& name, Function func) {
        functions[name] = func;
    }
    void addFunctions(const std::vector<std::pair<std::string, Function>>& fn_list) {
        for (auto& [name, fn]: fn_list) {
            addFunction(name, fn);
        }
    }
}