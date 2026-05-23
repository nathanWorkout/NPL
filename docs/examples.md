# Examples

All examples below run on the current interpreter.

---

## Hello World

```npl
>> "Hello, World!"
```

---

## Variables and arithmetic

```npl
x = 10
y = 3

>> x + y    # 13
>> x - y    # 7
>> x * y    # 30
>> x / y    # 3.333...
>> x % y    # 1
```

---

## String operations

```npl
use std/string

s = "  Hello, World!  "
>> len(s)                          # 17
>> trim(s)                         # Hello, World!
>> up(s)                           # HELLO, WORLD!
>> low(s)                          # hello, world!
>> has(s, "World")                 # true
>> replace(s, "World", "NPL")      #   Hello, NPL!  
>> split("hello,world,npl", ",")   # [hello, world, npl]
>> join(["Hello", "World"], " ")   # Hello World
```

---

## Arrays

```npl
arr = [10, 20, 30, 40, 50]

>> array_length(arr)              # 5
>> array_push(arr, 60)            # [10, 20, 30, 40, 50, 60]
>> array_pop(arr)                 # [10, 20, 30, 40]
>> array_last(arr)                # 50
>> array_slice(arr, 1, 3)         # [20, 30]
>> array_reverse(arr)             # [50, 40, 30, 20, 10]
```

---

## Maps

```npl
person = {"name": "Nathan", "age": 17, "city": "Caen"}

>> person["name"]    # Nathan
>> person["age"]     # 17

person["age"] = 18
>> person["age"]     # 18
```

---

## JSON

```npl
use json/json

# Stringify
obj = {"lang": "npl", "version": 1, "active": true}
>> stringify(obj)

# Parse
raw = "{\"name\": \"Nathan\", \"score\": 42}"
data = parse(raw)
>> data["name"]     # Nathan
>> data["score"]    # 42

# Nested
nested = {"user": {"name": "Nathan", "scores": [10, 20, 30]}}
>> stringify(nested)
```

---

## Functions

```npl
fn add(a, b) -> << a + b
fn square(n) -> << n * n

fn factorial(n) -> {
    if n <= 1 -> << 1
    << n * factorial(n - 1)
}

>> add(3, 5)         # 8
>> square(7)         # 49
>> factorial(6)      # 720
```

---

## Loops

```npl
# repeat
repeat 5 -> >> "hop"

# while
i = 0
while i < 5 -> {
    >> i
    i++
}

# for
for i = 0, i < 5, i++ -> {
    >> i * i
}
```

---

## Try / Catch

```npl
use json/json

try -> {
    data = parse("not valid json {{{")
    >> data["key"]
} catch err -> {
    >> "Caught: " + err
}
```
## Pipeline
```npl
nombres = [1, 2, 3, 4, 5]

# filter — garde les éléments qui passent la condition
nombres | filter { << item > 3 } | >>
# 4
# 5

# map — transforme chaque élément
nombres | map { << item * 10 } | >>
# 10
# 20
# 30
# 40
# 50

# chaînage — filter puis map
nombres | filter { << item > 2 } | map { << item * 10 } | >>
# 30
# 40
# 50
```



## Single neuron — gradient descent

```npl
w1 = 0.5
w2 = -0.3
b  = 0.1
x1 = 1.0
x2 = 0.5
y  = 1.0
lr = 0.1

fn sigmoid(z) -> {
    << 1.0 / (1.0 + math_exp(0 - z))
}

z = (w1 * x1) + (w2 * x2) + b
a = sigmoid(z)
E = (a - y) * (a - y)
>> "z = " + z
>> "a = " + a
>> "E = " + E
>> ""

repeat 1000 -> {
    z  = (w1 * x1) + (w2 * x2) + b
    a  = sigmoid(z)
    E  = (a - y) * (a - y)
    >> "E = " + E
    derive = 2 * (a - y) * (a * (1 - a)) * x1
    w1 = w1 - (lr * derive)
}

>> ""
>> "w = " + w1
```

---

## Date and time

```npl
use sys/time
  
ts = time()
>> ts   # 18/05/2026 21:07:19 (for my)
```