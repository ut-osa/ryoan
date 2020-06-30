from my_lasagne import *
from my_nonlinear import softmax, rectify
import my_init as init
import numpy as np
init.set_rng(np.random.RandomState(12345))


def build_cnn(input_var=None, input_shape=(None, 3, 32, 32), classes=10):
    network = InputLayer(shape=input_shape, input_var=input_var)
    network = Conv2DLayer(
            network, num_filters=30, filter_size=(5, 5),
            nonlinearity=rectify,
            W=init.GlorotUniform())
    network = Conv2DLayer(
            network, num_filters=32, filter_size=(3, 3), pad=1,
            nonlinearity=rectify,
            W=init.GlorotUniform())

    network = MaxPool2DLayer(network, pool_size=(2, 2))

    network = Conv2DLayer(
            network, num_filters=30, filter_size=(5, 5), pad=1,
            nonlinearity=rectify)
    network = Conv2DLayer(
            network, num_filters=64, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)

    network = MaxPool2DLayer(network, pool_size=(2, 2))

    network = DenseLayer(
            network,
            num_units=500,
            nonlinearity=rectify)

    network = DenseLayer(
            network,
            num_units=classes,
            nonlinearity=softmax)
    return network


def build_nin(input_var=None):
    net = dict()
    net['input'] = InputLayer(shape=(None, 3, 32, 32), input_var=input_var)
    net['conv1'] = Conv2DLayer(net['input'],
                               num_filters=192,
                               filter_size=5,
                               pad=2,
                               flip_filters=False)
    net['cccp1'] = Conv2DLayer(
        net['conv1'], num_filters=160, filter_size=1, flip_filters=False)
    net['cccp2'] = Conv2DLayer(
        net['cccp1'], num_filters=96, filter_size=1, flip_filters=False)
    net['pool1'] = Pool2DLayer(net['cccp2'],
                               pool_size=3,
                               stride=2,
                               mode='max',
                               ignore_border=False)
    net['conv2'] = Conv2DLayer(net['pool1'],
                               num_filters=192,
                               filter_size=5,
                               pad=2,
                               flip_filters=False)
    net['cccp3'] = Conv2DLayer(
        net['conv2'], num_filters=192, filter_size=1, flip_filters=False)
    net['cccp4'] = Conv2DLayer(
        net['cccp3'], num_filters=192, filter_size=1, flip_filters=False)
    net['pool2'] = Pool2DLayer(net['cccp4'],
                               pool_size=3,
                               stride=2,
                               mode='average_exc_pad',
                               ignore_border=False)
    net['conv3'] = Conv2DLayer(net['pool2'],
                               num_filters=192,
                               filter_size=3,
                               pad=1,
                               flip_filters=False)
    net['cccp5'] = Conv2DLayer(
        net['conv3'], num_filters=192, filter_size=1, flip_filters=False)
    net['cccp6'] = Conv2DLayer(
        net['cccp5'], num_filters=10, filter_size=1, flip_filters=False)
    net['pool3'] = Pool2DLayer(net['cccp6'],
                               pool_size=8,
                               mode='average_exc_pad',
                               ignore_border=False)

    net['output'] = FlattenLayer(net['pool3'])
    network = DenseLayer(net['output'],
                         num_units=10,
                         nonlinearity=softmax)
    return network


