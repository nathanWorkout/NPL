# NPL — Nathan Programming Language

NPL est un langage de programmation conçu pour réduire drastiquement la quantité de code nécessaire au développement web. Initialement créé pour les utilisateurs avancés fatigués des langages verbeux comme Java, il a évolué vers une syntaxe ultra concise pour le backend, les serveurs, les bases de données et la génération HTML/CSS.

La syntaxe de NPL est pensée pour couvrir 95 % des cas d’usage du web. Il est assumé qu’il ne couvrira pas tout — c’est attendu pour un langage de ce type.

En comparant NPL avec Node.js/Express sur de vrais projets, NPL réduit la quantité de code d’environ **x7 pour le backend** et **x10 pour le HTML/CSS**.

---

## Syntaxe rapide

| Symbole | Signification |
|---|---|
| `>>` | afficher |
| `<<` | retourner |
| `?>` | lire une entrée |
| `->` | corps de bloc |
| `use` | importer une librairie |

```npl
fn add(a, b) -> << a + b

if x > 0 -> >> "positif"
elif x == 0 -> >> "zéro"
else -> >> "négatif"

repeat 5 -> >> "hop"

while x < 10 -> {
    >> x
    x++
}

try -> {
    result = parse(data)
} catch err -> {
    >> "Erreur : " + err
}
```

---

## Exemples fonctionnels

### Neurone unique — descente de gradient

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

### std/string

```npl
use std/string

s = "  Bonjour, Monde !  "
>> len(s)                           # 19
>> trim(s)                          # Bonjour, Monde !
>> up(s)                            # BONJOUR, MONDE !
>> has(s, "Monde")                  # true
>> replace(s, "Monde", "NPL")       #   Bonjour, NPL !  
>> split("bonjour,monde,npl", ",")  # [bonjour, monde, npl]
>> join(["Bonjour", "Monde"], " ")  # Bonjour Monde
```

---

## Installation

```bash
git clone https://github.com/nathanWorkout/NPL
cd NPL
make
sudo cp ./npl /usr/local/bin/npl
sudo ln -s $(pwd)/libs /usr/local/lib/npl
```

```bash
echo '>> "Bonjour, Monde !"' > hello.npl
npl hello.npl
```

Testé sur Linux (Arch/Artix). Nécessite g++ C++23 et make.

---

## Roadmap

```txt
✅ Interpréteur C++
✅ Variables, fonctions, boucles, conditions
✅ Tableaux, Maps, JSON
✅ Try/catch/throw
✅ std/string, std/array, std/math, std/json, sys/time
✅ Système de modules (use)

🚧 Opérateur pipeline |
⬜ io/  — lecture/écriture de fichiers
⬜ sys/ — environnement, processus
⬜ net/ — UDP, socket, SSE
⬜ http/ — serveur HTTP
⬜ db/  — stockage JSON flat-file
⬜ web/ — DSL HTML/CSS
```

---

## À propos

Développeur autodidacte de 17 ans passionné par la programmation bas niveau. J’ai créé NPL parce que je trouvais le code backend trop verbeux et peu agréable à écrire.

Mon objectif concret : utiliser NPL pendant mon BTS SIO afin d’écrire moins de code pour le même résultat.

C’est un projet bootstrapé. Je découvre et corrige en moyenne 3 à 4 bugs par session — c’est le quotidien et je ne compte pas m’arrêter.