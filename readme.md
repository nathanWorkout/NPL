# NPL — Nathan Programming Language

> Un langage pensé pour aller vite.

NPL est un langage interprété léger écrit en C++. Il utilise des symboles plutôt que des mots-clés pour garder le code court et expressif.

**Ses avantages :**
- **Concis** — moins de caractères à écrire, moins de bruit syntaxique
- **Intuitif** — `>>` pour afficher, `<<` pour retourner, `?>` pour lire. Les symboles parlent d'eux-mêmes
- **Vibe coding friendly** — moins de tokens consommés, moins de boilerplate à générer
- **Lisible** — un bloc NPL tient souvent en quelques lignes là où Python ou JS en demanderaient dix

---

## Installation

```bash
git clone https://github.com/nathanWorkout/NPL
cd NPL
make
sudo cp ./npl /usr/local/bin/npl
```

## Usage

```bash
npl yourfile.npl
```

---

## Syntaxe

### Variables

```npl
x = 10
name = "Nathan"
active = true
arr = [1, 2, 3]
m = {"key": "value"}
```

### Entrée / Sortie / Retour

| Symbole | Rôle |
|---------|------|
| `>>` | Affiche une valeur |
| `?>` | Lit une entrée utilisateur |
| `<<` | Retourne une valeur depuis une fonction |

```npl
>> "Hello world"
x = ?> "Entrez un nombre : "
```

### Conditions

```npl
# inline
if x > 5 -> >> "grand"

# bloc
if x > 10 -> {
    >> "très grand"
} elif x > 5 -> {
    >> "moyen"
} else -> {
    >> "petit"
}
```

### Boucles

```npl
# repeat
repeat 3 -> >> "hello"

# while
i = 0
while i < 5 -> {
    >> i
    i++
}

# for
for k = 0; k < 3; k++ -> {
    >> k
}
```

### Fonctions

```npl
# inline
fn add(a, b) -> << a + b

# bloc
fn div(a, b) -> {
    if b == 0 -> throw "Division par zéro"
    << a / b
}

result = add(2, 4)
>> result   # 6
```

### Arrays

```npl
arr = [1, 2, 3]
>> arr[0]       # 1
arr[1] = 99
>> arr          # [1, 99, 3]
```

### Maps

```npl
m = {"name": "Alice", "age": 30}
>> m["name"]    # Alice
m["age"] = 31
>> m["age"]     # 31
```

### Try / Catch / Throw

```npl
try -> {
    >> div(10, 0)
} catch err -> {
    >> "Erreur : " + err   # Erreur : Division par zéro
}
```

`throw` lance une erreur manuellement. `catch` la récupère dans la variable `err`.

### Import

```npl
use std/string
use std/math
use std/array
use json/json
```

---

## Types

| Type | Exemple |
|------|---------|
| Number | `42`, `3.14`, `-7` |
| String | `"hello"`, `'world'` |
| Bool | `true`, `false` |
| Array | `[1, 2, 3]` |
| Map | `{"key": val}` |
| Null | `null` |

---

## Opérateurs

| Opérateur | Description |
|-----------|-------------|
| `+` `-` `*` `/` `%` | Arithmétique |
| `==` `!=` `<` `>` `<=` `>=` | Comparaison |
| `&&` `\|\|` | Logique (court-circuit) |
| `++` `--` | Incrémentation |
| `->` | Corps de bloc |
| `<<` | Retour de valeur |
| `>>` | Affichage |
| `?>` | Lecture utilisateur |

---

## Bibliothèque Standard

### `std/string`

```npl
use std/string

s = "  Hello, World!  "

>> len(s)                          # 17
>> trim(s)                         # "Hello, World!"
>> up(s)                           # "  HELLO, WORLD!  "
>> low(s)                          # "  hello, world!  "
>> has(s, "World")                 # true
>> starts(s, "  Hell")             # true
>> ends(s, "!  ")                  # true
>> replace(s, "World", "NPL")      # "  Hello, NPL!  "
>> split("a,b,c", ",")             # [a, b, c]
>> join(["Hello", "World"], " ")   # "Hello World"
>> char_at("hello", 1)             # "e"
>> to_num("42")                    # 42
>> to_string(42)                   # "42"
```

