import os

# Define your customized C-style block header
HEADER = """/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

"""

# Target C source and header files
TARGET_EXTENSIONS = {'.c', '.h'}

# Files or directories to skip entirely
IGNORE_LIST = {'add_headers.py', '.git', '.github', 'build', 'obj', 'bin', '.ccls', '.ccls-cache'}

def add_header_to_files():
    modified_count = 0
    
    for root, dirs, files in os.walk('.'):
        # Modify dirs in-place to skip ignored directories
        dirs[:] = [d for d in dirs if d not in IGNORE_LIST]
        
        for file in files:
            if file in IGNORE_LIST:
                continue
                
            file_path = os.path.join(root, file)
            _, ext = os.path.splitext(file)
            
            if ext in TARGET_EXTENSIONS:
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                    
                    # Check if the header is already there to avoid duplicates
                    if "Copyright (c) 2026 Bruce Botes" in content:
                        print(f"⏩ Skipped (already has header): {file_path}")
                        continue
                    
                    # Prepend the C header to the existing content
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(HEADER + content)
                    
                    print(f"✅ Added header to: {file_path}")
                    modified_count += 1
                    
                except Exception as e:
                    print(f"❌ Error processing {file_path}: {e}")

    print(f"\n🎉 Done! Successfully added C headers to {modified_count} files.")

if __name__ == "__main__":
    add_header_to_files()
