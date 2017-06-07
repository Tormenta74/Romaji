Generación de código Q
===================
#### Diego Sáinz de Medrano

Introducción
-------------

Una vez construidos los analizadores léxico y sintáctico-semántico (con Flex y la técnica de *descenso recursivo* en C/C++ respectivamente), procedemos a integrar un módulo que genere código Q y lo imprima en un fichero objeto.

Inicialización y técnicas de impresión
--------------------------------------------------

Para generar código Q, abriremos un fichero con el mismo nombre que el fichero fuente más la extensión `.q.c` en el que escribiremos las cabeceras que se requieren para compatibilidad con la máquina Q:
```C
#include "Q.h"
BEGIN
```
Una vez finalizado el proceso de compilación escribiremos antes de cerrar el fichero objeto:
```C
END
```
---
Para escribir todo tipo de código en el objeto, utilizaremos un wrapper de la función `fprintf` que nos facilitará enormemente el pasado de argumentos.

Dada la función
```C
void codegen(char *qline) {
	fprintf(obj_file, qline);
}
```
definimos la siguiente macro
```C
#define qgen(...) {\
	char qline[256];\
	sprintf(q_line,__VA_ARGS__);\
	strcat(q_line,"\n");\
	codegen(q_line);\
}
```
que permite llamadas con argumentos variadicos. Un ejemplo de llamada es:
```C
    qgen("L %d:\t",label);   // label es un int
```
Declaración de variables
---------------------------------

En Romaji se permite el uso de variables globales y locales. Las variables globales se almacenarán en memoria estática y serán accesibles a todo el programa (en las líneas posteriores a su declaración) mientras que las locales sólo serán accesibles al bloque de código de función en el que fueron declaradas, y se almacenarán en la pila, referenciadas por el **contexto** de la función.

La declaración de variables globales es sencilla. Una vez encontrado el tipo de la variable se escribirá un bloque de memoria estática de esta forma
```C
STAT(0)
	...
CODE(0)
```
Donde podemos llamar a las funciones de memoria de Q. De acuerdo con el tipo, reservaremos memoria con la función `DAT` o `STR` para cadenas (se rellenará la memoria con espacios en blanco o el literal que se usó en la inicialización en este caso). La dirección de memoria estática (que previamente habremos decrementado en una cantidad de octetos adecuada para hacer caber el tipo de dato) será la que podamos utilizar para referenciar esta variable (para ello guardaremos la dirección en la tabla de símbolos).

El caso de las variables locales es aún más sencillo. Para todos los tipos de datos (menos las cadenas de caracteres) reservaremos tantos octetos de la pila como sea necesario y guardaremos la posición de la "base" de la variable como su dirección.
```C
// ejemplo de un caso int
qgen("\tR7 = R7 - 4;");
// acceso
qgen("\tR0 = I(R6 - %d);",address);
```
Llamadas a funciones
-----------------------------
Para manejar las funciones en Romaji se ha hecho un uso intensivo de la pila de la máquina Q. Por cada llamada a una función se realizan las siguientes tareas:

1. Salvado de registros
2. Salvado del frame pointer
3. Salvado de la dirección de retorno
4. Paso de parámetros
5. Salvado del número de parámetros
6. "Reset" del frame pointer
7. Salto al código de la función

El contexto (o frame pointer) de una función es meramente una dirección de memoria de pila que será siempre referenciado por el registro R6. A partir de esta dirección se referencian los parámetros que se han pasado a la función y las variables que se han declarado localmente.

Las funciones, al retornar, se encargan de las siguientes tareas:

1. Resetear el puntero de pila al frame pointer (descartando todas las variables locales).
2. Guardar el valor de retorno en la pila (opcional).
2. Descartar todos los parámetros almacenados en la pila.
3. Recuperar la dirección de retorno.
4. Saltar a la dirección de retorno.

En la dirección de retorno se terminará de restaurar la pila:

1. Recuperar el frame pointer del llamador
2. Recuperar el valor de retorno (opcional)
3. Liberar la pila asignada al área de retorno.
4. Salvado de registros.

Al terminar esta operación, la pila queda restaurada a su estado inmediatamente anterior a la llamada, dado que por cada reserva de memoria de pila se ha liberado la misma.

