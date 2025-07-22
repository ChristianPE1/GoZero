
## 1. **Inferencia y consulta de tipos**

- Permite saber de qué tipo es cada variable, expresión o función.
- Por ejemplo, si tienes `x := 5;`, el `TypeAnalyzer` infiere que `x` es de tipo `int`.
- Si tienes `print(x + 2.5);`, infiere que la expresión es de tipo `float`.

### Funciones clave:
- `inferType(const Expr *expr)`:  
  Recibe un puntero a una expresión del AST y retorna el tipo (`VarDeclStmt::Kind`) de esa expresión (por ejemplo, `INT`, `FLOAT`, `STRING`, `INT_ARRAY`, etc.).
- `getVariableType(const std::string &name)`:  
  Devuelve el tipo de una variable.
- `getFunctionReturnType(const std::string &name)`:  
  Devuelve el tipo de retorno de una función.

---

## 2. **Validación de scopes y tipos**

- Verifica que todas las variables y funciones estén correctamente declaradas y sean accesibles en el scope adecuado.
- Detecta errores como:
  - Uso de variables no declaradas.
  - Acceso a variables fuera de su scope.
  - Llamadas a funciones no declaradas.
  - Tipos incompatibles en operaciones.

### Funciones :

---

## **Funciones de manejo de scopes**

- **TypeAnalyzer::TypeAnalyzer()**
  - Constructor. Inicializa el analizador de tipos y crea el scope global.

- **pushScope()**
  - Agrega un nuevo scope (alcance) al stack de scopes.  
    Útil para entrar a funciones, bloques, etc.

- **popScope()**
  - Elimina el scope más reciente del stack.  
    Útil para salir de funciones, bloques, etc.

---

## **Inferencia y consulta de tipos**

- **inferType(const Expr *expr)**
  - Dada una expresión del AST, determina su tipo (int, float, string, array, etc.).
  - Analiza el tipo de literales, variables, operaciones, arrays, indexaciones, llamadas a función, etc.
  - Si encuentra un error (por ejemplo, variable no declarada), muestra un mensaje y termina el programa.

- **declareFunction(const std::string &name, VarDeclStmt::Kind returnType)**
  - Registra el tipo de retorno de una función en el mapa de funciones.

- **setFunctionParams(const std::string &name, const std::vector<VarDeclStmt::Kind> &paramTypes)**
  - Registra los tipos de parámetros de una función.

- **getFunctionParams(const std::string &name)**
  - Devuelve un vector con los tipos de parámetros de la función dada.

- **hasFunction(const std::string &name)**
  - Retorna `true` si la función está declarada.

- **declareVariable(const std::string &name, VarDeclStmt::Kind type)**
  - Declara una variable en el scope actual con su tipo.

- **getVariableType(const std::string &name)**
  - Busca el tipo de una variable en los scopes activos (de adentro hacia afuera).
  - Si no la encuentra, muestra un error y termina el programa.

- **hasVariable(const std::string &name)**
  - Retorna `true` si la variable está declarada en algún scope activo.

---

## **Análisis de expresiones y sentencias**

- **analyzeCallExpr(const CallExpr *call)**
  - Analiza una llamada a función para inferir los tipos de sus argumentos y actualizar los tipos de parámetros de la función.

- **analyzeExpression(const Expr *expr)**
  - Analiza recursivamente una expresión (llamadas, operaciones, indexaciones, arrays) para inferir tipos y actualizar información.

- **analyzeStatement(const Stmt *stmt)**
  - Analiza recursivamente una sentencia (print, asignación, declaración, if, while, for, función, return) para inferir tipos y actualizar información.

---

## **Validación de funciones y scopes**

- **hasExplicitReturn(const std::vector<StmtPtr> &body)**
  - Recorre el cuerpo de una función y retorna `true` si encuentra un `return` explícito (directo o anidado en if/while/for).

- **getFunctionReturnType(const std::string &name)**
  - Devuelve el tipo de retorno de una función (o `VOID` si no está registrada).

- **validateFunctionScopes(const FunctionStmt *funcStmt)**
  - Valida que todas las variables usadas en una función sean accesibles (parámetros o variables locales).
  - Crea un mapa de variables locales (parámetros y variables declaradas en la función).
  - Recorre todas las sentencias del cuerpo de la función y valida su scope.

- **validateExpressionInFunctionScope(const Expr *expr, const std::map<std::string, VarDeclStmt::Kind> &localVars, const std::string &functionName)**
  - Valida recursivamente que todas las variables usadas en una expresión estén en el scope local de la función.
  - Si encuentra una variable fuera de scope, muestra un error y termina el programa.

- **validateStatementInFunctionScope(const Stmt *stmt, const std::map<std::string, VarDeclStmt::Kind> &localVars, const std::string &functionName)**
  - Valida recursivamente que todas las variables usadas en una sentencia estén en el scope local de la función.
  - Actualiza el scope local cuando se declaran nuevas variables dentro de la función o en bucles.

---

## Resumen

- **TypeAnalyzer** es el "cerebro" de tipos y scopes del compilador.
- Sirve para:
  - Inferir tipos de expresiones y variables.
  - Validar que el código sea correcto antes de la generación de código.
- No produce un AST ni un string, sino que ayuda a que el AST sea semánticamente válido y a que el generador de código sepa qué tipo tiene cada cosa.
