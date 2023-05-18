# Бинарный транслятор #

Эта программа осуществляет "перевод" команд [программного процессора](https://github.com/SimonaytesYan/SoftCPU) в команды x64 процессора и исполнение полученных инструкций.

Программный процессор имел стек вызовов, стек данных, RAM и 4 регистра. Для простоты трансляции, под стек вызовов и RAM были выделены отдельные области памяти. Регистр rsi стал стековым регистром для стека вызовов.

Для простоты обращения к RAM указатель на начало блока памяти, выделенного под неё, сохранён в rdi. Соответственно, все обращения к памяти должны происходить по смещению, хранящемуся в rdi.

Стандартная библиотека включающая в себя реализации инструкций in и out программного процессора, лежит во время исполнения программы в отдельном блоке памяти, на который указывает r15. (в разработке)

В программном процессоре существовали только регистры RAX, RBX, RCX и RDX. Они соответствуют таким же регистрам в x64. Эти 4 регистры мы будем называть пользовательскими регистрами. Остальные регистры x64 - системными.

Гарантируется, что в начале программы все пользовательские регистры равны нулю.

Компиляция осуществляется в два прохода, для обработки jmp вниз.
