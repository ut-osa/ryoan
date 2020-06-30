import sys
import time
from collections import OrderedDict

import numpy as np
import theano
import theano.tensor as T

import my_obj as objectives
import my_reg as regularization
import my_updates
from my_lasagne import get_output, get_all_params, get_all_layers, count_params, BatchNormLayer
from net import build_vgg as build_cnn

use_ryoan = hasattr(theano, 'trusted_build_network')
if use_ryoan:
    sys.stderr.write("Using Ryoan...\n")


def log(msg, f):
    f.write(msg)
    f.flush()


def unpickle(file):
    try:
        import cPickle as pickle
        fo = open(file, 'rb')
        dict = pickle.load(fo)
        fo.close()
    except ImportError:
        import pickle
        fo = open(file, 'rb')
        dict = pickle.load(fo, encoding='latin1')
        fo.close()
    return dict


def load_data():
    xs = []
    ys = []
    for j in range(5):
        d = unpickle('cifar-10-batches-py/data_batch_{}'.format(j + 1))
        x = d['data']
        y = d['labels']
        xs.append(x)
        ys.append(y)

    d = unpickle('cifar-10-batches-py/test_batch')
    xs.append(d['data'])
    ys.append(d['labels'])

    x = np.concatenate(xs) / np.float32(255)
    y = np.concatenate(ys)
    x = np.dstack((x[:, :1024], x[:, 1024:2048], x[:, 2048:]))
    x = x.reshape((x.shape[0], 32, 32, 3)).transpose(0, 3, 1, 2)

    # subtract per-pixel mean
    pixel_mean = np.mean(x[0:50000], axis=0)
    # pickle.dump(pixel_mean, open("cifar10-pixel_mean.pkl","wb"))
    x -= pixel_mean

    # create mirrored images
    X_train = x[0:50000, :, :, :]
    Y_train = y[0:50000]

    # X_train_flip = X_train[:, :, :, ::-1]
    # Y_train_flip = Y_train
    # X_train = np.concatenate((X_train, X_train_flip), axis=0)
    # Y_train = np.concatenate((Y_train, Y_train_flip), axis=0)

    X_test = x[50000:, :, :, :]
    Y_test = y[50000:]
    return dict(
        X_train=X_train,
        Y_train=Y_train.astype('int32'),
        X_test=X_test,
        Y_test=Y_test.astype('int32'))


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


def print_addr(params):
    for p in params:
        pointer = p.get_value(True, True).__array_interface__['data']
        print(p, pointer)


def get_bn_stats_updates(network):
    layers = get_all_layers(network)
    stats_updates = OrderedDict()
    for layer in layers:
        if isinstance(layer, BatchNormLayer):
            stats_updates.update(layer.stats_updates)
    return stats_updates


