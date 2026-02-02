# VCC Compiler Collections

[How to Build](./docs/build.md) | [Standard and Semantics](./docs/standard.md) | [To Do](./docs/todo.md)

# Build Instruction 

## Windows

```
git clone --recursive https://github.com/Mr-Anyone/vcc-compiler-collection.git
cd vcc-compiler-collection
mkdir build && cmake .. -D CMAKE_BUILD_TYPE=Release -DCMAKE_GENERATOR_PLATFORM=x64
cmake --build . --target vcc --config Release  
```

** Currently Windows build but doesn't pass testcase. This is likely caused by the difference in filesystem. **

## Linux

```
git clone --recursive https://github.com/Mr-Anyone/vcc-compiler-collection.git
cd vcc-compiler-collection
cmake -B build  -G "Ninja" -DCMAKE_BUILD_TYPE=Release
```