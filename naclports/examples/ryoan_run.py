#!/usr/bin/env python3
import sys
import os
import subprocess
import re
import atexit
import argparse

parser = argparse.ArgumentParser(description='Run using Ryoan')
parser.add_argument('--debug', '-d', action='store_true')
parser.add_argument('--baseline', '-b', action='store_true')
parser.add_argument('--no-libc', dest='ryoan', action='store_false')
parser.add_argument('epochs', type=int, nargs='?')
parser.add_argument('--param_server', type=str, default='localhost:6379')
parser.add_argument('--port', '-p', type=int, default=6379)
parser.add_argument('--sp_drop', type=int, default=0)
parser.add_argument('--n_push', type=int, default=1)
parser.add_argument('--n_pull', type=int, default=1)
parser.add_argument('--ps_msec', type=int, default=-1)
parser.add_argument('--launch_server', '-l', action='store_true') 
parser.add_argument('--multi', '-m', action='store_true') 

param_server_process = None
param_server_log_file = 'param_server.log'

def kill_parameter_server():
    if param_server_process:
        param_server_process.terminate()

def choose_port():
    # default is 9000 + pid
    return str(9000 + int(os.getpid()))
def signal_handler(sig, stack):
    sys.exit(sig)

argline = None
if not 'argline' in os.environ:
   print("need env variable \"argline\" did you try to run me directly?" \
         "You shouldn't do that.")
   sys.exit(1)
else:
    argline = os.environ['argline']

def env_or_def(string, default):
    return os.environ[string] if string in os.environ else default

def _(string):
    return os.path.join(naclports_root, string)

# allow the caller to override these
naclports_root = env_or_def('naclports_root', "../..")
nacl_build = env_or_def('nacl_build', _('../nacl/native_client/scons-out'))
pyroot = env_or_def('pyroot', _('../nacl/root/lib/python3.5/site-packages'))

def build_commandline(debug, baseline, ryoan, nacl_args):
   if ryoan:
      opt_binary = os.path.join(nacl_build, 'opt-ryoan-x86-64/staging/sel_ldr')
      dbg_binary = os.path.join(nacl_build, 'dbg-ryoan-x86-64/staging/sel_ldr')
   else:
      opt_binary = os.path.join(nacl_build, 'opt-linux-x86-64/staging/sel_ldr')
      dbg_binary = os.path.join(nacl_build, 'dbg-linux-x86-64/staging/sel_ldr')

   if debug:
      return ['cgdb', '--args', dbg_binary, *nacl_args]
   elif baseline:
      os.environ['PYTHONPATH'] = pyroot
      return ['python3', *nacl_args[4:]]
   else:
      return [opt_binary,  *nacl_args]

def run(debug, baseline, ryoan, epochs, param_server_string):
   arglines = argline.split()
   if param_server_string:
       nacl_args = ['--param_server', param_server_string, '--sp_drop', str(args.sp_drop),
                    '--n_push', str(args.n_push), '--n_pull', str(args.n_pull),
                    '--param-update-msec', str(args.ps_msec)]
   else:
       nacl_args = []
   if not ryoan:
       nacl_args.extend(['--no-sgx-perf', '--no-encrypt'])
   nacl_args.extend(['-P', choose_port(), '-ap', _('out/root/bin/python3.nexe'), *arglines])
   commandline = build_commandline(debug, baseline, ryoan, nacl_args)
   if epochs:
      commandline.append(str(epochs))

   os.environ['NACLVERBOSITY'] = '-10'
   res = subprocess.run(commandline)
   return res.returncode

def launch_parameter_server(port, baseline):
    global param_server_process
    ready_re = re.compile(r'.+The server is now ready to accept connections.+')
    log = open(param_server_log_file, 'wb')
    if baseline:
        os.environ['REDIS_NOENC'] = '1'

    param_server_cmd = [
        _('../nacl/redis-3.2.8/src/redis-server'), '--port', str(port), '--protected-mode no',
    ]

    param_server_process = subprocess.Popen(param_server_cmd,
                                            stdout=log, stderr=log)

    # atexit.register(kill_parameter_server)
    log.close()
    log = open(param_server_log_file, 'r')
    found = False

    while not found:
        if param_server_process.poll():
            print('Parameter server failed to start, check',
                  param_server_log_file, 'for details')
            sys.exit(1)
        for line in log:
            if ready_re.match(line):
                found = True
                break

if __name__ == '__main__':
    args = parser.parse_args()
    if args.launch_server:
        launch_parameter_server(args.port, args.baseline)

    ret = run(args.debug, args.baseline, args.ryoan, args.epochs,
              args.param_server if args.multi else None)
    sys.exit(ret)
