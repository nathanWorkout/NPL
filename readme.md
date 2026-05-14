# NPL — Nathan Programming Language

> A fast-to-write language for power users who don't want to deal with C's verbosity.

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

---

## License

GPL — open source, keep it open source.