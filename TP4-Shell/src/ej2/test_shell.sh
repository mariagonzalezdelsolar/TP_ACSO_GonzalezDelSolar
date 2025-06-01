#!/bin/bash

# Test script para Shell
# Uso: ./test_shell.sh

echo "=== TESTING SHELL ==="
echo "Compilando shell..."
gcc -o shell shell.c
if [ $? -ne 0 ]; then
    echo "ERROR: No se pudo compilar shell.c"
    exit 1
fi

# Crear archivos de prueba
echo "Creando archivos de prueba..."
echo "test content" > test.txt
echo "another test" > test2.txt  
echo "zip file content" > test.zip
echo "png file content" > test.png
mkdir -p testdir

echo -e "\n=== CASOS BÁSICOS ==="

# Test 1: Comandos simples
echo "Test 1: Comandos simples"
echo -e "ls\nexit" | timeout 5 ./shell > test1_output.txt 2>&1
if grep -q "test.txt" test1_output.txt; then
    echo " Test 1 PASSED"
else
    echo "  Test 1 FAILED"
    echo "Output:"
    cat test1_output.txt
fi

# Test 2: Pipes básicos
echo "Test 2: Pipes básicos"
echo -e "ls | grep .txt\nexit" | timeout 5 ./shell > test2_output.txt 2>&1
if grep -q "test.txt" test2_output.txt && grep -q "test2.txt" test2_output.txt; then
    echo " Test 2 PASSED"
else
    echo "  Test 2 FAILED"
    echo "Output:"
    cat test2_output.txt
fi

# Test 3: Múltiples pipes
echo "Test 3: Múltiples pipes"
echo -e "ls | grep .txt | wc -l\nexit" | timeout 5 ./shell > test3_output.txt 2>&1
if grep -q "2" test3_output.txt; then
    echo " Test 3 PASSED"
else
    echo "  Test 3 FAILED"
    echo "Output:"
    cat test3_output.txt
fi

echo -e "\n=== CASOS BORDE ==="

# Test 4: Comando vacío
echo "Test 4: Comando vacío"
echo -e "\n\nexit" | timeout 5 ./shell > test4_output.txt 2>&1
if [ $? -eq 0 ]; then
    echo "  Test 4 PASSED"
else
    echo "  Test 4 FAILED"
fi

# Test 5: Comando inexistente
echo "Test 5: Comando inexistente"
echo -e "comandoinexistente\nexit" | timeout 5 ./shell > test5_output.txt 2>&1
if grep -q "No such file" test5_output.txt || grep -q "command not found" test5_output.txt || grep -q "execvp failed" test5_output.txt; then
    echo "  Test 5 PASSED"
else
    echo "  Test 5 FAILED"
    echo "Output:"
    cat test5_output.txt
fi

# Test 6: Espacios extra
echo "Test 6: Espacios extra"
echo -e "   ls   |   grep   .txt   \nexit" | timeout 5 ./shell > test6_output.txt 2>&1
if grep -q "test.txt" test6_output.txt; then
    echo "  Test 6 PASSED"
else
    echo "  Test 6 FAILED"
    echo "Output:"
    cat test6_output.txt
fi

# Test 7: Pipes con comando inexistente
echo "Test 7: Pipes con comando inexistente"
echo -e "ls | comandoinexistente\nexit" | timeout 5 ./shell > test7_output.txt 2>&1
if grep -q "execvp failed" test7_output.txt || grep -q "No such file" test7_output.txt; then
    echo "  Test 7 PASSED"
else
    echo "  Test 7 FAILED"
    echo "Output:"
    cat test7_output.txt
fi

# Test 8: Muchos pipes
echo "Test 8: Muchos pipes (4 comandos)"
echo -e "ls | grep test | head -2 | wc -l\nexit" | timeout 5 ./shell > test8_output.txt 2>&1
if grep -q "2" test8_output.txt; then
    echo "  Test 8 PASSED"
else
    echo "  Test 8 FAILED"
    echo "Output:"
    cat test8_output.txt
fi

# Test 9: Pipe solo (edge case)
echo "Test 9: Solo pipe"
echo -e "|\nexit" | timeout 5 ./shell > test9_output.txt 2>&1
# Debería manejarse sin crash
if [ $? -eq 0 ]; then
    echo "  Test 9 PASSED (no crash)"
else
    echo "  Test 9 FAILED (crashed)"
fi

# Test 10: Comando con argumentos complejos
echo "Test 10: Argumentos complejos"
echo -e "ls -la | grep -v total | head -3\nexit" | timeout 5 ./shell > test10_output.txt 2>&1
if [ -s test10_output.txt ]; then
    echo "  Test 10 PASSED"
else
    echo "  Test 10 FAILED"
    echo "Output:"
    cat test10_output.txt
fi

echo -e "\n=== CASOS DE ESTRÉS ==="

# Test 11: Muchos comandos seguidos
echo "Test 11: Muchos comandos seguidos"
{
    echo "ls"
    echo "pwd" 
    echo "ls | grep test"
    echo "date"
    echo "whoami"
    echo "exit"
} | timeout 10 ./shell > test11_output.txt 2>&1

if [ $? -eq 0 ]; then
    echo "  Test 11 PASSED"
else
    echo "  Test 11 FAILED"
fi

# Test 12: EOF (Ctrl+D)
echo "Test 12: EOF handling"
echo "ls" | timeout 5 ./shell > test12_output.txt 2>&1
if [ $? -eq 0 ]; then
    echo "  Test 12 PASSED"
else
    echo "  Test 12 FAILED"
fi

echo -e "\n=== COMPARACIÓN CON BASH ==="

# Test 13: Comparar salida con bash
echo "Test 13: Comparación con bash"
echo "ls | grep .txt" | bash > bash_output.txt 2>&1
echo -e "ls | grep .txt\nexit" | timeout 5 ./shell > shell_output.txt 2>&1

# Extraer solo la salida del comando (sin el prompt)
grep -v "Shell>" shell_output.txt | grep -v "Simple Shell" | grep -v "exit" > shell_clean.txt

if diff -q bash_output.txt shell_clean.txt > /dev/null; then
    echo "  Test 13 PASSED (salida idéntica a bash)"
else
    echo "  Test 13 FAILED (salida diferente a bash)"
    echo "Bash output:"
    cat bash_output.txt
    echo "Shell output:"
    cat shell_clean.txt
fi

echo -e "\n=== LIMPIEZA ==="
rm -f test*.txt test*.zip test*.png bash_output.txt shell_clean.txt
rm -rf testdir
rm -f shell

echo -e "\n=== RESUMEN ==="
echo "Tests completados. Revisa los resultados arriba."
echo "IMPORTANTE: Asegúrate de que tu shell maneja correctamente:"
echo "- Comandos simples ✓"
echo "- Pipes básicos ✓" 
echo "- Múltiples pipes ✓"
echo "- Casos borde (comandos vacíos, inexistentes) ✓"
echo "- Manejo de espacios extra ✓"
echo "- No crashes con inputs inválidos ✓"