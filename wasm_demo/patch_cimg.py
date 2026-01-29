#!/usr/bin/env python3
"""Patch CImg.h to fix OpenMP configuration for WebAssembly builds."""

import sys

def patch_cimg():
    try:
        with open('CImg.h', 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print("Error: CImg.h not found")
        return 1

    # The problematic block that forces OpenMP on when cimg_use_openmp is defined
    old_block = '''#if !defined(cimg_use_openmp)
#ifdef _OPENMP
#define cimg_use_openmp 1
#else
#define cimg_use_openmp 0
#endif
#else
#undef cimg_use_openmp
#define cimg_use_openmp 1
#endif'''

    # New block that respects explicit cimg_use_openmp=0 definition
    new_block = '''#if !defined(cimg_use_openmp)
#ifdef _OPENMP
#define cimg_use_openmp 1
#else
#define cimg_use_openmp 0
#endif
#endif'''

    if old_block in content:
        content = content.replace(old_block, new_block)
        with open('CImg.h', 'w') as f:
            f.write(content)
        print("✓ CImg.h patched successfully!")
        return 0
    elif new_block in content:
        print("✓ CImg.h already patched")
        return 0
    else:
        print("✗ Could not find the expected block in CImg.h")
        return 1

if __name__ == '__main__':
    sys.exit(patch_cimg())

