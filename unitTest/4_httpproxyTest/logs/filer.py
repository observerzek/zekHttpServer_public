import os
import re
import time

def getFileList():
    current_dir = os.getcwd()
    files = os.listdir(current_dir)
    file_list = []
    for i, file in enumerate(files):
        file_path = os.path.join(current_dir, file)
        if os.path.islink(file_path):
            continue

        stat_info = os.stat(file_path)
        create_time = time.ctime(stat_info.st_ctime)
        file_list.append([create_time, file])

    file_list = sorted(file_list, key = lambda x : x[0])
    return file_list


if __name__ == "__main__" :
    files_list = getFileList()
    for i, file in enumerate(files_list):
        print(f"[{i}]: {file[1]}")

    FILE_INDEX = input("请选择文件 : ")
    FILE_PATH = files_list[int(FILE_INDEX)][1]
    if not os.path.exists(FILE_PATH) :
        print("路径不存在")
        exit()
    file = open(FILE_PATH, 'r')

    while True :
        file.seek(0)
        filer_data = input("请输入过滤关键字: ")
        if filer_data == 'q':
            break

        pattern = re.compile(r'\(.+?\)')

        filer_keys = pattern.findall(filer_data)

        if len(filer_keys) == 0:
            print("输入格式错误")
            break


        lines = file.readlines()
        pair_data = r''
        for i, key in enumerate(filer_keys):
            if "&&" in key:
                keys = key.split("&&")
                pair_data += keys[0]
                pair_data += ".*"
                pair_data += keys[1]
            else:
                pair_data += key
            if i is not len(filer_keys) - 1:
                pair_data += '|'

        print(pair_data)
        count = 1
        for i, line in enumerate(lines):
            pair = re.compile(pair_data)
            matches = pair.search(line)
            if matches is not None:
                line = line.replace("\n", "")
                line = line.strip()
                print(f"{count} line:{i} {line}")
                count += 1
        
        