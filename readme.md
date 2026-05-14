# NPL - Nathan Programming Language

## Variables
```npl
nom = nathan
age = 17.5
string = "Welcome to npl !"
actif = true
```

## Output
```npl
>> nom
>> age
>> "Welcome to npl !"
```

## Conditions
```npl
if age > 18 -> >> "majeur"
elif age == 18 -> >> "ok"
else -> >> "mineur"
```

## Boucles
```npl
repeat 10 -> >> "it"


while actif -> >> "run"


for i = 0; i < 10; i++ -> >> i

```

## Fonctions
```npl
fn add(a, b) : a + b
>> add(5, 3)
```

## Pipeline
```npl
users | filter(adult) | map(name) | print
```

## Pointeurs
```npl
ptr = &age
val = *ptr
>> val
```

## Erreurs
```npl
try {
    div(10, 0)
} catch e {
    >> e
}
```

// A faire : commentaires, if prenom = "nathan" && age = 18 par exemple, les float...