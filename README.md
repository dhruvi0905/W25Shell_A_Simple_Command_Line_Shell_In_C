# w25Shell - A Simple UNIX-like Shell in C

## About
A lightweight Unix-style shell in C supporting standard and reverse piping, I/O redirection, conditional (&&, ||) and sequential command execution, along with custom file operations. Includes process management commands and strong error handling for a smooth CLI experience.

## Features
- Standard piping (`|`) and reverse piping (`=`)
- Input and output redirection (`<`, `>`, `>>`)
- Sequential command execution (`;`)
- Conditional execution (`&&`, `||`)
- Word count in files with `# filename`
- Appending two files together with `file1 ~ file2`
- Concatenating multiple files using `file1 + file2 + file3`
- Process management commands:
  - `killterm` : Kill current shell terminal
  - `killallterms` : Kill all running instances of the shell
- Robust error handling
- Dynamic prompt name based on executable

## Requirements
- GCC compiler
- Unix/Linux-based operating system

## How to Compile
```bash
gcc -o w25shell w25shell_dhruvi_dobariya_110159758.c
```

## How to Run
```bash
./w25shell
```

## Usage
- Enter commands just like a normal shell.
- Use special operators like `|`, `=`, `&&`, `||`, `;`, `<`, `>`, and `>>`.
- Use `killterm` to exit the shell or `killallterms` to kill all shell instances.
- Supports file operations and command chaining.

## Notes
- Designed for educational purposes to understand shell internals.
- Ensure proper permissions when operating on files and processes.
- Redirection and piping support up to 5 commands.

## License
Open-source project for learning and academic use.

---

*Developed by Dhruvi Dobariya*

