#! /usr/bin/env python3

import re

fc36_regex = re.compile(r'^5\.[1-9]\d+\..*')
rhel8_regex = re.compile(r'^(?:4|5)\.\d+\..*')


def build_fc36_task_file(task_file, fc36):
    for line in task_file.readlines():
        if fc36_regex.match(line):
            fc36.write(line)


def main(task_file):
    fc36 = open('/fc36-tasks', 'w')
    rhel8 = open('/rhel8-tasks', 'w')

    for line in task_file.readlines():
        if fc36_regex.match(line):
            fc36.write(line)
            continue

        if rhel8_regex.match(line):
            rhel8.write(line)
            continue

        print(f'No builder for {line}')

    fc36.close()
    rhel8.close()


if __name__ == "__main__":
    with open('/build-tasks', 'r') as task_file:
        main(task_file)
