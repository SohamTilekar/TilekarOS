# Global Descriptor Table (GDT) & Task State Segment (TSS)

The GDT is a fundamental x86 structure that defines memory segments and their access rights. TilekarOS uses a **Flat Memory Model**, where segments cover the entire 4GB address space.

## 1. Segment Descriptors

TilekarOS defines five main descriptors in the GDT:

| Index | Offset | Description | Base | Limit | Privilege |
|-------|--------|-------------|------|-------|-----------|
| 0     | 0x00   | Null        | -    | -     | -         |
| 1     | 0x08   | Kernel Code | 0    | 4GB   | Ring 0    |
| 2     | 0x10   | Kernel Data | 0    | 4GB   | Ring 0    |
| 3     | 0x18   | User Code   | 0    | 4GB   | Ring 3    |
| 4     | 0x20   | User Data   | 0    | 4GB   | Ring 3    |
| 5     | 0x28   | TSS         | &tss | size  | Ring 3    |

### Segment Format
Each GDT entry is 8 bytes and includes the **Base Address** (32-bit), **Segment Limit** (20-bit), and control bits:

- **Access Byte**: 
    - **P (Present)**: Must be 1.
    - **DPL (Privilege)**: 00 for Kernel, 11 for User.
    - **Type**: Code (0x18) or Data (0x10).
- **Flags**:
    - **G (Granularity)**: 1 for 4KB blocks (allows 4GB limit).
    - **D (Size)**: 1 for 32-bit protected mode.

---

## 2. Task State Segment (TSS)

While TilekarOS uses software-based multitasking, it still requires one hardware TSS to handle privilege transitions (e.g., from User Mode to Kernel Mode).

### Why do we need it?
When an interrupt occurs while the CPU is in Ring 3, it needs to switch to a safe Ring 0 stack. The CPU looks up the `esp0` field in the TSS to find the kernel stack pointer for the current task.

### Implementation
- **`TSSEntry` Structure**: Contains pointers for all privilege levels (`esp0`, `esp1`, `esp2`) and current registers. TilekarOS primarily uses the `esp0` and `ss0` fields.
- **`tss_entry`**: A global structure containing the `esp0` and `ss0` values.
- **`tss_set_kernel_stack(uint32_t stack_base)`**: Updates `esp0` during every context switch so that the CPU always uses the correct kernel stack for the active task.

## 3. Loading the GDT

The GDT is loaded into the CPU using the `lgdt` instruction. After loading, the segment registers (`cs`, `ds`, `es`, `fs`, `gs`, `ss`) must be reloaded to point to the new descriptors.

```c
void init_gdt() {
  gdt_load_register(&gdt_discriptor);
  gdt_install_tss();
  tss_load_register();
}
```
