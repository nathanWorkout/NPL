# NPL - Nathan Programming Language
> A fast-to-write language for power users who don't want to deal with Java's verbosity.

NPL is a lightweight interpreted language written in C++. It uses symbols over keywords to keep code short and expressive — built for people who live in the terminal and want to write fast.

---

## Installation
```bash
git clone https://github.com/yourname/NPL
cd NPL
make
sudo cp ./npl /usr/local/bin/npl
```

---

## Usage
```bash
npl yourfile.npl
```

---

## Syntax

### Variables
```npl
x = 10
name = "Nathan"
active = true
arr = [1, 2, 3]
```

### Output
```npl
>> x
>> "Hello world"
```

### Input
```npl
x = ?> "Enter a number: "
```

### Conditions
Two modes — inline or block:
```npl
# inline
if x > 5 -> >> "big"

# block
if x > 5 -> {
    >> "big"
    >> x
}

if x > 10 -> >> "very big"
elif x > 5 -> >> "medium"
else -> >> "small"
```

### Loops
Two modes — inline or block:
```npl
# inline
repeat 3 -> >> "hello"
for j = 0; j < 5; j++ -> >> j

# block
repeat 3 -> {
    >> "hello"
    >> x
}

i = 0
while i < 5 -> {
    >> i
    i = i + 1
}

for k = 0; k < 3; k++ -> {
    >> "k ="
    >> k
}
```

### Functions
Two modes — inline or block:
```npl
# inline
fn add(a, b) -> << a + b

# block
fn div(a, b) -> {
    if b == 0 -> >> "Error: divide by zero"
    else -> << a / b
}

result = add(2, 4)
>> result
```

- `<<` returns a value
- `>>` prints a value
- `?>` reads input from the user

### Arrays
```npl
arr = [1, 2, 3]
>> arr[0]
arr[1] = 99
>> arr
```

### Maps
```npl
m = {"name": "Alice", "age": 30}
>> m["name"]        # -> Alice
>> m["age"]         # -> 30
m["age"] = 31
>> m["age"]         # -> 31
```

### Import
```npl
use std/string
use std/math
use std/array
```

---

## Example — Calculator
```npl
fn add(a, b) -> << a + b
fn sub(a, b) -> << a - b
fn mult(a, b) -> << a * b
fn div(a, b) -> {
    if b == 0 -> >> "Error: divide by zero"
    else -> << a / b
}

a = ?> "First number: "
b = ?> "Second number: "
>> add(a, b)
>> sub(a, b)
>> mult(a, b)
>> div(a, b)
```

---

## Types

| Type    | Example         |
|---------|-----------------|
| Number  | `42`, `3.14`    |
| String  | `"hello"`       |
| Bool    | `true`, `false` |
| Array   | `[1, 2, 3]`     |
| Map     | `{"key": val}` |

---

## Standard Library

NPL ships with a standard library under `libs/`. Use `use <lib>` to import.

### std/string
String manipulation functions.
```npl
use std/string

s = "  Hello, World!  "
>> len(s)               # length -> 17
>> trim(s)              # remove spaces -> Hello, World!
>> up(s)                # uppercase -> HELLO, WORLD!
>> low(s)               # lowercase ->   hello, world!
>> has(s, "World")      # contains -> true
>> starts(s, "  Hell")  # starts with -> true
>> ends(s, "!  ")       # ends with -> true
>> replace(s, "World", "NPL")       # ->   Hello, NPL!
>> split("a,b,c", ",")              # -> [a, b, c]
>> join(["Hello", "World"], " ")    # -> Hello World
```

| Function | Description |
|----------|-------------|
| `len(s)` | Length of string |
| `trim(s)` | Remove leading/trailing spaces |
| `up(s)` | Convert to uppercase |
| `low(s)` | Convert to lowercase |
| `has(s, sub)` | True if sub is in s |
| `starts(s, sub)` | True if s starts with sub |
| `ends(s, sub)` | True if s ends with sub |
| `replace(s, from, to)` | Replace from by to |
| `split(s, delim)` | Split string into array |
| `join(arr, delim)` | Join array into string |

---

### std/math
Mathematical functions.
```npl
use std/math

>> abs(-5)          # -> 5
>> max(3, 7)        # -> 7
>> min(3, 7)        # -> 3
>> clamp(15, 0, 10) # -> 10
>> pow(2, 8)        # -> 256
>> floor(3.7)       # -> 3
>> ceil(3.2)        # -> 4
>> round(3.5)       # -> 4
```

| Function | Description |
|----------|-------------|
| `abs(n)` | Absolute value |
| `max(a, b)` | Larger of two values |
| `min(a, b)` | Smaller of two values |
| `clamp(val, min, max)` | Clamp val between min and max |
| `pow(base, exp)` | base to the power of exp |
| `floor(n)` | Round down |
| `ceil(n)` | Round up |
| `round(n)` | Round to nearest |

---

### std/array
Array manipulation functions.
```npl
use std/array

arr = [3, 1, 4, 1, 5]
>> len(arr)             # -> 5
>> first(arr)           # -> 3
>> last(arr)            # -> 5
>> contains(arr, 4)     # -> true
>> sum(arr)             # -> 14
>> reverse(arr)         # -> [5, 1, 4, 1, 3]

arr = push(arr, 9)      # -> [3, 1, 4, 1, 5, 9]
arr = pop(arr)          # -> [3, 1, 4, 1, 5]
arr = slice(arr, 1, 3)  # -> [1, 4]
```

| Function | Description |
|----------|-------------|
| `len(arr)` | Number of elements |
| `first(arr)` | First element |
| `last(arr)` | Last element |
| `push(arr, val)` | Add val at end, returns new array |
| `pop(arr)` | Remove last element, returns new array |
| `contains(arr, val)` | True if val is in array |
| `reverse(arr)` | Reverse the array |
| `slice(arr, from, to)` | Sub-array from index to index (excluded) |
| `sum(arr)` | Sum of all numbers |


---

## License
GPL - open source, keep it open source.