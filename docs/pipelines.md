# Pipeline Operator `|`

## What is it?

The pipeline operator `|` passes the result of the left side into the right side.

It's the same idea as Linux pipes — `ls | grep .txt` feeds the output of `ls` into `grep`. In NPL it works the same way, but on arrays and values.

```npl
nombres = [1, 2, 3, 4, 5]

nombres | filter { << item > 3 } | >>

# 4
# 5
```

At each `|`, the current array becomes available as `item` inside the `{ }` block on the right.

---

# Universal pipeline (values + functions)

Since recent NPL versions, the pipeline works with any value (numbers, strings, maps) and any function call — not just arrays with `filter` / `each`.

```npl
"  hello  " | trim() | up() | >>    # "HELLO"

10 | triple() | >>                  # 30
```

When you write:

```npl
value | func()
```

NPL automatically calls:

```npl
func(value)
```

If the function takes extra arguments:

```npl
value | func(a, b)
```

becomes:

```npl
func(value, a, b)
```

Example:

```npl
fn add(a, b) -> << a + b

5 | add(3) | >>    # 8
```

Equivalent to:

```npl
add(5, 3)
```

---

## Core concept

```text
10
 |
triple()      →   triple(10) → 30
 |
>>            →   prints 30
```

The data flows left to right. Each stage receives the output of the previous one.

---

# Built-in pipeline operations

## `filter { << condition }`

Keeps elements where the condition returns `true`.

Works only on arrays.

```npl
nombres = [1, 2, 3, 4, 5]

nombres | filter { << item > 3 } | >>

# 4
# 5
```

---

## `each { << expression }`

Transforms each element into a new value.

Works only on arrays.

```npl
nombres = [1, 2, 3, 4, 5]

nombres | each { << item * 2 } | >>

# 2
# 4
# 6
# 8
# 10
```

---

## `>>` (output)

Prints the value.

If the value is an array, prints each element on its own line.

```npl
"Hello" | >>          # Hello

[1, 2, 3] | >>        # 1
                       # 2
                       # 3
```

---

# Chaining

You can chain as many stages as you want.

```npl
nombres = [1, 2, 3, 4, 5]

nombres
| filter { << item > 2 }
| each { << item * 10 }
| >>

# 30
# 40
# 50
```

Step by step:

```text
[1, 2, 3, 4, 5]
    | filter { item > 2 }   →   [3, 4, 5]
    | each { item * 10 }    →   [30, 40, 50]
    | >>                    →   prints 30, 40, 50
```

---

# Working with maps

```npl
users = [
    {"name": "Alice", "age": 25},
    {"name": "Bob",   "age": 15},
    {"name": "Carol", "age": 32}
]

users
| filter { << item["age"] > 18 }
| each { << item["name"] }
| >>

# Alice
# Carol
```

---

# Real-world examples

## String processing

```npl
"  hello world  "
| trim()
| up()
| replace("WORLD", "NPL")
| >>

# HELLO NPL
```

---

## Date formatting

```npl
use sys/time

time()
| get_date()
| >>

# {day: 27, month: 5, year: 2026, ...}
```

---

## File reading pipeline

```npl
use io/file

"data.txt"
| read()
| split("\n")
| filter { << len(item) > 0 }
| each { << "> " + item }
| >>

# > line1
# > line2
# > line3
```

---

## Custom functions

```npl
fn triple(x) -> << x * 3
fn inc(x) -> << x + 1

2 | triple() | inc() | >>

# 7
```

Explanation:

```text
2
→ triple(2)
→ 6
→ inc(6)
→ 7
```

---

## Mixed pipelines (array + functions)

```npl
[1, 2, 3, 4, 5]
| filter { << item > 2 }
| each { << item * 10 }
| >>

# 30
# 40
# 50
```

---

# How it's implemented

The pipeline is implemented in three parts:

- lexer
- parser
- interpreter

---

## 1. Lexer (`lexer.cpp`)

`|` is added to the operators list so it gets tokenized as `OPERATOR`.

---

## 2. AST (`ast.hpp`)

Two new nodes:

```cpp
struct PipeExpr : ASTNode {
    std::unique_ptr<ASTNode> lhs;
    std::unique_ptr<ASTNode> rhs;
};

struct LambdaBlock : ASTNode {
    std::unique_ptr<ASTNode> body;
};
```

---

## 3. Parser (`parser.cpp`)

`parse_pipeline()` reads the left side, then loops on every `|`.

For each one it builds a `PipeExpr`.

The right side can be:

- a `LambdaBlock` if it starts with `{`
- a `FuncCall`
- an `Output` node if it sees `>>`

Examples of valid pipeline stages:

```npl
value | func()
value | func(a, b)
value | filter { ... }
value | each { ... }
value | >>
```

`parse_statement()` also detects pipelines starting from arrays/maps directly:

```npl
[1,2,3] | >>
{"x":1} | >>
```

---

## 4. Interpreter (`interpreter.hpp`)

`eval_pipe()` evaluates the left side, then checks the right side type.

### `>>`

Prints the value.

If it's an array, prints each element.

---

### `filter { }`

- array only
- injects `item`
- keeps values where the lambda returns `true`

---

### `each { }`

- array only
- injects `item`
- collects transformed values

---

### Universal pipeline (`func()`)

For:

```npl
value | func(a, b)
```

the interpreter internally builds:

```cpp
func(value, a, b)
```

Implementation idea:

```cpp
std::vector<Value> args_values;

args_values.push_back(left);

for(auto& arg : call->args) {
    args_values.push_back(eval(arg.get()));
}

return func(args_values);
```

The piped value is always injected as the first argument.

---

## Return mechanism

`<<` inside pipeline blocks throws a `ReturnException`.

The interpreter catches it to retrieve the returned value.

Same mechanism as normal function returns.

Example:

```cpp
try {
    execute(lambda->body);
}
catch(ReturnException& ret) {
    result = ret.value;
}
```

---

# Why use pipelines?

## Readable

Data flow is explicit left → right.

---

## Composable

Build complex transformations by chaining simple operations.

---

## No nesting hell

Instead of:

```npl
f(g(h(x)))
```

write:

```npl
x | h() | g() | f()
```

---

## Universal

Works with:

- arrays
- strings
- numbers
- maps
- custom functions

---

## Functions as stages

Any function can become a pipeline stage.

---

# Summary table

| Syntax | Meaning |
|---|---|
| `value | >>` | print the value |
| `array | >>` | print each element |
| `array | filter { << item > n }` | keep matching elements |
| `array | each { << expr }` | transform elements |
| `value | func()` | call `func(value)` |
| `value | func(a, b)` | call `func(value, a, b)` |
| `value | func() | g() | >>` | chain multiple transformations |

---

# Final note

NPL pipelines are now truly universal.

They work with:

- arrays
- scalars
- strings
- maps
- functions
- custom transformations

Everything can flow through pipelines.