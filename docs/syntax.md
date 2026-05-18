# Syntax

## Variables

No declaration keyword. Assignment creates the variable.

```npl
x = 42
name = "Nathan"
flag = true
nothing = null
```

---

## Types

| Type | Example |
|---|---|
| Number | `42`, `3.14`, `-7` |
| String | `"hello"` |
| Bool | `true`, `false` |
| Array | `[1, 2, 3]` |
| Map | `{"key": "value"}` |
| Null | `null` |

```npl
arr = [1, 2, 3]
person = {"name": "Nathan", "age": 17}
```

---

## Output

```npl
>> "Hello, World!"
>> 42
>> "result: " + x
```

---

## Input

```npl
name ?> "Enter your name: "
>> "Hello, " + name
```

---

## Operators

```npl
# Arithmetic
x = 10 + 5
x = 10 - 5
x = 10 * 5
x = 10 / 5
x = 10 % 3

# Comparison
x == y
x != y
x > y
x < y
x >= y
x <= y

# Logical
x && y
x || y

# Increment / Decrement
i++
i--

# String concatenation
s = "Hello" + " World"
s = "result: " + 42    # number is automatically converted
```

---

## Conditions

```npl
if x > 0 -> >> "positive"

if x > 0 -> {
    >> "positive"
}
elif x == 0 -> {
    >> "zero"
}
else -> {
    >> "negative"
}
```

---

## Loops

**repeat** — fixed number of iterations

```npl
repeat 5 -> >> "hop"

repeat 10 -> {
    >> i
}
```

**while** — condition-based loop

```npl
i = 0
while i < 10 -> {
    >> i
    i++
}
```

**for** — classic C-style loop

```npl
for i = 0, i < 10, i++ -> {
    >> i
}
```

---

## Functions

```npl
fn add(a, b) -> << a + b

fn greet(name) -> {
    msg = "Hello, " + name
    << msg
}

result = add(3, 5)
>> result
```

`<<` is the return keyword.

Single-line functions don't need braces.

---

## Arrays

```npl
arr = [10, 20, 30]

# Access by index
>> arr[0]    # 10

# Modify
arr[1] = 99

# Iterate
i = 0
while i < array_length(arr) -> {
    >> arr[i]
    i++
}
```

---

## Maps

```npl
person = {"name": "Nathan", "age": 17}

# Access
>> person["name"]

# Modify
person["age"] = 18

# Add a key
person["city"] = "Caen"
```

---

## Try / Catch / Throw

```npl
try -> {
    result = parse(raw)
} catch err -> {
    >> "Error: " + err
}

# Throw manually
throw "something went wrong"
```

---

## Modules

```npl
use std/string
use std/array
use std/math
use json/json
use sys/time
```

---

## Comments

```npl
# This is a comment
x = 42    # inline comment
```