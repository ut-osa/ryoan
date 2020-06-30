import sys
import theano
import theano.tensor as T
import numpy as np
import time
from collections import OrderedDict

import my_obj as objectives
import my_reg as regularization
import my_updates
from my_lasagne import get_output, get_all_params, get_all_layers, count_params
from my_lasagne import InputLayer, Conv2DLayer, MaxPool2DLayer, DenseLayer
import my_init as init
import my_nonlinear as nonlinearities

from parameter_server import RedisParameterServer


def iterate_minibatches(inputs, targets, batchsize, shuffle=False):
    assert len(inputs) == len(targets)
    if shuffle:
        indices = np.arange(len(inputs))
        np.random.shuffle(indices)
    for start_idx in range(0, len(inputs) - batchsize + 1, batchsize):
        if shuffle:
            excerpt = indices[start_idx:start_idx + batchsize]
        else:
            excerpt = slice(start_idx, start_idx + batchsize)
        yield inputs[excerpt], targets[excerpt]


def build_cnn(input_var=None):
    # Input layer, as usual:
    network = InputLayer(shape=(None, 1, 28, 28),
                         input_var=input_var)
    network = Conv2DLayer(
        network, num_filters=64, filter_size=(5, 5),
        nonlinearity=nonlinearities.tanh,
        W=init.GlorotUniform())
    # Max-pooling layer of factor 2 in both dimensions:
    network = MaxPool2DLayer(network, pool_size=(2, 2))

    # Another convolution with 32 5x5 kernels, and another 2x2 pooling:
    network = Conv2DLayer(
        network, num_filters=32, filter_size=(5, 5),
        nonlinearity=nonlinearities.tanh)
    network = MaxPool2DLayer(network, pool_size=(2, 2))

    # A fully-connected layer of 256 units with 50% dropout on its inputs:
    network = DenseLayer(
        network,
        num_units=512,
        nonlinearity=nonlinearities.tanh)

    # A fully-connected layer of 256 units with 50% dropout on its inputs:
    network = DenseLayer(
        network,
        num_units=256,
        nonlinearity=nonlinearities.tanh)

    # And, finally, the 10-unit output layer with 50% dropout on its inputs:
    network = DenseLayer(
        network,
        num_units=10,
        nonlinearity=nonlinearities.softmax)

    return network


def log(msg, f):
    f.write(msg)
    f.flush()


def train(num_trainer, trainer_id, host='localhost', port=6379):
    # load data
    train_x, train_y = load_trainer_data(num_trainer, trainer_id)
    sys.stderr.write('trainer {} start training with {} data points\n'.format(trainer_id, len(train_x)))

    epochs = 30
    batch_size = 128

    # Prepare Theano variables for inputs and targets
    input_var = T.tensor4('inputs')
    target_var = T.ivector('targets')

    # Create neural network model
    sys.stderr.write("Building model...\n")
    network = build_cnn(input_var)

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

    sh_lr = theano.shared(0.01)
    updates = my_updates.sgd(loss, params, learning_rate=sh_lr)

    name_idx = 0
    param_dict = OrderedDict()
    for k in updates:
        if k.name is not None:
            k.name = k.name + str(name_idx)
        else:
            k.name = 'param' + str(name_idx)
        name_idx += 1
        param_dict[k.name] = k.get_value()

    params = updates.keys()
    ps = RedisParameterServer(host=host, port=port)
    ps.send_params(param_dict)

    rtn = dict([(p.name, updates[p] - p) for p in updates])
    rtn['loss'] = loss

    train_fn = theano.function([input_var, target_var], rtn, updates=updates)
    logger = open('./logs/mnist_trainer{}-{}.log'.format(num_trainer, trainer_id), 'w')
    test_prediction = get_output(network, deterministic=True)
    test_fn = theano.function([input_var], test_prediction)

    it = 0
    it_time = 0.
    for epoch in range(epochs):
        for batch in iterate_minibatches(train_x, train_y, batch_size, shuffle=True):
            it += 1
            inputs, targets = batch
            start_time = time.time()

            # download params from server
            value_dict = ps.recv_params()
            start_count = ps.recv_count()

            # set the received params
            set_params(params, value_dict)

            # calculate gradients and loss, updates params
            result = train_fn(inputs, targets)
            cost = result.pop('loss')

            # upload gradients
            staleness = ps.recv_count() - start_count
            ps.send_updates(result, staleness)
            it_time += time.time() - start_time
            log("Done training iteration {} with loss {} staleness {},"
                " used {:.3f}s\n".format(it, cost, staleness, it_time), logger)

            if it % 50 == 0 and trainer_id == 0:
                np.save('./params/{}trainer_mnist_pred{}.npy'.format(num_trainer, it), test_fn)

    logger.close()


def set_params(params, values):
    for p in params:
        v = values[p.name]
        p_shape = p.get_value().shape
        p.set_value(v.reshape(p_shape))


def load_trainer_data(num_trainer, trainer_id):
    data_dir = './data/'
    fname = data_dir + 'mnist_trainer{}-{}.npz'.format(num_trainer, trainer_id)
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
        port = 6379

    train(total_trainers, trainer_id, host=host, port=port)
