#! /usr/bin/env python3.5

import sys
import theano
import theano.tensor as T
import numpy as np
import time
from collections import OrderedDict

import my_obj as objectives
import my_reg as regularization
import my_updates
from my_lasagne import get_output, get_all_params, get_all_layers, count_params, BatchNormLayer

from cifar_net import build_vgg as build_cnn
#from parameter_server import RedisParameterServer
from ChironClient import Client as RedisParameterServer


def iterate_minibatches(inputs, targets, batchsize, shuffle=False, augment=False):
    assert len(inputs) == len(targets)
    if shuffle:
        indices = np.arange(len(inputs))
        np.random.shuffle(indices)
    for start_idx in range(0, len(inputs) - batchsize + 1, batchsize):
        if shuffle:
            excerpt = indices[start_idx:start_idx + batchsize]
        else:
            excerpt = slice(start_idx, start_idx + batchsize)
        if augment:
            # as in paper :
            # pad feature arrays with 4 pixels on each side
            # and do random cropping of 32x32
            padded = np.pad(inputs[excerpt], ((0, 0), (0, 0), (4, 4), (4, 4)), mode='constant')
            random_cropped = np.zeros(inputs[excerpt].shape, dtype=np.float32)
            crops = np.random.random_integers(0, high=8, size=(batchsize, 2))
            for r in range(batchsize):
                if np.random.rand() < 0.5:
                    padded[r] = padded[r, :, :, ::-1]
                random_cropped[r, :, :, :] = padded[r, :, crops[r, 0]: (crops[r, 0] + 32),
                                                          crops[r, 1]: (crops[r, 1] + 32)]
            inp_exc = random_cropped
        else:
            inp_exc = inputs[excerpt]

        yield inp_exc, targets[excerpt]


def log(msg, f):
    f.write(msg)
    f.flush()


def get_bn_stats_updates(network):
    layers = get_all_layers(network)
    stats_updates = OrderedDict()
    for layer in layers:
        if isinstance(layer, BatchNormLayer):
            stats_updates.update(layer.stats_updates)
    return stats_updates


def train(num_trainer, trainer_id, bn=True, host='localhost', port=6379):
    # load data
    train_x, train_y = load_trainer_data(num_trainer, trainer_id)
    sys.stderr.write('trainer {} start training with {} data points\n'.format(trainer_id, len(train_x)))

    epochs = 100 + num_trainer * 2
    batch_size = 128

    # Prepare Theano variables for inputs and targets
    input_var = T.tensor4('inputs')
    target_var = T.ivector('targets')

    # Create neural network model
    sys.stderr.write("Building model...\n")
    network = build_cnn(input_var, bn=bn)

    sys.stderr.write("number of parameters in model: %d\n" % count_params(network, trainable=True))
    # Create a loss expression for training, i.e., a scalar objective we want
    # to minimize (for our multi-class problem, it is the cross-entropy loss):
    prediction = get_output(network)
    loss = objectives.categorical_crossentropy(prediction, target_var).mean()

    # add weight decay
    all_layers = get_all_layers(network)
    l2_penalty = regularization.regularize_layer_params(all_layers, regularization.l2) * 5e-4
    loss += l2_penalty

    # Create update expressions for training
    # Stochastic Gradient Descent (SGD) with momentum
    params = get_all_params(network, trainable=True)

    sh_lr = theano.shared(0.1)
    updates = my_updates.sgd(loss, params, learning_rate=sh_lr)
    if bn:
        stats_updates = get_bn_stats_updates(network)
        updates.update(stats_updates)

    name_idx = 0
    for k in updates:
        if k.name is not None:
            k.name = k.name + str(name_idx)
        else:
            k.name = 'param' + str(name_idx)
        name_idx += 1

    params = updates.keys()
    ps = RedisParameterServer(host + ':' + str(port))
    ps.send_params(params)

    rtn = dict([(p.name, updates[p] - p) for p in updates])
    rtn['loss'] = loss

    train_fn = theano.function([input_var, target_var], rtn, updates=updates)
    logger = open('./logs/cifar_trainer{}-{}.log'.format(num_trainer, trainer_id), 'w')
    test_prediction = get_output(network, deterministic=True)
    test_fn = theano.function([input_var], test_prediction)

    it = 0
    it_time = 0.
    it_cpu = 0

    for epoch in range(epochs):
        for batch in iterate_minibatches(train_x, train_y, batch_size, shuffle=True, augment=True):
            it += 1
            inputs, targets = batch
            start_time = time.time()
            start_cpu = time.process_time()

            # download params from server
            ps.recv_params(params)
            start_count = ps.recv_count()

            # calculate gradients and loss, updates params
            result = train_fn(inputs, targets)
            cost = result.pop('loss')

            # upload gradients
            staleness = ps.recv_count() - start_count
            ps.send_updates(result, staleness)

            it_cpu += time.process_time() - start_cpu
            it_time += time.time() - start_time
            log("Done training iteration {} with loss {} staleness {},"
                " wall took {:.3f}s, cpu took {:.3f}\n".format(it, cost, staleness,
                                                               it_time, it_cpu), logger)

            if it % 200 == 0 and trainer_id == num_trainer - 1:
                np.save('./params/{}trainer_cifar_pred{}.npy'.format(num_trainer, it), test_fn)
        curr_lr = sh_lr.get_value()
        sh_lr.set_value(curr_lr * 0.96)
        # if epoch in {30 + num_trainer * 2, 45 + num_trainer * 2}:
        #     curr_lr = sh_lr.get_value()
        #     curr_lr *= 0.1
        #     sh_lr.set_value(curr_lr)

    logger.close()



def load_trainer_data(num_trainer, trainer_id):
    data_dir = './data/'
    fname = data_dir + 'cifar_trainer{}-{}.npz'.format(num_trainer, trainer_id)
    with np.load(fname) as f:
        return f['arr_0'], f['arr_1']


if __name__ == '__main__':
    assert len(sys.argv) >= 3
    trainer_id = int(sys.argv[1])
    total_trainers = int(sys.argv[2])

    assert 0 <= trainer_id < total_trainers
    if len(sys.argv) == 5:
        host = sys.argv[3]
        port = int(sys.argv[4])
    else:
        host = 'localhost'
        port = 8000

    train(total_trainers, trainer_id, host=host, port=port)
