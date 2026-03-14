# Coding Style & Guidelines

To maintain a clean and consistent codebase, TilekarOS follows these guidelines.

## 1. C Language Style

- **Indentation**: 4 spaces (No tabs).
- **Naming**:
  - Functions: `snake_case()` (e.g., `terminal_putchar`).
  - Variables: `snake_case`.
  - Types: `snake_case_t` (e.g., `task_t`).
  - Macros/Constants: `UPPER_SNAKE_CASE`.
- **Braces**: K&R style (Brace on the same line for `if`/`while`, new line for functions).
- **Headers**: Use `#ifndef HEADER_NAME_H` guards.

## 2. Assembly Style

- **NASM**: Use Intel syntax.
- **Comments**: Use `;` for comments.
- **Organization**: Label global entry points with an underscore (e.g., `_start`).

## 3. Documentation

- Every new public function in the kernel should have a brief comment explaining its parameters and return value.
- Significant changes should be reflected in the relevant file in the `docs/` directory.

## 4. Git

- Keep commits small and focused.
- Use descriptive commit messages.
