# Бинарный транслятор #

Цель работы ускорить исполнение кода, написанного на ассемблере для [программного процессора](https://github.com/SimonaytesYan/SoftCPU). Будем использовать для этого технологию JIT компиляции. Таким образом программа получает на вход исполняемый файл для программного процессора, переводит команды, содержащиеся в нём, в инструкции процессора x86-64, помещая их в выделенный буфер, и исполняет инструкции из этого буфера.

## Запуск проекта ##

После клонирования репозитория нужно создать недостающие папки, использующиеся во время компиляции. Это можно сделать, выполнив команду
```
make create_dir
```
Далее необходимо осуществить сборку проекта.

Чтобы собрать проект в debug режиме воспользуйтесь командой
```
make debug
```

Чтобы собрать проект в release режиме воспользуйтесь командой
```
make release
```

Исполняемый файл будет лежать в папке Exe под названием Translate. При запуске программы в качестве аргумента командной строки необходимо передать название исполняемого файла программного процессора Пример:
```
Exe/Translate Test.sy
```

## Механизм работы программы ##

Программный процессор имел стек вызовов, стек данных, RAM и 4 регистра. Для простоты трансляции, под стек вызовов и RAM были выделены отдельные области памяти. Регистр `rsi` стал стековым регистром для стека вызовов.

Для простоты обращения к RAM указатель на начало блока памяти, выделенного под неё, сохранён в `rdi`. Соответственно, все обращения к памяти должны происходить по смещению, хранящемуся в `rdi`.

В программном процессоре существовали только регистры RAX, RBX, RCX и RDX. Они соответствуют таким же регистрам в x64. Эти 4 регистры мы будем называть пользовательскими регистрами. Остальные регистры x64 - системными.

Гарантируется, что в начале программы все пользовательские регистры равны нулю.

Реализованна стандартная библиотека, включающая в себя реализации инструкций `in` и `out` программного процессора. Реализацию функций можно увидеть в файле Src/Stdlib/Stdlib.cpp. Функция `long long InputNumber10()` соответствует `in`, функция `void OutputNumber10(long long number)` соответствует `out`.\
Было принято решение написать собственную реализацию, вместо стандартных printf и scanf, из соображений производительности. Это является не сильно трудозатратным решением, так как необходимо реализовать только ввод и вывод десятичных чисел.

Компиляция осуществляется в два прохода, для обработки переходов вниз.

Программа завершается инструкцией ret.

Полученные команды помещаются в буфер для последующего исполнения. После завершения компиляции с помощью ассемблерной вставки осуществляется переход на начало выше описанного буфера.

## Перевод ##

| Команда программного процессора | x86-64 инструкции |
|---------------------------------|-------------------|
|`push reg/mem/num`                 |`push reg/mem/num`
|`pop  reg/mem`                     |`pop reg/mem`
|`add`                              |`pop r9` + `pop r8` + `add r8, r9` + `push r8` |
|`sub`                              |`pop r9` + `pop r8` + `add r8, r9` + `push r8`
|`mul`                              | Сохранение регистров rax, rbx и rdx + `pop rax` + `pop rbx` + `cqo` + `imul rbx` + `push rax` + восстановление сохранённых регистров
|`div`                              | Сохранение регистров rax, rbx и rdx + `pop rbx` + `pop rax` + `cqo` + `idiv rbx` + `push rax` + восстановление сохранённых регистров
|`in`                               | Сохранения пользовательских регистров + сохранение rsi и rdi + `call InputNumber10` + `push rax`, восстановление сохранённых регистров
|`out`                              | Сохранения пользовательских регистров + сохранение rsi и rdi + `pop rdi` + `call OutputNumber10` + восстановление сохранённых регистров
|`jmp label`                        | `jmp label`
|`ja  label`                        | `pop r9` + `pop r8` + `cmp r8, r9` + `jle  label` 
|`jae label`                        | `pop r9` + `pop r8` + `cmp r8, r9` + `jl label` 
|`jb  label`                        | `pop r9` + `pop r8` + `cmp r8, r9` + `jge  label` 
|`jbe label`                        | `pop r9` + `pop r8` + `cmp r8, r9` + `jg label` 
|`je  label`                        | `pop r9` + `pop r8` + `cmp r8, r9` + `jne  label` 
|`jne label`                        | `pop r9` + `pop r8` + `cmp r8, r9` + `je label` 
|`call label`                       | Кладём в память `[rsi]` адрес возврата + `add rsi, 8` + `jmp label`
|`ret`                              | Берём из памяти `[rsi]` адрес возврата + `jmp` на полученный адрес
|`hlt`                              | `ret`

Инверсия условных переходов обусловлена необычной реализацией условных переходов в soft CPU

## Ускорение ##

### Оборудование ###
 |||
-------------------|-
**Compiler**           | g++ 11.3.0
**OS**                 | Ubuntu 11.3.0 
**CPU**                | 11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz

**Флаги компиляции**: -01

### Условия ###

Для тестирования скорости работы программы были выбраны рекурсивный алгоритм вычисления i-го числа Фибоначчи(производился расчёт 32-го числа Фибоначчи) и алгоритм вычисления корней квадратного уравнения. Коды программ на ассемблере для программного процессора размещёны в файлах Examples/NthFib и Examples/QuadraticEqu. Исполняемые файлы для программного процессора этих программы размещён в той же папке под названиями NthFib.sy и QuadraticEqu.sy. В качестве времени бралось среднее значение по нескольким запускам программы на одном и том же входе(повторялся и замерялся только этап исполнения инструкций). Ввод данных программы осуществлялся из файла, путём перенаправления стандартного потока ввода.

Входные данные для расчёта чисел Фибоначчи были одни и те же: число 32. Тест считался 10 раз. Время - среднее по 10 запускам\
Входные данные для расчёта корней квадратного уравнения можно увидеть в файле Data/TestQuadraticSolver. Один тест состоял из вычисления корней для 7 троек коэффициентов:
| a | b | c |
|---|---|---|
| 1 | 1 | 1 |
| 0 | 0 | 0 |
| 1 | 0 | 0 |
| 1 |-2 | 1 |
| 1 | 5 |-6 |
| 2 |-10| 12|
| 2 | 1 |-1 | 
100 раз. То есть один тест - решение 700 квадратных уравнений. 

Этот тест запускался 10 раз. Время - среднее на одном тесте

### Результат ###

Время указано в миллисекундах 

| Программа             | Soft CPU    | JIT         | Коэффициент ускорения  |
|-----------------------|-------------|-------------|------------------------|
| Числа Фибоначчи       | 16595 +- 1  | 92.2 +-0.08 |  180                   |
| Квадратное уравнение  | 7.3 +- 0.1  | 4.7 +- 0.09 |  1.55                  |

Таким образом, использовании технологии JIT полностью себя оправдало

