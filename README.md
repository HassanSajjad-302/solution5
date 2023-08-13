
In this link I describe a new approach for compiling C++.
 https://github.com/HassanSajjad-302/HMake/wiki/HMake-Flaw-And-Its-Possible-Fixes#5

Compared to the current approach,
this results in better compilation speed for the following reasons

## 1
The current approach for compiling C++20 modules / header-units is to first scan them 
for dependencies.
Then build these files in the topological order.
In the new approach that I presented, scanning is not needed.
I performed few tests to measure the possible impact of this on the compilation speed-up.

I did the test with LLVM code-base. 
I performed the test with this commit
https://github.com/llvm/llvm-project/commit/58de24ebbb352a2345de3ebe280db98468ae96e3.
After cloning, I configured the project with CMake for Ninja in Debug Configuration.
Then I ran the ```ninja -nv > "ninja-nv.txt"``` in this build-dir.
This does dry-run build with verbose output and saves it in ```ninja-nv.txt``` file.
In this output, I replaced all instances of ```-std:c++17``` with ```-std:c++20```.
This file is also copied in this repo.
Then I built the project to create all the required directories,
otherwise the commands outputted in the ```ninja-nv.txt``` would not work.
Now, I compiled the ```main1.cpp```
I 






In this new approach this scanning could be avoided. 
This is described in the link.
The scanning is 