def build_vgg(input_var=None, input_shape=(None, 3, 32, 32), classes=10, bn=True):
    network = InputLayer(shape=input_shape, input_var=input_var)
    # cnn block 1
    network = Conv2DLayer(
            network, num_filters=64, filter_size=(3, 3), pad=1,
            nonlinearity=rectify,
            W=init.GlorotUniform())
    if bn:
        network = batch_norm(network)
    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))

    # cnn block 2
    network = Conv2DLayer(
            network, num_filters=64, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    if bn:
        network = batch_norm(network)
    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))

    # cnn block 3
    network = Conv2DLayer(
            network, num_filters=128, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    if bn:
        network = batch_norm(network)
    network = Conv2DLayer(
            network, num_filters=128, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    if bn:
        network = batch_norm(network)
    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))
    # cnn block 4
    network = Conv2DLayer(
            network, num_filters=256, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    if bn:
        network = batch_norm(network)
    network = Conv2DLayer(
            network, num_filters=256, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    if bn:
        network = batch_norm(network)
    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))
    # cnn block 5
    network = Conv2DLayer(
            network, num_filters=256, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    if bn:
        network = batch_norm(network)
    network = Conv2DLayer(
            network, num_filters=256, filter_size=(3, 3), pad=1,
            nonlinearity=rectify)
    if bn:
        network = batch_norm(network)

    network = MaxPool2DLayer(network, pool_size=(2, 2), stride=(2, 2))
    # fc block
    network = DenseLayer(
            network,
            num_units=classes,
            nonlinearity=softmax)
    return network


def build_resnet(input_var=None, input_shape=(None, 3, 32, 32), n=5, classes=10, final_act=softmax):
    # create a residual learning building block with two stacked 3x3 convlayers as in paper
    def residual_block(l, increase_dim=False, projection=True):
        input_num_filters = l.output_shape[1]
        if increase_dim:
            first_stride = (2, 2)
            out_num_filters = input_num_filters * 2
        else:
            first_stride = (1, 1)
            out_num_filters = input_num_filters

        stack_1 = batch_norm(
            Conv2DLayer(l, num_filters=out_num_filters, filter_size=(3, 3), stride=first_stride, nonlinearity=rectify,
                        pad='same', W=init.HeNormal(gain='relu'), flip_filters=False))
        stack_2 = batch_norm(
            Conv2DLayer(stack_1, num_filters=out_num_filters, filter_size=(3, 3), stride=(1, 1), nonlinearity=None,
                        pad='same', W=init.HeNormal(gain='relu'), flip_filters=False))

        # add shortcut connections
        if increase_dim:
            if projection:
                # projection shortcut, as option B in paper
                projection = batch_norm(
                    Conv2DLayer(l, num_filters=out_num_filters, filter_size=(1, 1), stride=(2, 2), nonlinearity=None,
                                pad='same', b=None, flip_filters=False))
                block = NonlinearityLayer(ElemwiseSumLayer([stack_2, projection]), nonlinearity=rectify)
            else:
                # identity shortcut, as option A in paper
                identity = ExpressionLayer(l, lambda X: X[:, :, ::2, ::2], lambda s: (s[0], s[1], s[2] // 2, s[3] // 2))
                padding = PadLayer(identity, [out_num_filters // 4, 0, 0], batch_ndim=1)
                block = NonlinearityLayer(ElemwiseSumLayer([stack_2, padding]), nonlinearity=rectify)
        else:
            block = NonlinearityLayer(ElemwiseSumLayer([stack_2, l]), nonlinearity=rectify)

        return block

    # Building the network
    l_in = InputLayer(shape=input_shape, input_var=input_var)

    # first layer, output is 16 x 32 x 32
    l = batch_norm(Conv2DLayer(l_in, num_filters=16, filter_size=(3, 3), stride=(1, 1), nonlinearity=rectify, pad='same',
                               W=init.HeNormal(gain='relu'), flip_filters=False))

    # first stack of residual blocks, output is 16 x 32 x 32
    for _ in range(n):
        l = residual_block(l)

    # second stack of residual blocks, output is 32 x 16 x 16
    l = residual_block(l, increase_dim=True)
    for _ in range(1, n):
        l = residual_block(l)

    # third stack of residual blocks, output is 64 x 8 x 8
    l = residual_block(l, increase_dim=True)
    for _ in range(1, n):
        l = residual_block(l)

    # average pooling
    l = GlobalPoolLayer(l)

    # fully connected layer
    network = DenseLayer(
        l, num_units=classes,
        W=init.HeNormal(),
        nonlinearity=final_act)

    return network
