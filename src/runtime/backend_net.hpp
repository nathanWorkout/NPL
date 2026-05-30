#ifndef RUNTIME_BACKEND_NET_HPP
#define RUNTIME_BACKEND_NET_HPP

#include <string>
#include <vector>
#include <memory>


class Interpreter;
struct Value;

namespace runtime {

void register_net_functions(Interpreter& interp);

}

#endif
