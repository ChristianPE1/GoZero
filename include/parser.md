
1. El lexer convierte el código fuente en un `std::vector<Token>`.
2. Ese vector de tokens se pasa al parser:  
   ```cpp
   Parser parser(tokens);
   auto stmts = parser.parse();
   ```
3. El método `parse()` devuelve un `std::vector<StmtPtr>`.

---
¡Perfecta pregunta para tu documentación! Aquí tienes una explicación clara de cada función de `parser.cpp`, qué retorna y qué tipo de datos conforma ese retorno:

---

## Tipos principales

- **ExprPtr**: alias de `std::unique_ptr<Expr>`.  
  Representa un nodo de expresión en el AST (número, variable, operación, llamada, etc.).

- **StmtPtr**: alias de `std::unique_ptr<Stmt>`.  
  Representa un nodo de sentencia en el AST (declaración, asignación, if, while, print, etc.).

---

## Funciones del parser

### Funciones de navegación y utilidades

- **peek()**  
  Devuelve una referencia al token actual (no avanza la posición).  
  Retorna: `const Token&`

- **advance()**  
  Devuelve el token actual y avanza la posición.  
  Retorna: `const Token&`

- **match(TokenType t)**  
  Si el token actual es de tipo `t`, lo consume y retorna `true`. Si no, retorna `false`.

- **check(TokenType t)**  
  Retorna `true` si el token actual es de tipo `t`, sin consumirlo.

- **expect(TokenType t, const std::string &msg)**  
  Si el token actual es de tipo `t`, lo consume. Si no, muestra un error y termina el programa.

---

### Funciones de parsing de expresiones

- **parseExpression()**  
  Punto de entrada para analizar cualquier expresión.  
  Retorna: `ExprPtr` (nodo raíz de la expresión)

- **parseOr(), parseAnd(), parseEquality(), parseRelational(), parseAddSub(), parseMulDiv()**  
  Analizan expresiones con operadores lógicos y aritméticos, respetando la precedencia.  
  Retornan: `ExprPtr` (nodo de operación binaria o subexpresión)

- **parseUnary()**  
  Analiza operadores unarios (`++`, `--`).  
  Retorna: `ExprPtr` (nodo de operación unaria o subexpresión)

- **parsePrimary()**  
  Analiza literales, variables, llamadas a función, acceso a arrays, paréntesis, arrays literales.  
  Retorna: `ExprPtr` (nodo de literal, variable, llamada, indexación, etc.)

- **parseArray()**  
  Analiza un array literal (`[1,2,3]`).  
  Retorna: `ExprPtr` (nodo de tipo `ArrayExpr`)

---

### Funciones de parsing de sentencias

- **parseBlock()**  
  Analiza un bloque de sentencias `{ ... }`.  
  Retorna: `std::vector<StmtPtr>` (lista de sentencias del bloque)

- **parseIf()**  
  Analiza una sentencia `if` (con posible `else`).  
  Retorna: `StmtPtr` (nodo `IfStmt`)

- **parseWhile()**  
  Analiza una sentencia `while`.  
  Retorna: `StmtPtr` (nodo `WhileStmt`)

- **parseFor()**  
  Analiza una sentencia `for`.  
  Retorna: `StmtPtr` (nodo `ForStmt`)

- **parseForInitOrDecl()**  
  Analiza la inicialización o declaración en un `for`.  
  Retorna: `StmtPtr` (nodo de declaración, asignación o expresión)

- **parseForPost()**  
  Analiza la parte de incremento/post en un `for`.  
  Retorna: `StmtPtr` (nodo de asignación o expresión)

- **parseFunction(bool inference)**  
  Analiza la declaración de una función.  
  Retorna: `StmtPtr` (nodo `FunctionStmt`)

- **parseStatement()**  
  Analiza una sentencia individual (declaración, asignación, función, if, while, print, etc.).  
  Retorna: `StmtPtr` (nodo de la sentencia correspondiente)

---

### Función principal

- **parse()**  
  Analiza todo el programa, construyendo el AST completo.  
  Retorna: `std::vector<StmtPtr>` (lista de sentencias del programa)

---