def main(data, num_epochs=60, trainer_id=0, total_trainers=1, multi=False, test_data=None, msec=None):
    if not multi:
        trainer_id = 'one'

    decay = msec is None or msec > 0

    num_epochs += total_trainers * 2
    np.random.seed(12345)
    batch_size = 128
    # Load the dataset
    sys.stderr.write("Loading data...\n")

    X_train, Y_train = data
    if test_data is not None:
        X_test, Y_test = test_data

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
    test_prediction = get_output(network, deterministic=True)

    loss = objectives.categorical_crossentropy(prediction, target_var).mean()
    test_loss = objectives.categorical_crossentropy(test_prediction, target_var).mean()

    # add weight decay
    all_layers = get_all_layers(network)
    l2_penalty = regularization.regularize_layer_params(all_layers, regularization.l2) * 5e-4
    loss += l2_penalty

    # Create update expressions for training
    # Stochastic Gradient Descent (SGD) with momentum
    params = get_all_params(network, trainable=True)
    # bn_stats = get_all_params(network, trainable=False)

    sh_lr = theano.shared(0.1)

    updates = my_updates.sgd(loss, params, learning_rate=sh_lr)
    stats_updates = get_bn_stats_updates(network)
    updates.update(stats_updates)

    name_idx = 0
    for k in updates:
        if k.name is not None:
            k.name = k.name + str(name_idx)
        else:
            k.name = 'param' + str(name_idx)
        name_idx += 1

    # Compile a function performing a training step on a mini-batch (by giving
    # the updates dictionary) and returning the corresponding training loss:
    sys.stderr.write("Compiling functions...\n")
    start_time = time.time()
    if use_ryoan:
        if msec is None:
            logger = open('./logs/trainer{}-{}_time.log'.format(total_trainers, trainer_id), 'w')
        else:
            logger = open('./logs/trainer_ms{}_{}-{}_time.log'.format(msec, total_trainers, trainer_id), 'w')
        if multi:
            rtn = dict([(p.name, updates[p] - p) for p in updates])
            rtn['loss'] = loss
        else:
            rtn = {'loss': loss}
        theano.trusted_build_network([input_var, target_var], rtn,
                                     updates=updates, sh_lr=sh_lr,
                                     test_prediction=test_prediction,
                                     on_unused_input='ignore')
        test_fn = None
    else:
        logger = open('./logs/base_trainer{}-{}.log'.format(total_trainers, trainer_id), 'w')
        train_fn = theano.function([input_var, target_var], loss, updates=updates)
        test_fn = theano.function([input_var, target_var], [test_loss, test_prediction])

    sys.stderr.write("Building network took {:.3f}s\n".format(time.time() - start_time))

    # launch the training loop
    sys.stderr.write("Starting training...\n")
    # We iterate over epochs:
    it = 0
    it_time = 0
    for epoch in range(num_epochs):
        # In each epoch, we do a full pass over the training data:
        for batch in iterate_minibatches(X_train, Y_train, batch_size, shuffle=True, augment=True):
            it += 1
            inputs, targets = batch
            start_time = time.time()
            if use_ryoan:
                # save a model snapshot once a while
                if it % 200 == 0 and trainer_id in {0, 'one'}:
                    theano.trusted_train(inputs, targets, it, total_trainers)
                else:
                    theano.trusted_train(inputs, targets, it, 0)
                it_time += time.time() - start_time
                if it % 10 == 0:
                    log("trainer {}: iteration {} wall took {:3f}s"
                        "\n".format(trainer_id, it, it_time), logger)
            else:
                loss = train_fn(inputs, targets)
                it_time += time.time() - start_time
                log("iteration {} took {:.3f}s with loss {:.6f}\n".format(it, it_time, float(loss)), logger)
                if it % 200 == 0:
                    np.save('./params/{}trainer_cifar_pred{}.npy'.format(total_trainers, it), test_fn)

        if decay and epoch in {30 + total_trainers * 2, 45 + total_trainers * 2}:
            if use_ryoan:
                ryoan_decay_lr(0.1)
            else:
                curr_lr = sh_lr.get_value()
                curr_lr *= 0.1
                sh_lr.set_value(curr_lr)

        if test_fn is not None:
            val_err = 0.0
            val_acc = 0.0
            val_batches = 0.0
            for batch in iterate_minibatches(X_test, Y_test, 500):
                inputs, targets = batch
                err, pred = test_fn(inputs, targets)
                val_err += err
                val_batches += 1
                pred = np.argmax(pred, -1)
                val_acc += np.mean(pred == targets)

            sys.stderr.write("Epoch {}\n".format(epoch))
            sys.stderr.write("\tTest loss:\t\t{:.6f}\n".format(val_err / val_batches if val_batches else val_err))
            sys.stderr.write("\tTest accuracy:\t\t{:.2f} %\n".format(val_acc / val_batches * 100 if val_batches
                                                                     else val_acc * 100))

    if use_ryoan:
        theano.trusted_train(inputs[:batch_size], targets[:batch_size], it, total_trainers)
    logger.close()


def load_trainer_data(num_trainer, trainer_id):
    data_dir = './data/'
    fname = data_dir + 'cifar_trainer{}-{}.npz'.format(num_trainer, trainer_id)
    with np.load(fname) as f:
        return f['arr_0'], f['arr_1']


if __name__ == '__main__':
    if len(sys.argv) == 1:
        data = load_data()
        x = data['X_train']
        y = data['Y_train']
        main(data=(x, y), test_data=(data['X_test'], data['Y_test']))
    else:
        assert len(sys.argv) >= 3
        trainer_id = int(sys.argv[1])
        total_trainers = int(sys.argv[2])
        assert 0 <= trainer_id < total_trainers
        data = load_trainer_data(total_trainers, trainer_id)
        if len(sys.argv) == 4:
            msec = int(sys.argv[3])
        else:
            msec = None
        main(data=data, trainer_id=trainer_id, total_trainers=total_trainers, multi=True, msec=msec)
