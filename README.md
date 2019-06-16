# Minimal Agnostic General Executable

Work in progress.

## Limitations

- Executables are loaded into the process space of the loader
- Only recognizes .text section
- Only supports position independent code
- Does not support exports or debugging symbols
- Does not safely handle malformed files