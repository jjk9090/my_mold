# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/jjk/project/mold2/my_mold

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jjk/project/mold2/my_mold/build

# Include any dependencies generated for this target.
include CMakeFiles/mymold.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/mymold.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mymold.dir/flags.make

CMakeFiles/mymold.dir/input_sections.c.o: CMakeFiles/mymold.dir/flags.make
CMakeFiles/mymold.dir/input_sections.c.o: ../input_sections.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jjk/project/mold2/my_mold/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/mymold.dir/input_sections.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mymold.dir/input_sections.c.o   -c /home/jjk/project/mold2/my_mold/input_sections.c

CMakeFiles/mymold.dir/input_sections.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mymold.dir/input_sections.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jjk/project/mold2/my_mold/input_sections.c > CMakeFiles/mymold.dir/input_sections.c.i

CMakeFiles/mymold.dir/input_sections.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mymold.dir/input_sections.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jjk/project/mold2/my_mold/input_sections.c -o CMakeFiles/mymold.dir/input_sections.c.s

CMakeFiles/mymold.dir/input_file.c.o: CMakeFiles/mymold.dir/flags.make
CMakeFiles/mymold.dir/input_file.c.o: ../input_file.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jjk/project/mold2/my_mold/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/mymold.dir/input_file.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mymold.dir/input_file.c.o   -c /home/jjk/project/mold2/my_mold/input_file.c

CMakeFiles/mymold.dir/input_file.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mymold.dir/input_file.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jjk/project/mold2/my_mold/input_file.c > CMakeFiles/mymold.dir/input_file.c.i

CMakeFiles/mymold.dir/input_file.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mymold.dir/input_file.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jjk/project/mold2/my_mold/input_file.c -o CMakeFiles/mymold.dir/input_file.c.s

CMakeFiles/mymold.dir/output_chunks.c.o: CMakeFiles/mymold.dir/flags.make
CMakeFiles/mymold.dir/output_chunks.c.o: ../output_chunks.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jjk/project/mold2/my_mold/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/mymold.dir/output_chunks.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mymold.dir/output_chunks.c.o   -c /home/jjk/project/mold2/my_mold/output_chunks.c

CMakeFiles/mymold.dir/output_chunks.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mymold.dir/output_chunks.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jjk/project/mold2/my_mold/output_chunks.c > CMakeFiles/mymold.dir/output_chunks.c.i

CMakeFiles/mymold.dir/output_chunks.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mymold.dir/output_chunks.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jjk/project/mold2/my_mold/output_chunks.c -o CMakeFiles/mymold.dir/output_chunks.c.s

CMakeFiles/mymold.dir/xxhash.c.o: CMakeFiles/mymold.dir/flags.make
CMakeFiles/mymold.dir/xxhash.c.o: ../xxhash.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jjk/project/mold2/my_mold/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/mymold.dir/xxhash.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mymold.dir/xxhash.c.o   -c /home/jjk/project/mold2/my_mold/xxhash.c

CMakeFiles/mymold.dir/xxhash.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mymold.dir/xxhash.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jjk/project/mold2/my_mold/xxhash.c > CMakeFiles/mymold.dir/xxhash.c.i

CMakeFiles/mymold.dir/xxhash.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mymold.dir/xxhash.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jjk/project/mold2/my_mold/xxhash.c -o CMakeFiles/mymold.dir/xxhash.c.s

CMakeFiles/mymold.dir/passes.c.o: CMakeFiles/mymold.dir/flags.make
CMakeFiles/mymold.dir/passes.c.o: ../passes.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jjk/project/mold2/my_mold/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/mymold.dir/passes.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mymold.dir/passes.c.o   -c /home/jjk/project/mold2/my_mold/passes.c

CMakeFiles/mymold.dir/passes.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mymold.dir/passes.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jjk/project/mold2/my_mold/passes.c > CMakeFiles/mymold.dir/passes.c.i

CMakeFiles/mymold.dir/passes.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mymold.dir/passes.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jjk/project/mold2/my_mold/passes.c -o CMakeFiles/mymold.dir/passes.c.s

CMakeFiles/mymold.dir/main.c.o: CMakeFiles/mymold.dir/flags.make
CMakeFiles/mymold.dir/main.c.o: ../main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jjk/project/mold2/my_mold/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/mymold.dir/main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mymold.dir/main.c.o   -c /home/jjk/project/mold2/my_mold/main.c

CMakeFiles/mymold.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mymold.dir/main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jjk/project/mold2/my_mold/main.c > CMakeFiles/mymold.dir/main.c.i

CMakeFiles/mymold.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mymold.dir/main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jjk/project/mold2/my_mold/main.c -o CMakeFiles/mymold.dir/main.c.s

# Object files for target mymold
mymold_OBJECTS = \
"CMakeFiles/mymold.dir/input_sections.c.o" \
"CMakeFiles/mymold.dir/input_file.c.o" \
"CMakeFiles/mymold.dir/output_chunks.c.o" \
"CMakeFiles/mymold.dir/xxhash.c.o" \
"CMakeFiles/mymold.dir/passes.c.o" \
"CMakeFiles/mymold.dir/main.c.o"

# External object files for target mymold
mymold_EXTERNAL_OBJECTS =

../mymold: CMakeFiles/mymold.dir/input_sections.c.o
../mymold: CMakeFiles/mymold.dir/input_file.c.o
../mymold: CMakeFiles/mymold.dir/output_chunks.c.o
../mymold: CMakeFiles/mymold.dir/xxhash.c.o
../mymold: CMakeFiles/mymold.dir/passes.c.o
../mymold: CMakeFiles/mymold.dir/main.c.o
../mymold: CMakeFiles/mymold.dir/build.make
../mymold: CMakeFiles/mymold.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jjk/project/mold2/my_mold/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Linking C executable ../mymold"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mymold.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mymold.dir/build: ../mymold

.PHONY : CMakeFiles/mymold.dir/build

CMakeFiles/mymold.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mymold.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mymold.dir/clean

CMakeFiles/mymold.dir/depend:
	cd /home/jjk/project/mold2/my_mold/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jjk/project/mold2/my_mold /home/jjk/project/mold2/my_mold /home/jjk/project/mold2/my_mold/build /home/jjk/project/mold2/my_mold/build /home/jjk/project/mold2/my_mold/build/CMakeFiles/mymold.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/mymold.dir/depend
