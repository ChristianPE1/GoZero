# GoZero 

GoZero es un compilador para un lenguaje inspirado en Go, implementado en C++ con backend LLVM. Soporta inferencia de tipos, arrays, strings, control de flujo, funciones, y generación de código eficiente. El sistema de construcción es robusto y multiplataforma, con soporte para Makefile y CMake.

---

## 📁 Estructura del Proyecto

```
├── include/           # Cabeceras (.h)
├── src/               # Implementaciones (.cpp)
├── main.cpp           # Punto de entrada
├── Makefile           # Build principal
├── CMakeLists.txt     # Build alternativo
├── build/             # (opcional) Carpeta de build CMake
├── gozero             # Compilador generado
├── my_program         # Programa compilado
├── output.o           # Código objeto
├── *.goz, *.txt       # Archivos de entrada/prueba
```

---

## 🛠️ Comandos Principales (Makefile)

```bash
make                    # Compilar el compilador GoZero (genera 'gozero')
make clean              # Limpiar archivos generados
make test               # Compilar y ejecutar mini_input.txt
make demo               # Crear archivo de ejemplo demo_simple.goz
make run FILE=archivo.goz  # Compilar y ejecutar archivo específico
make ir                 # Ver solo el código LLVM IR
make assembly           # Ver código máquina generado
make help               # Ver ayuda de comandos
```

### Ejecución Manual

```bash
./gozero archivo.goz    # Compilar archivo fuente
./gozero archivo.goz -i # Para compilar y ver el codigo intermedio
./my_program            # Ejecutar el programa compilado
```

---

## 🛠️ Comandos Alternativos (CMake)

```bash
mkdir build && cd build
cmake ..

cmake --build . --target gozero           # Solo compilador
cmake --build . --target compile_and_run  # Compilar y ejecutar
cmake --build . --target view_ir          # Solo IR
cmake --build . --target view_assembly    # Solo código máquina
```

---

## 📝 Ejemplo de Flujo de Trabajo

```bash
make                        # Compilar el compilador
make demo                   # Crear ejemplo demo_simple.goz
make run FILE=demo_simple.goz  # Compilar y ejecutar el ejemplo
./my_program                # Ejecutar el resultado
```

---

## 💡 Ejemplo de Código GoZero

```go
// Variables y arrays
x := 10;
int nums = [1, 2, 3];
float valores = [1.1, 2.2, 3.3];
string nombres = ["Ana", "Luis", "Juan"];

// Operaciones con arrays
a := [1, 2, 3];
b := [4, 5, 6];
suma := a + b;        // [5, 7, 9]
producto := a * b;    // [4, 10, 18]
escalar := a * 2;     // [2, 4, 6]

// Funciones
fun saludar() {
    print("Hola mundo");
}
saludar();

// Control de flujo
if (x > 5) {
    print("x es mayor que 5");
}
for (i := 0; i < 3; ++i) {
    print(i);
}
```

---

## 📦 Archivos Generados

- `gozero` - Compilador
- `output.o` - Código objeto
- `my_program` - Ejecutable final
- `obj/` - Archivos temporales

---

## 🆘 Solución de Problemas

```bash
make clean && make         # Forzar recompilación
llvm-config --version      # Verificar instalación de LLVM
ls -la                     # Verificar archivos generados
```

---

## 🏆 Características Soportadas

- Arrays 1D (suma, multiplicación, escalar)
- Declaración explícita e inferida de tipos
- Strings y concatenación
- Control de flujo (if, for, while)
- Funciones con y sin return
- Bounds checking y manejo de errores
- Prevención de matrices 2D
- Mensajes de error descriptivos
- Generación de código LLVM IR y ensamblador

---

## 📚 Créditos y Licencia

Desarrollado por Christian Pardavé y Saul Condori. Uso educativo y libre.
