import my_obj as objectives
import my_reg as regularization
import my_updates
from my_lasagne import get_output, get_all_params, get_all_layers, count_params, BatchNormLayer
from imagenet_net import build_vgg, build_alex
from parameter_server import RedisParameterServer
from imagenet_utils import load_batch_data, load_batch_path_label
from collections import OrderedDict

import theano
import sys
import time

import numpy as np
import theano.tensor as T


use_ryoan = hasattr(theano, 'trusted_build_network')
if use_ryoan:
    sys.stderr.write("Using Ryoan...\n")


def get_bn_stats_updates(network):
    layers = get_all_layers(network)
    stats_updates = OrderedDict()
    for layer in layers:
        if isinstance(layer, BatchNormLayer):
            stats_updates.update(layer.stats_updates)
    return stats_updates


def log(msg, f):
    f.write(msg)
    f.flush()


def main(num_trainer, trainer_id, batch_path, num_epochs=50, lr=5e-2, img_size=112, vgg=False,
         host='localhost', port=6379):

    # Load the dataset
    np.random.seed(12345)

    sys.stderr.write("Loading data...\n")
    n_batches = len(batch_path)
    classes = 200

    # Prepare Theano variables for inputs and targets
    input_var = T.tensor4('inputs')
    target_var = T.ivector('targets')

    # Create neural network model (depending on first command line parameter)
    sys.stderr.write("Building model and compiling functions...\n")
    if vgg:
        network = build_vgg(input_var=input_var, input_shape=(None, 3, img_size, img_size), classes=classes)
    else:
        network = build_alex(input_var=input_var, input_shape=(None, 3, img_size, img_size), classes=classes)

    total_params = count_params(network, trainable=True)
    sys.stderr.write("Number of parameters in model: %d\n" % total_params)

    # Create a loss expression for training, i.e., a scalar objective we want
    # to minimize (for our multi-class problem, it is the cross-entropy loss):
    prediction = get_output(network)

    loss = objectives.categorical_crossentropy(prediction, target_var)
    loss = loss.mean()
    # weight decay
    all_layers = get_all_layers(network)
    l2_penalty = regularization.regularize_layer_params(all_layers, regularization.l2) * 5e-4
    loss += l2_penalty
    # We could add some weight decay as well here, see lasagne.regularization.

    params = get_all_params(network, trainable=True)

    sh_lr = theano.shared(lr)
    updates = my_updates.sgd(loss, params, learning_rate=sh_lr)
    stats_updates = get_bn_stats_updates(network)
    updates.update(stats_updates)

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
    logger = open('./logs/imagenet_trainer{}-{}.log'.format(num_trainer, trainer_id), 'w')

    test_prediction = get_output(network, deterministic=True)
    test_fn = theano.function([input_var], test_prediction)

    # Finally, launch the training loop.
    sys.stderr.write("Starting training...\n")
    # We iterate over epochs:
    it = 0
    indices = np.arange(n_batches)
    it_time = 0
    it_cpu = 0

    for epoch in range(num_epochs):
        # In each epoch, we do a full pass over the training data:
        np.random.shuffle(indices)
        i = 0
        while i < n_batches:
            idx = indices[i]
            inputs, targets = load_batch_data(batch_path[idx])
            if inputs is None or targets is None:
                # NAS host down for some reason, wait and reconnect
                log("NAS read batch{} data error!\n".format(idx), logger)
                continue
            i += 1
            it += 1
            start_time = time.time()
            start_cpu = time.process_time()

            # download params from server
            value_dict = ps.recv_params()
            start_count = ps.recv_count()

            # set the received params
            set_params(params, value_dict)

            # calculate gradients and loss, updates params
            result = train_fn(inputs, targets)
            loss = result.pop('loss')

            # upload gradients
            staleness = ps.recv_count() - start_count
            ps.send_updates(result, staleness)

            it_cpu += time.process_time() - start_cpu
            it_time += time.time() - start_time
            log("Done training iteration {} with loss {} staleness {},"
                " wall took {:.3f}s, cpu took {:.3f}\n".format(it, loss, staleness,
                                                               it_time, it_cpu), logger)

            if it % 2000 == 0 and trainer_id == 0:
                np.save('./params/trainer{}_imagenet_pred{}.npy'.format(num_trainer, it), test_fn)

        if epoch + 1 in {18, 28}:
            curr_lr = sh_lr.get_value()
            curr_lr *= 0.1
            sh_lr.set_value(curr_lr)


def set_params(params, values):
    for p in params:
        v = values[p.name]
        p_shape = p.get_value().shape
        p.set_value(v.reshape(p_shape))


if __name__ == '__main__':
    assert len(sys.argv) >= 3
    trainer_id = int(sys.argv[1])
    total_trainers = int(sys.argv[2])
    assert 0 <= trainer_id < total_trainers
    batch_path, _, _ = load_batch_path_label()
    n = len(batch_path) / total_trainers + 1
    batch_path = batch_path[n * trainer_id: n * (trainer_id + 1)]

    if len(sys.argv) == 5:
        host = sys.argv[3]
        port = int(sys.argv[4])
    else:
        host = 'localhost'
        port = 8000

    main(total_trainers, trainer_id, batch_path, host=host, port=port)

