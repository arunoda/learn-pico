import random
import os
import time

# Configure the size
SIZE_MB = 8
size = SIZE_MB * 1024 * 1024
chunk_size = 16 * 1024  # 16KB chunks for better performance

print(f"Generating {SIZE_MB}MB of random data...")
start_time = time.time()

# Generate header file start
with open("random_data.h", "w") as f:
    f.write("#ifndef RANDOM_DATA_H\n#define RANDOM_DATA_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"#define RANDOM_DATA_SIZE ({SIZE_MB} * 1024 * 1024)  // {SIZE_MB}MB\n\n")
    f.write("const uint8_t random_data[RANDOM_DATA_SIZE] = {\n")

# Generate data in chunks to avoid memory issues
total_written = 0
for chunk_start in range(0, size, chunk_size):
    # Generate this chunk of random data
    chunk_end = min(chunk_start + chunk_size, size)
    chunk_size_actual = chunk_end - chunk_start
    data = [random.randint(0, 255) for _ in range(chunk_size_actual)]
    
    
    # Convert to hex string
    hex_data = ", ".join([f"0x{b:02x}" for b in data])
    
    # Add comma if not the last chunk
    if chunk_end < size:
        hex_data += ","
    
    # Append to file
    with open("random_data.h", "a") as f:
        f.write("    " + hex_data + "\n")
    
    # Update progress
    total_written += chunk_size_actual
    if total_written % (1024 * 1024) == 0 or total_written == size:
        mb_written = total_written / (1024 * 1024)
        elapsed = time.time() - start_time
        print(f"Written {mb_written:.1f}MB of {SIZE_MB}MB ({mb_written/SIZE_MB*100:.1f}%) in {elapsed:.1f} seconds")

# Finish the header file
with open("random_data.h", "a") as f:
    f.write("};\n\n#endif // RANDOM_DATA_H\n")

total_time = time.time() - start_time
file_size = os.path.getsize("random_data.h") / (1024 * 1024)
print(f"Done! Generated {SIZE_MB}MB of random data in {total_time:.1f} seconds")
print(f"Header file size: {file_size:.2f}MB")