Uso de parámetros por referencia
---------------------------------------------


Manejo de registros
---------------------------

Expresiones
----------------
El manejo de expresiones en el compilador es relativamente sencillo. Cada vez que percibamos un operador, sabremos que hay que obtener los valores de los dos operandos, y luego guardar el valor de la operación per se. Cada vez que percibamos un terminal, será o bien
una variable, una llamada a una función (en cuyo caso tomaremos el retorno), o un valor literal.

Pongamos la operación de ejemplo:
```
+ a 1
```

El código se parecería a esto:
```C
qgen("\tR%d = I(0x%x);",get_reg(),address);
...
qgen("\tR%d = %d;",get_reg(),value);
...
qgen("\tR%d = R%d %c R%d;",reg1, reg1, oper, reg2);
```
Las expresiones lógicas siguen un modelo parecido, con dos diferencias:

1. No existen variables de tipo booleano (`shinri`), por lo que no se debe nunca recuperar el valor de una.
2. Sólo se llaman a expresiones booleanas cuando escribimos una guarda condicional (`to`, traducción de `if`), cuando escribimos la condición de un bucle (`naka`, traducción de `while`) o cuando retornamos en una función con retorno booleano (ejemplo, `kisu = a b`).

---
#### Uso de las expresiones en condicionales
Al usarse estas expresiones en lugares muy concretos, definimos un mecanismo en la función que parsea las expresiones booleanas que consiste en lo siguiente. Cuando en la gramática encontramos un caso en el que debamos hacer un salto condicional, iniciamos el parseado de la misma (siguiendo la técnica de descenso recursivo) y al final del computado de la expresión añadimos la línea
```C
qgen("\tIF(R%d);",result_reg());
```
Ejemplo del funcionamiento:
```C
/** fichero fuente .rji **/

to & >= a 18 < a 21 {
	tsutaeru("Go vote, but don't drink")
}

/** fichero objeto .rji.q.c **/

	R0 = I(R6-8);    // toma del primer operando
	R1 = 18;         // toma del segundo operando
	R0 = R0 >= R1;   // guardado de la operación (y toma del primer operando)
	R1 = I(R6-8);    // toma del primer operando
	R2 = 21;         // toma del segundo operando
	R1 = R1 < R2;    // guardado de la operación (y toma del segundo operando)
	R0 = R0 && R1;   // guardado de la operación
	IF(R0)           // checkeo del valor de la condición
	GT(2);           // dirección de salto en caso de ser cierta
	GT(3);           // dirección de salto en caso de no serlo
```

Impresión
--------------
La impresión es un caso especial de llamada en Romaji, diferenciándose en que puede tomar literales de todo tipo además de variables. El esquema para generar llamadas a la pasarela de impresión de Qlib es el siguiente.

* Se guardan los registros en la pila.
* Por cada parámetro parseado, se toma o bien su valor (en caso de variables enteras) y se guarda en el registro adecuado o bien se crea una cadena estática que es impresa con los valores percibidos.
* Se guarda la dirección de la ristra en la que se imprimió el valor o la ristra de formato en el registro adecuado.
* Se llama a la función `putf_` de la librería Q.
* Se llama a una función añadida a la librería Q, `putnl_` una vez termiando el trabajo con los parámetros.

Ejemplo:
```C
/** fichero fuente .rji **/

tsutaeru("print this number: " 12 "\nand this variable: " a)

/** fichero objeto .rji.q.c **/

STAT(1)
		STR(0x11ff4,"print this number: ");
CODE(1)
		R1 = 0x11ff4;
		R0 = 2;
		GT(putf_);
L 2:
		R2 = 12;
		R1 = 0x11ffb;   // predeclared format string
		R0 = 3;
		GT(putf_);
L 3:	
STAT(2)
		STR(0x11d7a,"\nand this variable: "); // valor ejemplo
CODE(2)	
		R1 = 0x11d7a;
		R0 = 4;
		GT(putf_);
L 4:
		R0 = I(R6-12);
		R2 = R1;
		R1 = 0x11ffb;   // predeclared format string
		R0 = 5;
		GT(putf_);
L 5:
		R0 = 6;
		GT(putnl_);
L 6:
```
