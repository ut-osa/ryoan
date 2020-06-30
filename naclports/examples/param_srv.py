#!/usr/bin/env python

import os

def env_or_def(string, default):
    return os.environ[string] if string in os.environ else default

naclports_root = env_or_def('naclports_root', "..")

def _(string):
    return os.path.join(naclports_root, string)

def launch_parameter_server():
    os.unlink("dump.rdb")
    param_server_cmd = [
        _('../nacl/redis-3.2.8/src/redis-server'),
    ]

    os.execv(param_server_cmd[0], param_server_cmd)

if __name__ == '__main__':
    launch_parameter_server()
