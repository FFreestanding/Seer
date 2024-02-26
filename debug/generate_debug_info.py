import os
import subprocess
import math
import re
import time
import copy
from struct import *

pattern_func_name = re.compile(r"(\s+<[\d|\w]+>\s+DW_AT_name\s+: \(.*\): )(?P<func_name>[a-zA-Z_]+).*")
pattern_func_addr = re.compile(r"(\s+<[\d|\w]+>\s+DW_AT_low_pc\s+: )(?P<func_addr>0x\w+)")
pattern_file_name_line = re.compile(r"(?P<func_file_name>\w+.c)\s+(?P<func_file_line>\d+)\s+(?P<line_addr>0x[\d|\w]+)")


class line_info:
    line_number: int
    line_address: int
    line_func_name: str
    def __init__(self):
        self.line_number = 0
        self.line_address = 0
        self.line_func_name = "\0"


class debug_info:
    file_name: str
    file_lines: list
    def __init__(self):
        self.file_name = "\0"
        self.file_lines = []


info_objects_arry = []

def get_name_addr():
    global info_objects_arry
    tmp_obj = debug_info()
    result = subprocess.run(["objdump", "--dwarf=info", "./build/bin/Seer.bin"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print(result.stderr)
        return
    flag = 0
    output = result.stdout.decode()
    # results=re.findall(r"DW_TAG_subprogram.*(\n.*){2}: (?P<func_name>\w+)(\n.*){5}.*: (?P<func_addr>0x[\w|\d]+)",output)
    results=re.findall(r"DW_TAG_subprogram.*(\n.*){2}: (?P<func_name>\w+)(\n\s+<[\d|\w]+>\s+\w+\s+: \d+)+\n\s+<[\d|\w]+>\s+DW_AT_low_pc\s+: (?P<func_addr>0x[\w|\d]+)",output)
    # DW_TAG_subprogram.*(\n.*){2}: \w+(\n\s+<[\d|\w]+>\s+\w+\s+:\s\d+)+\n\s+<[\d|\w]+>\s+DW_AT_low_pc\s+:\s(0x[\d|\w]+)
    for i in info_objects_arry:
        for l in i.file_lines:
            for r in results:
                if hex(l.line_address) == r[3]:
                    l.line_func_name = r[1]+"\0"

def get_file_name_line():
    global info_objects_arry
    result = subprocess.run(["objdump", "--dwarf=decodedline", "./build/bin/Seer.bin"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print(result.stderr)
        return
    output = result.stdout.decode().split("\n")
    tmp_line_obj = line_info()
    tmp_file_obj = debug_info()
    for o in output:
        if "-" not in o and ".c" in o and "0x" in o:
            tmp_line_obj.line_number = int(pattern_file_name_line.match(o).group("func_file_line"))
            tmp_line_obj.line_address = int(pattern_file_name_line.match(o).group("line_addr"),16)
            if tmp_file_obj.file_name == "\0":
                tmp_file_obj.file_name = pattern_file_name_line.match(o).group("func_file_name")+"\0"
                tmp_file_obj.file_lines.append(copy.deepcopy(tmp_line_obj))
            elif tmp_file_obj.file_name == pattern_file_name_line.match(o).group("func_file_name")+"\0":
                tmp_file_obj.file_lines.append(copy.deepcopy(tmp_line_obj))
            elif tmp_file_obj.file_name != pattern_file_name_line.match(o).group("func_file_name")+"\0":
                info_objects_arry.append(tmp_file_obj)
                tmp_file_obj = debug_info()
                tmp_file_obj.file_name = pattern_file_name_line.match(o).group("func_file_name")+"\0"
                tmp_file_obj.file_lines.append(copy.deepcopy(tmp_line_obj))
            else:
                print("error")
                return

    info_objects_arry.append(tmp_file_obj)

if __name__ == "__main__":
    get_file_name_line()
    get_name_addr()
    buffer=bytearray()
    file_info_entries = bytearray()
    file_info_entries.extend(pack("I", len(info_objects_arry)))
    for file_info_entry in info_objects_arry:
        f_n = file_info_entry.file_name
        file_name_len = len(f_n)
        buffer.extend(pack("H", file_name_len))
        buffer.extend(f_n.encode())
        buffer.extend(pack("I", len(file_info_entry.file_lines)))
        for line_info_entry in file_info_entry.file_lines:
            buffer.extend(pack("H", line_info_entry.line_number))
            buffer.extend(pack("I", line_info_entry.line_address))
            buffer.extend(pack("H", len(line_info_entry.line_func_name)))
            buffer.extend(line_info_entry.line_func_name.encode())
        file_info_entries.extend(buffer)
        buffer = bytearray()
    info_bytes = bytearray()
    info_bytes.extend(pack("I", len(file_info_entries)))
    # print(info_bytes)
    with open("./build/iso/boot/sym_mod.bin", mode='wb') as f:
        f.write(info_bytes+file_info_entries)

