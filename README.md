
In this link, I describe a new approach for compiling C++.
 https://github.com/HassanSajjad-302/HMake/wiki/HMake-Flaw-And-Its-Possible-Fixes#5

Compared to the current approach,
this results in better compilation speed for the following reasons:

## No Scanning
The current approach for compiling C++20 modules / header-units is to first scan them 
for dependencies.
Then build these files in the topological order.
In the new approach that I presented, scanning is not needed.
I performed a few tests to measure the possible impact of this on the compilation speed-up.

1) I did the test with LLVM code-base. 
I performed the test with this commit
https://github.com/llvm/llvm-project/commit/58de24ebbb352a2345de3ebe280db98468ae96e3.
After cloning, I configured the project with CMake for Ninja in Debug Configuration.
Then I ran the ```ninja -nv > "ninja-nv.txt"``` in this build-dir.
This does dry-run build with verbose output and saves it in ```ninja-nv.txt``` file.
In this output, I replaced all instances of ```-std:c++17``` with ```-std:c++20```.
And removed all instances of ```/showIncludes ``` to keep the output clean.
You can see the contents of it in this repo.
Then I built the project to create all the required directories,
otherwise, the commands outputted in the ```ninja-nv.txt``` would not work.
Now, I compiled the ```main.cpp``` and ran the generated exe in the build-dir.
You can see the sample output in ```llvm-output.txt```.
The executable reads the ```ninja-nv.txt``` file,
and weeds out all other commands except the compiler-commands.
Now it runs the first 100 and last 100 of these commands with ```/scanDependencies```
appended to the command.
This way test completes quickly and gives a good estimate about the whole llvm repo.
The compilation-units in the first 100 commands have the least dependencies while the files
in the last 100 have the most.
The exe then compiles all these files to calculate what % is the scanning of the total
process.
It is **8.46%**. Please see the ```llvm-output.txt```.

This test does not paint the complete picture. 
As we are compiling files the old way.

2) I compiled SFML with C++20 header-units https://github.com/HassanSajjad-302/SFML.
I used this to measure the % of scanning time in total compilation time in
a project using C++20 header-units.
I compiled the ```main2.cpp``` and ran the generated exe in build-dir/drop-in.
The executable reads ```.response``` and ```.smrules.response``` from all subdirectories.
It erases ```/showIncludes``` from these files to keep output clean.
It then first scans and then builds using these response files.
You can see the output in the ```sfml-output.txt``` file in this repo.
Scanning takes on average **25.72%** of total compilation time per unit. 
That is because with header-units compilation becomes fast, 
thus scanning taking a bigger portion of the total compilation time.
For a few files scanning takes more time than compilation.


3) What will be the average time of scanning a file in a project size of LLVM. 
To measure this, I little modified the ```main.cpp``` to ```main3.cpp```.
It just scans, but does not compile.
Run the executable in the build-dir.
This scans 2000/2854 files.
The average time of scanning is **217.9ms**. 
For few files, it is more than **400ms**.
You can see this in ```llvm-2000-scanning.txt```.


## Files Caching

In the solution I presented, the compiler is loaded as a DLL.
And the compiler interacts with the build-system when it has to read a file
from the disk.
The build-system can cache the file so that it is not read twice from the disk.
This is specifically helpful in the case of modules / header-files.

Consider the following structure.

a.cpp includes a.hpp.

b.cpp includes both a.hpp and b.hpp.

c.cpp includes thrice of a.hpp, b.hpp and c.hpp

d.cpp includes b.hpp and d.hpp

In Unity build, often used today in bigger projects,
1 process is needed to amalgamate all 4 files.
1 process is needed to compile this amalgamated file. 
This reads 4(not considering amalgamated file as one) + all header-files(3)
So, in total 2 processes with 12 file reads.

In Conventional Build 4 processes are launched with a total of 12 file reads.
2 for a.cpp, 3 for b.cpp, 4 for c.cpp, 3 for d.cpp

In the modules / header-units build however 8 processes are launched with 22 file reads.
1 for a.hpp, 2(a.hpp.ifc + a.cpp) for a.cpp, 2 for b.hpp, 3 for b.cpp, 
3 for c.hpp, 4 for c.cpp, 3 for d.hpp, 4 for d.cpp

I measured the number of includes per file in the LLVM project.
To reproduce, Compile ```main4.cpp``` and run the exe in the build-dir.
You can find the sample output in ```includes-output.txt```.
On average, the LLVM source-file includes **400** header-files.
And there are a total of 3598 unique header-files.
If this project is built with C++20 drop-in header-unit replacement,
there will be 2854 + 3598 process launches for compilation instead of 2854. 
The total number of files read also increases.
If the ```two-phase``` model is to be supported as well,
then the amount of processes becomes 2854 + (2 * 3598).
Discussed here https://gitlab.kitware.com/cmake/cmake/-/issues/18355#note_1329192.

I think avoiding these costs of process setup and file reads should result in a 1-2% of
compilation speed-up in a clean build in the project size of LLVM.

Other very small improvements:
i) Rebuild is faster as there is no smrules file needed to be checked for an update.
ii) No response file is needed on Windows
iii) header includes are directly returned to the build-system instead of the  build-system
parsing a file or command-line output.

## Conclusion

Because of the reasons mentioned above,
I conjecture that using this approach should result in a **25-40%** improvement in
fully header-units optimized clean build of a mega project.
While the clean build of a C++ mega project using modules should be **3-5%** faster.
Compile speed should not deteriorate using this approach in any case.
I think the compiler and build-system can support both of these simultaneously.