DEBUG   = -D _DEBUG -ggdb3 -std=c++2a -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -Wstack-usage=8192 -pie -fPIE -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,nonnull-attribute,leak,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr
RELEASE = -O1 -g

all: translation_debug command_system_debug stdlib_debug create_dir

debug: translation_debug command_system_debug stdlib_debug create_dir
	g++ $(DEBUG) Src/main.cpp Obj/CommandSystem.o Obj/Stdlib.o Obj/Translation.o -o Exe/Translate
release: translation_release command_system_release stdlib_release create_dir
	g++ $(RELEASE) Src/main.cpp Obj/CommandSystem.o Obj/Stdlib.o Obj/Translation.o -o Exe/Translate

translation_debug: create_dir 
	g++ -c $(DEBUG) Src/Translation/Translation.cpp -o Obj/Translation.o 
translation_release: create_dir
	g++ -c $(RELEASE)   Src/Translation/Translation.cpp -o Obj/Translation.o

command_system_debug: create_dir
	g++ -c $(DEBUG)   Src/CommandSystem/CommandSystem.cpp -o Obj/CommandSystem.o 
command_system_release: create_dir
	g++ -c $(RELEASE) Src/CommandSystem/CommandSystem.cpp -o Obj/CommandSystem.o 

stdlib_debug: create_dir
	g++ -c $(DEBUG)   Src/Stdlib/Stdlib.cpp -o Obj/Stdlib.o
stdlib_release: create_dir
	g++ -c $(RELEASE) Src/Stdlib/Stdlib.cpp -o Obj/Stdlib.o

update_submodules:
	git submodule update --init --recursive --remote

run_lang_with_proc:
	cd Src/MyLanguage && make run

run_lang: 
	cd Src/MyLanguage && make compile FILE="../../$(FILE)"

run: debug
	./Exe/Translate a.sy

create_dir:
	-mkdir Exe
	-mkdir Obj