# NPL — Nathan Programming Language

NPL is a programming language designed to drastically reduce the amount of code needed for web development. Originally built for power users tired of verbose languages like Java, it evolved into an ultra-concise syntax for backend, servers, databases, and HTML/CSS generation.

NPL's syntax is designed to cover 95% of web use cases. It assumes it won't cover everything — that's expected for a language of this type.

Comparing NPL with Node.js/Express on real projects, NPL reduces code by approximately **x7 for backend** and **x10 for HTML/CSS**.

---

## Quick syntax

| Symbol | Meaning |
|---|---|
| `>>` | print |
| `<<` | return |
| `?>` | read input |
| `->` | block body |
| `use` | import a lib |

```npl
fn add(a, b) -> << a + b

if x > 0 -> >> "positive"
elif x == 0 -> >> "zero"
else -> >> "negative"

repeat 5 -> >> "hop"

while x < 10 -> {
    >> x
    x++
}

try -> {
    result = parse(data)
} catch err -> {
    >> "Error: " + err
}
```

---

## Working examples

**Single neuron — gradient descent**

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

fn get_loss(pred, target) -> {
    error = pred - target
    << error * error
}

repeat 1000 -> {
    z = (w1 * x1) + (w2 * x2) + b
    a = sigmoid(z)
    
    E = get_loss(a, y)
    >> "Loss = " + E
    
    dz = 2 * (a - y) * (a * (1.0 - a))
    
    dw1 = dz * x1
    dw2 = dz * x2
    db  = dz * 1.0
    
    w1 = w1 - (lr * dw1)
    w2 = w2 - (lr * dw2)
    b  = b - (lr * db)
}

>> ""
>> "w1 final = " + w1
>> "w2 final = " + w2
>> "b final  = " + b
>> "Final prediction : = " + sigmoid((w1 * x1) + (w2 * x2) + b)

```

**std/string**

```npl
use std/string

s = "  Hello, World!  "
>> str_len(s)                      # 17
>> trim(s)                         # Hello, World!
>> up(s)                           # HELLO, WORLD!
>> has(s, "World")                 # true
>> replace(s, "World", "NPL")      #   Hello, NPL!  
>> split("hello,world,npl", ",")   # [hello, world, npl]
>> join(["Hello", "World"], " ")   # Hello World
```

---

## Installation

```bash
git clone https://github.com/nathanWorkout/NPL
cd NPL
make
sudo cp ./npl /usr/local/bin/npl
sudo mkdir -p /usr/local/lib
sudo ln -s /home/nathan/Dev/NathanProgrammingLangage/libs /usr/local/lib/npl
```

```bash
echo '>> "Hello, World!"' > hello.npl
npl hello.npl
```

Tested on Linux (Arch/Artix). Requires g++ C++23 and make.

---

## Roadmap

```
✅ C++ interpreter
✅ Variables, functions, loops, conditions
✅ Arrays, Maps, JSON
✅ Try/catch/throw
✅ std/string, std/array, std/math, std/json, sys/time
✅ Module system (use)

🚧 Pipeline operator |
⬜ io/  — file read/write
⬜ sys/ — env, process
⬜ net/ — UDP, socket, SSE
⬜ http/ — HTTP server
⬜ db/  — JSON flat-file storage
⬜ web/ — HTML/CSS DSL
```

---

## About

Self-taught 17-year-old developer with a passion for low-level programming. I built NPL because I found backend code too verbose and not enjoyable to write.

My concrete goal: use NPL during my BTS SIO studies to write less code for the same result.

This is a bootstrapped project. I discover and fix an average of 3-4 bugs per session — that's the daily routine and I'm not stopping.
