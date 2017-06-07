// Qlib.h 3.7.1 - 3.7.3

// Fichero únicamente de definiciones opcionales de macros.
// Está permitido modificarlo, al igual que Qlib.c,
// debiéndose entonces recompilar IQ (ver Qman.txt).  
// Las macros aquí definidas son utilizables en el código Q 
// y en Qlib.c, sólo si inteprete y compilador de Q trabajan 
// sobre la salida del preprocesador de C (cpp).


// Reconfiguración de direcciones base de la máquina Q (véase Q.h para
// valores por defecto).  Descomentar para activar.  
// Deben cumplirse que H > Z > 0 y que ambos sean múltiplos de 4.
//#define H   0x00200000 // base+1 del heap (1 MB máx hasta Z=0x00100000)
//#define Z   0x00100000 // base+1 de la zona estática más pila (1 MB máx hasta 0x00000000) 
//#define LLL -9999      // última etiqueta admisible en Qlib; se debe cumplir LLL < -10


// Para el resto de nombres de macro, con el objeto de evitar posibles
// colisiones con las de Q.h, utilícese el carácter '_'

// Si queremos usar estos nombres en lugar de sus valores numéricos
#define __ini	 0    // comienzo 
#define __brk    -1   // breakpoint "manual" en IQ
#define __fin    -2   // terminacion normal
#define __abo    -3   // terminacion anticipada

// Igualmente para funciones de Qlib; úsense sólo etiquetas no superiores a -10
#define exit_    -10    // NOTA: a eliminar en futuras versiones, mantenido por compatibilidad
#define new_     -11    // asigna o libera espacio en heap
#define putf_    -12    // visualiza ristra y/o entero
#define putnl_   -13    // visualiza salto de línea