## ¿Qué contienen los retornos?

- **ExprPtr** y **StmtPtr** son punteros inteligentes a objetos derivados de `Expr` o `Stmt`.
- Cada objeto tiene información relevante:
  - Literales: valor (int, float, string)
  - Variables: nombre, posición
  - Operaciones: tipo de operación, operandos
  - Llamadas: nombre de función, argumentos
  - Sentencias: tipo, subnodos (por ejemplo, condición y ramas en un if)
  - Funciones: nombre, parámetros, cuerpo, tipo de retorno

**No es un string:**  
Es una estructura de datos (árbol) que representa el programa de manera jerárquica y estructurada, lista para análisis semántico y generación de código.

---

¿Quieres un ejemplo visual de cómo se vería un AST para un código simple? ¿O una explicación de una clase de nodo específica?

¡Por supuesto! Aquí tienes un ejemplo visual de cómo se vería el AST (árbol de sintaxis abstracta) que retorna el parser para un código simple:

### Código de entrada

```go
num := 14;
num2 := 16;
print(num + num2);
```

### Vector de StmtPtr retornado por el parser

Supongamos que el parser retorna un vector con 3 elementos, cada uno apuntando a un nodo del AST:

```
[
  InferDeclStmt("num", LiteralExpr(14)),
  InferDeclStmt("num2", LiteralExpr(16)),
  ExprStmt(
    CallExpr(
      "print",
      [
        BinaryExpr(
          Op::ADD,
          VarExpr("num"),
          VarExpr("num2")
        )
      ]
    )
  )
]
```

#### Explicación de cada nodo:

1. **InferDeclStmt("num", LiteralExpr(14))**
   - Declaración por inferencia: `num := 14;`
   - Nodo de tipo `InferDeclStmt` con nombre `"num"` y valor `LiteralExpr(14)`.

2. **InferDeclStmt("num2", LiteralExpr(16))**
   - Declaración por inferencia: `num2 := 16;`
   - Nodo de tipo `InferDeclStmt` con nombre `"num2"` y valor `LiteralExpr(16)`.

3. **ExprStmt(CallExpr(...))**
   - Sentencia de expresión: `print(num + num2);`
   - Nodo de tipo `ExprStmt` que contiene un `CallExpr`:
     - `CallExpr("print", [...])` representa la llamada a la función `print` con un argumento:
       - `BinaryExpr(Op::ADD, VarExpr("num"), VarExpr("num2"))` representa la suma `num + num2`.

---


### Código de entrada

```go
num := 14;
num2 := 16;
print(num + num2);
```

### Vector de StmtPtr retornado por el parser

```
[
  InferDeclStmt("num", LiteralExpr(14)),
  InferDeclStmt("num2", LiteralExpr(16)),
  ExprStmt(
    CallExpr(
      "print",
      [
        BinaryExpr(
          Op::ADD,
          VarExpr("num"),
          VarExpr("num2")
        )
      ]
    )
  )
]
```

#### Explicación de cada nodo:

1. **InferDeclStmt("num", LiteralExpr(14))**
   - Declaración por inferencia: `num := 14;`
   - Nodo de tipo `InferDeclStmt` con nombre `"num"` y valor `LiteralExpr(14)`.

2. **InferDeclStmt("num2", LiteralExpr(16))**
   - Declaración por inferencia: `num2 := 16;`
   - Nodo de tipo `InferDeclStmt` con nombre `"num2"` y valor `LiteralExpr(16)`.

3. **ExprStmt(CallExpr(...))**
   - Sentencia de expresión: `print(num + num2);`
   - Nodo de tipo `ExprStmt` que contiene un `CallExpr`:
     - `CallExpr("print", [...])` representa la llamada a la función `print` con un argumento:
       - `BinaryExpr(Op::ADD, VarExpr("num"), VarExpr("num2"))` representa la suma `num + num2`.

---


- El parser retorna un vector de punteros a sentencias (`std::vector<StmtPtr>`).
- Cada puntero apunta a un nodo que representa una sentencia del programa.
- Los nodos pueden contener otros nodos (por ejemplo, una llamada a función contiene una expresión como argumento).
