# Storage and Filesystem Stack

TilekarOS features a modular storage stack that abstracts hardware details from the application layer.

## Layers

### 1. Device Layer (`devices.h`)
The base layer provides a uniform interface for all hardware.
- **`device_t`**: Abstract structure for Block, Character, and Network devices.
- **Dynamic Registry**: Devices are registered at boot and stored in a dynamic linked list.

### 2. Buffer Cache (`buffer.h`)
A middleware layer that caches disk sectors in memory to reduce physical I/O.
- **`buffer_get`**: Retrieves a sector, loading it from the device if not cached.
- **`buffer_release`**: Decrements reference count.
- **`buffer_flush`**: Writes dirty buffers back to the device.

### 3. Virtual File System (`vfs.h`)
The VFS abstracts different filesystem formats (like FAT) behind a common API.
- **`vnode_t`**: Represents a file or directory object.
- **`file_t`**: Represents an open file instance with a seek position.
- **Path Resolution**: Handles traversing nested directories (e.g., `/bin/sh`).

### 4. FAT Filesystem (`fat.h`)
Implementation of the FAT12/16 specification.
- Supports files, subdirectories, and cluster chain management.
- Integrated into VFS via `fat_mount`.

## Syscalls
Applications interact with the storage stack using standard POSIX-like syscalls:
- `open(path, flags)`: Returns a file descriptor.
- `read(fd, buf, size)`: Reads data from a file.
- `write(fd, buf, size)`: Writes data to a file (or terminal).
- `close(fd)`: Closes a file descriptor.
- `mkdir(path)`: Creates a new directory.
- `readdir(fd, index, dirent)`: Enumerates directory contents.
- `unlink(path)`: Deletes a file.
- `rmdir(path)`: Deletes an empty directory.
