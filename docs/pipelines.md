# Pipeline Operator `|`

## What is it?

The pipeline operator `|` passes the result of the left side into the right side.

It's the same idea as Linux pipes — `ls | grep .txt` feeds the output of `ls` into `grep`. In NPL it works the same way, but on arrays.

```npl
nombres = [1, 2, 3, 4, 5]
nombres | filter { << item > 3 } | >>
# 4
# 5
```

At each `|`, the current array becomes available as `item` inside the `{ }` block on the right.

---

## Core concept

```
[1, 2, 3, 4, 5]
    |
filter { << item > 3 }   →   [4, 5]
    |
>>                       →   prints 4 then 5
```

The data flows left to right. Each stage receives the output of the previous one.

---

## Built-in pipeline operations

### `filter { << condition }`
Keeps elements where the condition returns `true`.

```npl
nombres = [1, 2, 3, 4, 5]
nombres | filter { << item > 3 } | >>
# 4
# 5
```

### `each { << expression }`
Transforms each element into a new value.

```npl
nombres = [1, 2, 3, 4, 5]
nombres | each { << item * 2 } | >>
# 2
# 4
# 6
# 8
# 10
```

### `>>` (output)
Prints each element of the array.

```npl
nombres = [1, 2, 3]
nombres | >>
# 1
# 2
# 3
```

---

## Chaining

You can chain as many stages as you want:

```npl
nombres = [1, 2, 3, 4, 5]
nombres | filter { << item > 2 } | each { << item * 10 } | >>
# 30
# 40
# 50
```

Step by step:
```
[1, 2, 3, 4, 5]
    | filter { item > 2 }   →   [3, 4, 5]
    | each { item * 10 }     →   [30, 40, 50]
    | >>                    →   prints 30, 40, 50
```

---

## Working with each

```npl
users = [
    {"name": "Alice", "age": 25},
    {"name": "Bob",   "age": 15},
    {"name": "Carol", "age": 32}
]

users | filter { << item["age"] > 18 } | each { << item["name"] } | >>
# Alice
# Carol
```

---

## How it's implemented

The pipeline is implemented in three parts: the lexer, the parser, and the interpreter.

### 1. Lexer (`lexer.cpp`)
`|` is added to the operators list so it gets tokenized as `OPERATOR`.

### 2. AST (`ast.hpp`)
Two new nodes:

```cpp
struct PipeExpr : ASTNode {
    std::unique_ptr<ASTNode> lhs;  // left side  (the array)
    std::unique_ptr<ASTNode> rhs;  // right side (filter, each, >>)
};

struct LambdaBlock : ASTNode {
    std::unique_ptr<ASTNode> body; // the { } block, with access to `item`
};
```

### 3. Parser (`parser.cpp`)
`parse_pipeline()` reads the left side, then loops on every `|` it finds. For each one it builds a `PipeExpr` node. The right side is either:
- a `LambdaBlock` if it starts with `{`
- a `FuncCall` (filter each) followed by a `LambdaBlock`
- an `Output` node if it sees `>>`

When a line starts with `identifier |`, `parse_statement()` wraps it in an `ExprStatement` instead of treating it as an assignment.

### 4. Interpreter (`interpreter.hpp`)
`eval_pipe()` evaluates the left side to get the array, then looks at what's on the right:

- `>>` → prints each element
- `filter { }` → iterates the array, injects `item` into a temporary scope, runs the block, keeps the element if `<<` returned `true`
- `each { }` → same but always keeps the result of `<<`

```cpp
for(auto& item : left.arr) {
    push_scope();
    def_var("item", item);         // inject `item`
    try { execute(lambda->body); }
    catch(ReturnException& ret) { cond = ret.value; }  // catch <<
    if(cond.truthy()) result.push_back(item);          // filter decision
}
```

The key mechanism: `<<` inside a pipeline block throws a `ReturnException` which is caught by the interpreter to get the returned value, exactly like a function return.