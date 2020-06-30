from my_lasagne import *
from my_nonlinear import softmax, rectify
import my_init as init
import numpy as np
init.set_rng(np.random.RandomState(12345))


def build_cnn(input_var=None, input_shape=(None, 3, 64, 64), classes=10):
    network = InputLayer(shape=input_shape, input_var=input_var)
    # cnn block 1
    network = Conv2DLayer(
            network, num_filters=96, filter_size=(5, 5), stride=(2, 2),
            nonlinearity=rectify,
            W=init.GlorotUniform())
    network = MaxPool2DLayer(network, pool_size=(3, 3), stride=(2, 2))
    # cnn block 2
    network = Conv2DLayer(
            network, num_filters=256, filter_size=(3, 3),
            nonlinearity=rectify)
    network = MaxPool2DLayer(network, pool_size=(3, 3), stride=(2, 2))
    # cnn block 3
    network = Conv2DLayer(
            network, num_filters=512, filter_size=(3, 3),
            nonlinearity=rectify)
    network = Conv2DLayer(
            network, num_filters=512, filter_size=(3, 3),
            nonlinearity=rectify)
    network = Conv2DLayer(
            network, num_filters=512, filter_size=(3, 3),
            nonlinearity=rectify)
    network = MaxPool2DLayer(network, pool_size=(3, 3), stride=(2, 2))
    # fc block
    network = DenseLayer(
            network,
            num_units=1000,
            nonlinearity=rectify)
    network = DenseLayer(
            network,
            num_units=1000,
            nonlinearity=rectify)
    network = DenseLayer(
            network,
            num_units=classes,
            nonlinearity=softmax)
    return network


def build_vgg(input_var, input_shape=(None, 3, 112, 112), classes=200):
    network = InputLayer(shape=input_shape, input_var=input_var)
    # cnn block 1
    network = Conv2DLayer(
            network, num_filters=64, filter_size=(3, 3), pad=1,
            nonlinearity=rectify,
            W=init.GlorotUniform())
    network = batch_norm(network)

    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))
    # cnn block 2
    network = Conv2DLayer(
            network, num_filters=64, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)

    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))
    # cnn block 3
    network = Conv2DLayer(
            network, num_filters=256, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)

    network = Conv2DLayer(
            network, num_filters=256, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)

    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))
    # cnn block 4
    network = Conv2DLayer(
            network, num_filters=512, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)

    network = Conv2DLayer(
            network, num_filters=512, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)

    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))
    # cnn block 5
    network = Conv2DLayer(
            network, num_filters=512, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)

    network = Conv2DLayer(
            network, num_filters=512, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)

    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))
    # fc block
    network = DenseLayer(
            network,
            num_units=1024,
            nonlinearity=rectify)
    network = DenseLayer(
            network,
            num_units=1024,
            nonlinearity=rectify)
    network = DenseLayer(
            network,
            num_units=classes,
            nonlinearity=softmax)
    return network


def build_alex(input_var, input_shape=(None, 3, 112, 112), classes=200):
    network = InputLayer(shape=input_shape, input_var=input_var)
    # cnn 1
    network = Conv2DLayer(
            network, num_filters=96, filter_size=(11, 11), pad=1,
            stride=(4, 4),
            nonlinearity=rectify,
            W=init.GlorotUniform())
    network = batch_norm(network)
    network = MaxPool2DLayer(network, pool_size=(3, 3), stride=(2, 2))
    # cnn 2
    network = Conv2DLayer(
            network, num_filters=256, filter_size=(5, 5), pad=2,
            nonlinearity=rectify)
    network = batch_norm(network)
    network = MaxPool2DLayer(network, pool_size=(3, 3), stride=(2, 2))
    # cnn 3
    network = Conv2DLayer(
            network, num_filters=384, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)
    # cnn 4
    network = Conv2DLayer(
            network, num_filters=384, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)
    # cnn 5
    network = Conv2DLayer(
            network, num_filters=256, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    network = batch_norm(network)
    network = MaxPool2DLayer(network, pool_size=(3, 3), stride=(2, 2))
    # fc block
    network = DenseLayer(
            network,
            num_units=1024,
            nonlinearity=rectify)
    network = DenseLayer(
            network,
            num_units=1024,
            nonlinearity=rectify)
    network = DenseLayer(
            network,
            num_units=classes,
            nonlinearity=softmax)
    return network
