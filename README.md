# Romaji

Compilador y especificación del lenguaje Romaji, incluyendo un módulo de análisis sintáctico por descenso recursivo y uno de generación de código Q en ~7K líneas (ver fichero _linecount.txt_).

### Versiones
- latest: (Important) fixes in codegen. **HEAD**
- beta: Recursive descent parser slightly redesigned. Codegen completely revamped.
- alpha: First attempt at codegen. Current grammar entirely working.
- clusterfuck: First major change in the grammar (barely working parser, no codegen).

### Gramática
Los cambios más significativos de la gramática con respecto a la especificación original son los siguientes:

- La función principal ahora se declara sin argumentos de entrada.
- Solo se permite que la función principal tenga un retorno "seisu" o "kyo".
- Se eliminan los tipos "daburu" y "naga seisu" (los tipos "seisu" y "furotingu" serán traducidos en lenguaje máquina a los enteros de 32 bits y números de coma flotante de 64 bits respectivamente).
- Se declaran las variables "mojiretsu" con un número entre corchetes después del identificador de la variable, lo cual inicializa una zona de memoria del tamaño especificado: la excepción es que se puede declarar sólo con los corchetes, siempre y cuando se inicialice de inmediato con un literal de tipo ristra.
- Las funciones sólo aceptan variables (o argumentos) como parámetros.
- Las funciones sólo pueden retornar valores (vía expresiones, tanto numéricas como lógicas)

### Generación de código

Existen las siguientes funcionalidades:
- declaración de variables globales (estáticas) y locales (en memoria de pila) y asignación
- declaración de funciones
- reserva de memoria para ristras (literales y variables)
- expresiones aritméticas, lógicas y de comparación (sin derramado de registros)
- incremento y decremento de variables enteras (incluyendo las variables sin signo)
- bloques de código guardados por condicionales _if_ / _else_ o _while_
- guardado y recuperación de registros en llamadas a funciones
- guardado y recuperación del contexto en llamadas a funciones
- paso de parámetros en llamadas a funciones vía pila
- paso de valor retorno de funciones vía pila
- salida del programa vía GT(-2)

