#Romaji
---
Nuevo en esta versión:

- El analizador sintáctico es ahora un parseador por descenso recursivo.
- Se encuentran las versiones iniciales (sin trabajar) del parseador y el escaneador léxico en las carpetas _depr\_drparser_ y _depr\_lexer_.
- Se ha cambiado la gramática para acomodar el análisis (cambios explicados más adelante).
- Ampliada y refinada la funcionalidad de la tabla de símbolos.

---
Los cambios más significativos de la gramática son los siguientes:

- La función principal ahora se declara sin argumentos de entrada.
- Solo se permite que la función principal tenga un retorno "seisu" o "kyo".
- Se eliminan los tipos "daburu" y "naga seisu" (los tipos "seisu" y "furotingu" serán traducidos en lenguaje máquina a los enteros de 32 bits y números de coma flotante de 64 bits respectivamente).
