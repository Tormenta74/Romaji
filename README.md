#Romaji

Última versión del compilador y especificación del lenguaje Romaji, incluyendo el módulo en construcción de generación de código Q. Para la versión "limpia" del compilador con el analizador sintáctico por descenso recursivo, hacer un checkout al commit con la etiqueta "alpha" (más útil para una visión del parseador sin la complejidad añadida de las instrucciones de generación de código).

---
Notas:

- Se encuentran las versiones iniciales (sin trabajar) del parseador y el escaneador léxico en las carpetas _\_depr\_drparser_ y _\_depr\_lexer_.
- Se ha cambiado la gramática para acomodar el análisis (cambios explicados más adelante).
- Ampliada y refinada la funcionalidad de la tabla de símbolos.
- Incluído el módulo de generación de código Q (en construcción).

---
Los cambios más significativos de la gramática son los siguientes:

- La función principal ahora se declara sin argumentos de entrada.
- Solo se permite que la función principal tenga un retorno "seisu" o "kyo".
- Se eliminan los tipos "daburu" y "naga seisu" (los tipos "seisu" y "furotingu" serán traducidos en lenguaje máquina a los enteros de 32 bits y números de coma flotante de 64 bits respectivamente).
- Se declaran las variables "mojiretsu" con un número entre corchetes después del identificador de la variable, lo cual inicializa una zona de memoria del tamaño especificado: la excepción es que se puede declarar sólo con los corchetes, siempre y cuando se inicialice de inmediato con un literal de tipo ristra.

---
Acerca de la última fase, de generación de código.

El módulo y la integración en el parseador están incompletos. Existen las siguientes funcionalidades:
- declaración de variables (en memoria) y asignación (globales y locales)
- declaración de funciones
- reserva de memoria para ristras (literales y variables)
- expresiones aritméticas, lógicas y de comparación (sin derramado de registros)
- incremento y decremento de variables enteras
- bloques de código guardados por condicionales _if_ / _else_ o _while_
- guardado y recuperación de registros en llamadas (incompleto)
- salida del programa vía GT(-2)

