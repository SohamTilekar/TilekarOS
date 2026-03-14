# Documentation Guide

This guide explains how to write and maintain documentation for the TilekarOS project.

## 1. Structure

Documentation is organized into logical sections:

- `getting-started/`: Installation, building, and running.
- `kernel/`: Core internals (memory, tasks, interrupts).
- `drivers/`: Hardware-specific implementation details.
- `api/`: Programmer-facing system calls and LibC reference.
- `development/`: Coding standards and contribution guides.

## 2. Mermaid Diagrams

TilekarOS uses the **default Mermaid theme** provided by MkDocs Material. This ensures that diagrams are automatically styled correctly for both light and dark modes without manual configuration.

### Best Practices

- **Keep it Simple**: Only use diagrams where they add significant value.
- **Quotes**: Always put labels in quotes to avoid syntax errors with special characters (e.g., `Node["Label with (Parentheses)"]`).
- **Standard Shapes**: Use standard shapes like `rect`, `circle`, and `subgraph`.

---

## 3. Writing Style

- **Be Concise**: Use high-signal language. Avoid fluff.
- **Code Blocks**: Always specify the language (e.g., ````c`, ````asm`, ````mermaid`).
- **Headings**: Use hierarchical headings (`#`, `##`, `###`).

## 4. Previewing Docs

TilekarOS uses **MkDocs** with the **Material** theme.

1. Install dependencies:
   ```bash
   pip install mkdocs-material
   ```
2. Serve the docs locally:
   ```bash
   mkdocs serve
   ```
3. Open `http://127.0.0.1:8000` in your browser.
