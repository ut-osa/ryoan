#!/bin/bash

cd $(dirname $0)
export argline="theano_example.py"
exec ../ryoan_run.py "$@"
