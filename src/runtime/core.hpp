#ifndef RUNTIME_CORE_HPP
#define RUNTIME_CORE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

class Interpreter;
struct Value;

namespace runtime {

void register_core_functions(Interpreter& interp);

}

#endif
