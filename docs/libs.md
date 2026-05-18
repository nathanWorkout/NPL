# Libraries

---

## std/string

```npl
use std/string
```

| Function | Description | Example |
|---|---|---|
| `len(s)` | length of string | `len("hello")` → `5` |
| `up(s)` | uppercase | `up("hello")` → `"HELLO"` |
| `low(s)` | lowercase | `low("HELLO")` → `"hello"` |
| `trim(s)` | remove leading/trailing spaces | `trim("  hi  ")` → `"hi"` |
| `has(s, sub)` | check if substring exists | `has("hello", "ell")` → `true` |
| `starts(s, prefix)` | check prefix | `starts("hello", "hel")` → `true` |
| `ends(s, suffix)` | check suffix | `ends("hello", "llo")` → `true` |
| `replace(s, from, to)` | replace all occurrences | `replace("hello", "l", "r")` → `"herro"` |
| `split(s, delim)` | split into array | `split("a,b,c", ",")` → `["a","b","c"]` |
| `join(arr, delim)` | join array into string | `join(["a","b"], "-")` → `"a-b"` |
| `char_at(s, i)` | character at index | `char_at("hello", 0)` → `"h"` |
| `to_num(s)` | parse string to number | `to_num("42")` → `42` |
| `to_string(n)` | convert value to string | `to_string(42)` → `"42"` |

---

## std/array

```npl
use std/array
```

| Function | Description | Example |
|---|---|---|
| `array_length(arr)` | number of elements | `array_length([1,2,3])` → `3` |
| `array_push(arr, val)` | add element at end (returns new array) | `array_push([1,2], 3)` → `[1,2,3]` |
| `array_pop(arr)` | remove last element (returns new array) | `array_pop([1,2,3])` → `[1,2]` |
| `array_last(arr)` | last element | `array_last([1,2,3])` → `3` |
| `array_slice(arr, from, to)` | sub-array | `array_slice([1,2,3,4], 1, 3)` → `[2,3]` |
| `array_reverse(arr)` | reversed array | `array_reverse([1,2,3])` → `[3,2,1]` |

> Note: `array_push` and `array_pop` return a new array. They don't mutate the original.

---

## std/math

```npl
use std/math
```

| Function | Description |
|---|---|
| `abs(n)` | absolute value |
| `max(a, b)` | maximum |
| `min(a, b)` | minimum |
| `clamp(val, min, max)` | clamp value between min and max |
| `pow(base, exp)` | power (integer exponent) |
| `floor(n)` | round down |
| `ceil(n)` | round up |
| `round(n)` | round to nearest integer |
| `exp(x)` | e^x (calls native `math_exp`) |

---

## json/json

```npl
use json/json
```

| Function | Description |
|---|---|
| `parse(s)` | parse a JSON string into a NPL map/array |
| `stringify(map)` | convert a NPL map to a JSON string |
| `stringify_array(arr)` | convert a NPL array to a JSON string |

```npl
use json/json

# Parse
data = parse("{\"name\": \"Nathan\", \"age\": 17}")
>> data["name"]    # Nathan
>> data["age"]     # 17

# Stringify
obj = {"lang": "npl", "version": 1}
>> stringify(obj)    # {"lang": "npl", "version": 1}

# Roundtrip
original = {"x": 1, "y": 2}
json_str = stringify(original)
reparsed = parse(json_str)
>> reparsed["x"]    # 1
```

---

## sys/time

```npl
use sys/time
```

| Function | Description |
|---|---|
| `time()` | current Unix timestamp (seconds) |
| `get_date(ts)` | map with date/time fields |

`get_date` returns a map with the following keys: `day`, `month`, `year`, `hour`, `min`, `sec`.

```npl
use sys/time

ts = time()
date = get_date(ts)

>> date["day"] + "/" + date["month"] + "/" + date["year"]
>> date["hour"] + ":" + date["min"] + ":" + date["sec"]
```