import sys
import theano
import numpy as np

from cifar_example import load_data, iterate_minibatches


use_ryoan = hasattr(theano, 'trusted_build_network')
if use_ryoan:
    sys.stderr.write("Using Ryoan...\n")


def test_pkl(p='./params/2trainer_pred_func200.npy'):
    if not use_ryoan:
        print(p)
        test_fn = np.load(p)[()]

    data = load_data()
    x = data['X_test']
    y = data['Y_test']
    print("Done loading data..")

    # And a full pass over the validation data:
    val_acc = 0.0
    val_batches = 0.0
    for batch in iterate_minibatches(x, y, 200, shuffle=False):
        inputs, targets = batch
        if use_ryoan:
            print(inputs.shape, targets.shape)
            theano.trusted_test(inputs, targets, 202)
        else:
            pred = test_fn(inputs, targets)
            val_batches += 1
            pred = np.argmax(pred, -1)
            acc = np.mean(pred == targets)
            val_acc += acc

    print("Test accuracy:\t\t{:.2f} %".format(val_acc / val_batches * 100 if val_batches else val_acc * 100))


if __name__ == '__main__':
    if len(sys.argv) > 1:
        p = sys.argv[1]
        test_pkl(p)
    else:
        test_pkl()
