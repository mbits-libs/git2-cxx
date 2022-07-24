import sys

filename = sys.argv[1]

with open(filename) as infile:
    contents = infile.readlines()

for index in range(len(contents)):
    if contents[index][:3] == "// ":
        continue
    contents.insert(index, "// @generated\n")
    break

with open(filename, "w") as outfile:
    outfile.write("".join(contents))
