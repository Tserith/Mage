# Minimal Agnostic General Executable

![](img/example.PNG)



## Limitations

- Executables are loaded into the process space of the loader
- Only recognizes .text section
- Only supports 64-bit object files with position independent code
- Only supports IMAGE_REL_AMD64_REL32 relocation type (functions)
- Does not support debugging symbols
- Does not support shared library creation
- Does not safely handle malformed files