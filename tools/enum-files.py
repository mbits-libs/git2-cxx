#!/usr/bin/env python

import os
import sys
import hashlib

escapes = {
    34	: '\\"',
    39	: '\'',
    92	: '\\\\',
    10	: '\\n',
    13	: '\\r',
    7	: '\\a',
    9	: '\t',
    11	: '\\v',
}

filelist = []
dirlist = []
for dirname in sys.argv[1:]:
    dirname = dirname.replace('\\', '/')
    if dirname[-1] == '/': dirname = dirname[:-1]
    for root, dirs, files in os.walk(dirname):
        root = root.replace('\\', '/')
        if root[-6:] == '/hooks' or root[-5:] == '/logs' : continue
        if len(root.split('/logs/', 1)) == 2: continue
        if not files:
            if not dirs:
                dirlist.append(root)
            continue
        filelist.extend(f'{root}/{filename}' for filename in files if filename not in ['exclude', 'description', 'COMMIT_EDITMSG'])

def is_binary(b):
    for byte in b:
        if byte > 127: return True
    return False

def string_view(b):
    output = '"'
    for byte in b:
        if byte in escapes:
            output += escapes[byte]
            continue
        if byte < 32:
            output += '\\{:03o}'.format(byte)
            continue
        output += chr(byte)
    output += '"sv'
    output = '\\n"\n"'.join(output.split('\\n'))
    if output[-6:] == '"\n""sv': output = output[:-6] + '"sv'
    return output

hashes = {}

for filename in filelist:
    with open(filename, 'rb') as stream:
        b = stream.read()
    
    sha = hashlib.sha256(b).hexdigest()
    if sha not in hashes:
        hashes[sha] = [(b, [filename])]
        continue

    found = False
    for entry in hashes[sha]:
        if entry[0] == b:
            entry[1].append(filename)
            found = True
            break
    if not found:
        hashes[sha].append([(b, [filename])])


text_files = {}
text_file_contents = {}
text_file_counter = 0

binary_files = {}
binary_file_contents = {}
binary_file_counter = 0

for sha in hashes:
    for entry in hashes[sha]:
        if is_binary(entry[0]):
            binary_file_counter += 1
            binary_file_contents[binary_file_counter] = '{{{}}}'.format(', '.join('0x{:02x}'.format(byte) for byte in entry[0]))
            for filename in entry[1]:
                binary_files[filename] = f'binary_{binary_file_counter}'
            continue

        text_file_counter += 1
        text_file_contents[text_file_counter] = string_view(entry[0])
        for filename in entry[1]:
            text_files[filename] = f'text_{text_file_counter}'
        continue

print('namespace file {')
for file_id in sorted(text_file_contents.keys()):
    contents = text_file_contents[file_id]
    if '\n' in contents:
        contents = '\n\t\t'.join(contents.split('\n'))
        print(f'	static constexpr auto text_{file_id} =\n		{contents};')
        continue
    print(f'	static constexpr auto text_{file_id} = {contents};')
for file_id in sorted(binary_file_contents.keys()):
    print(f'\n	static constexpr unsigned char binary_{file_id}[] = {binary_file_contents[file_id]};')
print('} // file\n')

print('constexpr std::string_view subdirs[] = {')    
for dirname in dirlist:
    print('\t{},'.format(string_view(dirname.encode('utf-8'))))

print('''};

constexpr file::text text[] = {''')

for filename in sorted(text_files.keys()):
    print('\t{{{}, file::{}}},'.format(string_view(filename.encode('utf-8')), text_files[filename]))

print('''};

constexpr file::binary binary[] = {''')

for filename in sorted(binary_files.keys()):
    print('\t{{{}, span(file::{})}},'.format(string_view(filename.encode('utf-8')), binary_files[filename]))

print('};')