| Fonction | Description |
|----------|-------------|
| `len(s)` | Longueur de la string |
| `trim(s)` | Supprime les espaces en début/fin |
| `up(s)` | Majuscules |
| `low(s)` | Minuscules |
| `has(s, sub)` | Vrai si `sub` est dans `s` |
| `starts(s, sub)` | Vrai si `s` commence par `sub` |
| `ends(s, sub)` | Vrai si `s` finit par `sub` |
| `replace(s, from, to)` | Remplace toutes les occurrences |
| `split(s, delim)` | Découpe en tableau |
| `join(arr, delim)` | Joint un tableau en string |
| `char_at(s, i)` | Caractère à la position `i` |
| `to_num(s)` | Convertit une string en nombre |
| `to_string(n)` | Convertit un nombre en string |

---

### `std/math`

```npl
use std/math

>> abs(-5)           # 5
>> max(3, 7)         # 7
>> min(3, 7)         # 3
>> clamp(15, 0, 10)  # 10
>> pow(2, 8)         # 256
>> floor(3.7)        # 3
>> ceil(3.2)         # 4
>> round(3.5)        # 4
```

| Fonction | Description |
|----------|-------------|
| `abs(n)` | Valeur absolue |
| `max(a, b)` | Maximum de deux valeurs |
| `min(a, b)` | Minimum de deux valeurs |
| `clamp(val, min, max)` | Borne `val` entre `min` et `max` |
| `pow(base, exp)` | `base` à la puissance `exp` |
| `floor(n)` | Arrondi inférieur |
| `ceil(n)` | Arrondi supérieur |
| `round(n)` | Arrondi au plus proche |

---

### `std/array`

```npl
use std/array

arr = [3, 1, 4, 1, 5]

>> array_length(arr)          # 5
>> array_last(arr)            # 5
>> array_reverse(arr)         # [5, 1, 4, 1, 3]

arr = array_push(arr, 9)      # [3, 1, 4, 1, 5, 9]
arr = array_pop(arr)          # [3, 1, 4, 1, 5]
arr = array_slice(arr, 1, 3)  # [1, 4]
```

| Fonction | Description |
|----------|-------------|
| `array_length(arr)` | Nombre d'éléments |
| `array_push(arr, val)` | Ajoute `val` à la fin, retourne le nouveau tableau |
| `array_pop(arr)` | Supprime le dernier élément, retourne le nouveau tableau |
| `array_slice(arr, from, to)` | Sous-tableau de `from` à `to` (exclu) |
| `array_reverse(arr)` | Inverse le tableau |
| `array_last(arr)` | Dernier élément |

---

### `json/json`

```npl
use json/json

# Sérialisation : map NPL → string JSON
m = {"name": "Alice", "age": 30}
s = stringify(m)
>> s   # {"name": "Alice", "age": 30}

# Désérialisation : string JSON → map NPL
parsed = parse('{"name": "Alice", "age": 30}')
>> parsed["name"]   # Alice
>> parsed["age"]    # 30

# Types supportés
>> parse('true')         # true
>> parse('null')         # null
>> parse('42.5')         # 42.5
>> parse('"hello"')      # hello
>> parse('[1, 2, 3]')    # [1, 2, 3]

# Objets imbriqués
nested = parse('{"user": {"name": "Bob", "age": 25}}')
user = nested["user"]
>> user["name"]   # Bob
>> user["age"]    # 25

# Round-trip
m = {"x": 1, "y": 2}
>> parse(stringify(m))   # restitue le map original
```

| Fonction | Description |
|----------|-------------|
| `stringify(map)` | Convertit un map NPL en string JSON |
| `parse(s)` | Convertit une string JSON en valeur NPL |

---

## Exemple complet — Calculatrice

```npl
fn add(a, b)  -> << a + b
fn sub(a, b)  -> << a - b
fn mult(a, b) -> << a * b
fn div(a, b)  -> {
    if b == 0 -> throw "Division par zéro"
    << a / b
}

a = ?> "Premier nombre : "
b = ?> "Deuxième nombre : "

>> add(a, b)
>> sub(a, b)
>> mult(a, b)

try -> {
    >> div(a, b)
} catch err -> {
    >> "Erreur : " + err
}
```

---

## License

GPL — open source, keep it open source.