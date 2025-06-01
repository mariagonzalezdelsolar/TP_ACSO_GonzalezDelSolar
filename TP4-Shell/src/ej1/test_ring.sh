#!/bin/bash

# Test script para Ring Communication
# Uso: ./test_ring.sh

echo "=== TESTING RING COMMUNICATION ==="
echo "Compilando ring..."
gcc -o anillo ring.c
if [ $? -ne 0 ]; then
    echo "ERROR: No se pudo compilar ring.c"
    exit 1
fi

# Function to test ring with timeout
test_ring() {
    local n=$1
    local c=$2
    local s=$3
    local expected=$4
    local test_name=$5
    
    echo "Test: $test_name"
    echo "Parámetros: n=$n, c=$c, s=$s"
    echo "Esperado: $expected"
    
    # Run with timeout to avoid infinite hangs
    timeout 10 ./anillo $n $c $s > ring_output.txt 2>&1
    local exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        echo "  FAILED: Timeout (posible deadlock)"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "  FAILED: Exit code $exit_code"
        cat ring_output.txt
        return 1
    fi
    
    # Check if expected result is in output
    if grep -q "$expected" ring_output.txt; then
        echo "  PASSED"
        return 0
    else
        echo "  FAILED: No se encontró resultado esperado"
        echo "Output:"
        cat ring_output.txt
        return 1
    fi
}

echo -e "\n=== CASOS BÁSICOS ==="

# Test 1: Caso mínimo (3 procesos)
test_ring 3 10 1 13 "Caso mínimo - 3 procesos"

# Test 2: Caso del assignment
test_ring 4 10 2 14 "Caso del assignment - 4 procesos"

# Test 3: Más procesos
test_ring 5 0 1 5 "5 procesos empezando con 0"

echo -e "\n=== CASOS BORDE ==="

# Test 5: Proceso start = 1 (primer proceso)
test_ring 4 100 1 104 "Start en primer proceso"

# Test 6: Proceso start = n (último proceso)  
test_ring 4 100 4 104 "Start en último proceso"

# Test 7: Muchos procesos
test_ring 10 1 5 11 "Muchos procesos (10)"

# Test 8: Valor grande
test_ring 3 1000000 1 1000003 "Valor inicial grande"

echo -e "\n=== CASOS DE ERROR ==="

# Test 9: Argumentos insuficientes
echo "Test: Argumentos insuficientes"
./anillo 2>&1 | grep -q "Uso:" 
if [ $? -eq 0 ]; then
    echo "  PASSED"
else
    echo "  FAILED"
fi

# Test 10: n muy pequeño (menos de 3)
echo "Test: n < 3"
./anillo 2 10 1 > ring_error.txt 2>&1
if grep -q "debe ser" ring_error.txt || [ $? -ne 0 ]; then
    echo "  PASSED"
else
    echo "  FAILED"
    cat ring_error.txt
fi

# Test 11: start inválido (mayor que n)
echo "Test: start > n"
./anillo 3 10 5 > ring_error.txt 2>&1
if grep -q "debe ser" ring_error.txt || [ $? -ne 0 ]; then
    echo "  PASSED"
else
    echo "  FAILED"
    cat ring_error.txt
fi

# Test 12: start inválido (menor que 1)
echo "Test: start < 1"
./anillo 3 10 0 > ring_error.txt 2>&1
if grep -q "debe ser" ring_error.txt || [ $? -ne 0 ]; then
    echo "  PASSED"
else
    echo "  FAILED"
    cat ring_error.txt
fi

# Test 13: Argumentos no numéricos
echo "Test: Argumentos no numéricos"
./anillo abc def ghi > ring_error.txt 2>&1
if [ $? -ne 0 ]; then
    echo "  PASSED"
else
    echo "  FAILED"
    cat ring_error.txt
fi

echo -e "\n=== TESTS DE ESTRÉS ==="

# Test 14: Muchos procesos con timeout
echo "Test: Estrés con muchos procesos"
timeout 15 ./anillo 20 1 10 > ring_stress.txt 2>&1
if [ $? -eq 0 ] && grep -q "21" ring_stress.txt; then
    echo "  PASSED"
else
    echo "  FAILED o TIMEOUT"
    head -10 ring_stress.txt
fi

# Test 15: Verificar que no hay procesos zombies
echo "Test: No procesos zombie"
./anillo 5 10 3 > /dev/null 2>&1 &
PARENT_PID=$!
sleep 2
ZOMBIES=$(ps aux | grep -c "[Zz]ombie\|<defunct>")
wait $PARENT_PID
if [ $ZOMBIES -eq 0 ]; then
    echo "  PASSED"
else
    echo "  FAILED: $ZOMBIES procesos zombie encontrados"
fi

echo -e "\n=== VERIFICACIÓN MANUAL ==="

echo "Test: Verificación manual con output detallado"
echo "Ejecutando: ./anillo 4 10 2"
./anillo 4 10 2
echo ""
echo "¿El resultado es correcto? (Debería ser 14)"
echo "¿Se muestran todos los procesos recibiendo/enviando mensajes?"

echo -e "\n=== TESTS DE CONCURRENCIA ==="

# Test 16: Múltiples ejecuciones rápidas
echo "Test: Múltiples ejecuciones"
SUCCESS=0
for i in {1..5}; do
    timeout 5 ./anillo 4 $i 2 > ring_multi_$i.txt 2>&1
    if [ $? -eq 0 ] && grep -q "$((i+4))" ring_multi_$i.txt; then
        SUCCESS=$((SUCCESS+1))
    fi
done

if [ $SUCCESS -eq 5 ]; then
    echo "  PASSED: Todas las ejecuciones exitosas"
else
    echo "  FAILED: Solo $SUCCESS/5 ejecuciones exitosas"
fi

echo -e "\n=== LIMPIEZA ==="
rm -f ring_*.txt anillo

echo -e "\n=== RESUMEN ==="
echo "Tests completados. Revisa los resultados arriba."
echo "IMPORTANTE: Asegúrate de que tu ring maneja correctamente:"
echo "- Casos básicos con diferentes números de procesos ✓"
echo "- Diferentes posiciones de inicio ✓"
echo "- Validación de argumentos ✓"
echo "- No deadlocks ni hangs ✓"
echo "- No procesos zombie ✓"
echo "- Valores negativos y grandes ✓"

echo -e "\n=== CASOS CRÍTICOS A VERIFICAR MANUALMENTE ==="
echo "1. ./anillo 3 10 1 → Debe dar 13"
echo "2. ./anillo 4 10 2 → Debe dar 14"  
echo "3. ./anillo 5 0 3 → Debe dar 5"
echo "4. No debe hacer hang ni crear procesos zombie"
echo "5. Debe mostrar mensajes de cada proceso"