
### Ejemplo de vector de tokens

Supón que el archivo fuente contiene:

```go
num := 14;
num2 := 16;
print(num + num2);
```

El vector de tokens generado:

```
[
  Token(IDENT, "num"),
  Token(COLON_ASSIGN, ":="),
  Token(INT_LITERAL, "14"),
  Token(SEMICOLON, ";"),
  Token(IDENT, "num2"),
  Token(COLON_ASSIGN, ":="),
  Token(INT_LITERAL, "16"),
  Token(SEMICOLON, ";"),
  Token(PRINT, "print"),
  Token(LPAREN, "("),
  Token(IDENT, "num"),
  Token(PLUS, "+"),
  Token(IDENT, "num2"),
  Token(RPAREN, ")"),
  Token(SEMICOLON, ";"),
  Token(EOF_TOKEN, "")
]
```

Cada `Token` tiene tipo, valor, y línea y columna.

---

## Explicación de las funciones de `Lexer`

- **peek()**  
  Devuelve el carácter actual sin avanzar la posición.

- **peekNext()**  
  Devuelve el siguiente carácter sin avanzar la posición.

- **advance()**  
  Avanza la posición y devuelve el carácter actual. También actualiza la línea y columna.

- **skipWhitespace()**  
  Salta todos los espacios en blanco (espacios, tabs, saltos de línea).

- **skipLineComment()**  
  Salta comentarios de línea (`// ...`).

- **skipBlockComment()**  
  Salta comentarios de bloque (`/* ... */`).

- **lexIdentifier()**  
  Lee un identificador o palabra clave (como `num`, `print`, `if`, etc.) y devuelve el token correspondiente.

- **lexNumber()**  
  Lee un número (entero o flotante) y devuelve el token correspondiente.

- **lexString()**  
  Lee una cadena entre comillas (`"texto"`) y devuelve el token correspondiente.

- **tokenize()**  
  Es el método principal: recorre todo el string fuente, usando las funciones anteriores para identificar y crear los tokens, y los agrega al vector de salida.

---