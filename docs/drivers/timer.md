# Timer Driver (PIT)

The Programmable Interval Timer (PIT) is used for timekeeping and preemption.

## 1. Hardware Details

- **Frequency**: 1.193182 MHz.
- **IRQ**: 0 (Interrupt Vector 32).
- **Ports**: `0x40` (Channel 0), `0x43` (Mode/Command Register).

## 2. Implementation

- `timer_init(uint32_t frequency)`: Configures the PIT to fire interrupts at a specific rate (e.g., 1000Hz = every 1ms).
- `onIrq0(InteruptReg *regs)`: The interrupt handler called on every tick.
  - Increments a global `ticks` counter.
  - Iterates through the active **Triggers**.

---

## 3. Trigger System

TilekarOS includes a "Trigger" system to schedule periodic tasks without complex multitasking overhead.

### Creating a Trigger
`insert_triger(tick_mod, func, flags)`: Registers a function `func` to be called every `tick_mod` ticks.

- **Flags**:
  - `TIMER_TRIGGER_ACTIVE`: The trigger is currently running.
  - `TIMER_TRIGGER_USE_GLOBAL`: Use the absolute system `ticks` instead of a local counter.

### Example
The scheduler uses a trigger to preempt tasks every 100ms:
```c
scheduler_trigger_index = insert_triger(100, &task_yield, 0);
```

---

## 4. Timekeeping

The timer provides basic timekeeping through:

- `get_ticks()`: Returns the number of milliseconds since boot.
- `set_ticks(uint32_t)`: Manually updates the tick counter.
