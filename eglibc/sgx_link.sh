#!/bin/bash
sgx_dir=sgx_syscall_interpos
dirs="elf iconv locale catgets timezone"

for i in $sgx_dir/*.c $sgx_dir/*.S
do
    for d in $dirs
    do
        ln -fs ../$i $d/$(basename $i)
    done
done

for i in $sgx_dir/*.h
do
    ln -fs ../$i include/$(basename $i)
done
